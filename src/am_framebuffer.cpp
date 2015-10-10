#include "amulet.h"

void am_framebuffer::init(lua_State *L, am_texture2d *texture, bool depth_buf, bool stencil_buf, glm::vec4 clear_color) {
    framebuffer_id = am_create_framebuffer();
    if (texture->buffer != NULL) {
        texture->buffer->update_if_dirty();
    }
    color_attachment0 = texture;
    color_attachment0_ref = ref(L, 1);
    am_bind_framebuffer(framebuffer_id);
    am_set_framebuffer_texture2d(AM_FRAMEBUFFER_COLOR_ATTACHMENT0, AM_TEXTURE_COPY_TARGET_2D, texture->texture_id);
    width = texture->width;
    height = texture->height;
    depth_renderbuffer_id = 0;
    if (depth_buf) {
        depth_renderbuffer_id = am_create_renderbuffer();
        am_bind_renderbuffer(depth_renderbuffer_id);
        am_set_renderbuffer_storage(AM_RENDERBUFFER_FORMAT_DEPTH_COMPONENT24, width, height);
        am_set_framebuffer_renderbuffer(AM_FRAMEBUFFER_DEPTH_ATTACHMENT, depth_renderbuffer_id);
        am_set_framebuffer_depth_mask(true);
    }
    stencil_renderbuffer_id = 0;
    if (stencil_buf) {
        stencil_renderbuffer_id = am_create_renderbuffer();
        am_bind_renderbuffer(stencil_renderbuffer_id);
        am_set_renderbuffer_storage(AM_RENDERBUFFER_FORMAT_STENCIL_INDEX8, width, height);
        am_set_framebuffer_renderbuffer(AM_FRAMEBUFFER_STENCIL_ATTACHMENT, stencil_renderbuffer_id);
    }
    am_framebuffer_status status = am_check_framebuffer_status();
    if (status != AM_FRAMEBUFFER_STATUS_COMPLETE) {
        luaL_error(L, "framebuffer incomplete");
    }
    clear_color = clear_color;
    user_projection = false;
    projection = glm::ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
}

void am_framebuffer::clear(bool clear_colorbuf, bool clear_depth, bool clear_stencil) {
    if (clear_colorbuf) {
        am_set_framebuffer_clear_color(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    }
    if (clear_depth) {
        am_set_framebuffer_depth_mask(true);
    }
    am_clear_framebuffer(clear_colorbuf, clear_depth, clear_stencil);
}

void am_framebuffer::destroy(lua_State *L) {
    am_delete_framebuffer(framebuffer_id);
    if (depth_renderbuffer_id != 0) {
        am_delete_renderbuffer(depth_renderbuffer_id);
    }
    color_attachment0 = NULL;
    unref(L, color_attachment0_ref);
    color_attachment0_ref = LUA_NOREF;
}

static int create_framebuffer(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_framebuffer *fb = am_new_userdata(L, am_framebuffer);
    fb->width = -1;
    fb->height = -1;
    am_texture2d *texture = am_get_userdata(L, am_texture2d, 1);
    bool depth_buf = nargs > 1 && lua_toboolean(L, 2);
    bool stencil_buf = nargs > 2 && lua_toboolean(L, 3);
    glm::vec4 clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    fb->init(L, texture, depth_buf, stencil_buf, clear_color);
    fb->clear(false, depth_buf, stencil_buf);
    return 1;
}

static int render_node_to_framebuffer(lua_State *L) {
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 2);
    am_render_state *rstate = &am_global_render_state;
    if (fb->color_attachment0->buffer != NULL) {
        fb->color_attachment0->buffer->update_if_dirty();
    }
    rstate->do_render(node, fb->framebuffer_id, false, fb->clear_color,
        0, 0, fb->width, fb->height, fb->projection, fb->depth_renderbuffer_id != 0);
    return 0;
}

static int render_children_to_framebuffer(lua_State *L) {
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 2);
    am_render_state *rstate = &am_global_render_state;
    if (fb->color_attachment0->buffer != NULL) {
        fb->color_attachment0->buffer->update_if_dirty();
    }
    am_scene_node tmpnode;
    tmpnode.children = node->children;
    rstate->do_render(&tmpnode, fb->framebuffer_id, false, fb->clear_color,
        0, 0, fb->width, fb->height, fb->projection, fb->depth_renderbuffer_id != 0);
    return 0;
}

static int framebuffer_gc(lua_State *L) {
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    fb->destroy(L);
    return 0;
}

static int clear_framebuffer(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    am_bind_framebuffer(fb->framebuffer_id);
    bool clear_color = true;
    bool clear_depth = true;
    bool clear_stencil = true;
    if (nargs > 1) {
        clear_color = lua_toboolean(L, 2);
    }
    if (nargs > 2) {
        clear_depth = lua_toboolean(L, 3);
    }
    if (nargs > 3) {
        clear_stencil = lua_toboolean(L, 4);
    }
    fb->clear(clear_color, clear_depth, clear_stencil);
    return 0;
}

static int resize(lua_State *L) {
    am_check_nargs(L, 3);
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    int w = luaL_checkinteger(L, 2);
    int h = luaL_checkinteger(L, 3);
    if (w <= 0) return luaL_error(L, "width must be positive");
    if (h <= 0) return luaL_error(L, "height must be positive");
    am_texture2d *color_at = fb->color_attachment0;
    if (color_at->buffer != NULL) {
        return luaL_error(L, "cannot resize a framebuffer whose color attachment has a backing buffer");
    }
    if (color_at->has_mipmap) {
        return luaL_error(L, "cannot resize a framebuffer whose color attachment is mipmapped");
    }
    color_at->width = w;
    color_at->height = h;
    void *data = malloc(am_compute_pixel_size(color_at->format, color_at->type) * w * h);
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, color_at->texture_id);
    am_set_texture_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, color_at->format, w, h, color_at->type, data);
    free(data);
    bool depth_buf = fb->depth_renderbuffer_id != 0;
    bool stencil_buf = fb->stencil_renderbuffer_id != 0;
    glm::vec4 clear_color = fb->clear_color;
    bool user_proj = fb->user_projection;
    glm::mat4 old_proj = fb->projection;
    fb->destroy(L);
    fb->init(L, color_at, depth_buf, stencil_buf, clear_color);
    fb->clear(true, depth_buf, stencil_buf);
    if (user_proj) {
        // restore user-specified projection
        fb->user_projection = true;
        fb->projection = old_proj;
    }
    return 0;
}

static int read_back(lua_State *L) {
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    am_texture2d *color_tex = fb->color_attachment0;
    if (color_tex->format != AM_TEXTURE_FORMAT_RGBA || color_tex->type != AM_PIXEL_TYPE_UBYTE) {
        return luaL_error(L, "sorry, read_back is only supported for textures with format/type rgba/8");
    }
    am_buffer *color_buffer = color_tex->buffer;
    if (color_buffer == NULL) {
        return luaL_error(L, "framebuffer texture has no backing buffer to write to");
    }
    color_buffer->update_if_dirty();
    am_bind_framebuffer(fb->framebuffer_id);
    am_read_pixels(0, 0, color_tex->width, color_tex->height, (void*)color_buffer->data);
    return 0;
}

static void get_clear_color(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    am_new_userdata(L, am_vec4)->v = fb->clear_color;
}
static void set_clear_color(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    fb->clear_color = am_get_userdata(L, am_vec4, 3)->v;
}

static am_property clear_color_property = {get_clear_color, set_clear_color};

static void get_projection(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    am_new_userdata(L, am_mat4)->m = fb->projection;
}

static void set_projection(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    if (lua_isnil(L, 3)) {
        fb->user_projection = false;
        fb->projection = glm::ortho(0.0f, (float)fb->width, 0.0f, (float)fb->height, -1.0f, 1.0f);
    } else {
        fb->user_projection = true;
        fb->projection = am_get_userdata(L, am_mat4, 3)->m;
    }
}

static am_property projection_property = {get_projection, set_projection};

static void get_pixel_width(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    lua_pushinteger(L, fb->width);
}

static void get_pixel_height(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    lua_pushinteger(L, fb->height);
}

static am_property pixel_width_property = {get_pixel_width, NULL};
static am_property pixel_height_property = {get_pixel_height, NULL};

static void register_framebuffer_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, framebuffer_gc, 0);
    lua_setfield(L, -2, "__gc");

    am_register_property(L, "clear_color", &clear_color_property);
    am_register_property(L, "projection", &projection_property);
    am_register_property(L, "pixel_width", &pixel_width_property);
    am_register_property(L, "pixel_height", &pixel_height_property);

    lua_pushcclosure(L, render_node_to_framebuffer, 0);
    lua_setfield(L, -2, "render");

    lua_pushcclosure(L, render_children_to_framebuffer, 0);
    lua_setfield(L, -2, "render_children");

    lua_pushcclosure(L, clear_framebuffer, 0);
    lua_setfield(L, -2, "clear");

    lua_pushcclosure(L, read_back, 0);
    lua_setfield(L, -2, "read_back");

    lua_pushcclosure(L, resize, 0);
    lua_setfield(L, -2, "resize");

    am_register_metatable(L, "framebuffer", MT_am_framebuffer, 0);
}

void am_viewport_node::render(am_render_state *rstate) {
    am_viewport_state old = rstate->active_viewport_state;
    rstate->active_viewport_state.set(x, y, w, h);
    render_children(rstate);
    rstate->active_viewport_state.restore(&old);
}

static int create_viewport_node(lua_State *L) {
    am_check_nargs(L, 4);
    am_viewport_node *node = am_new_userdata(L, am_viewport_node);
    node->tags.push_back(L, AM_TAG_VIEWPORT);
    node->x = luaL_checkinteger(L, 1);
    node->y = luaL_checkinteger(L, 2);
    node->w = luaL_checkinteger(L, 3);
    node->h = luaL_checkinteger(L, 4);
    return 1;
}

static void get_x(lua_State *L, void *obj) {
    am_viewport_node *node = (am_viewport_node*)obj;
    lua_pushinteger(L, node->x);
}

static void set_x(lua_State *L, void *obj) {
    am_viewport_node *node = (am_viewport_node*)obj;
    node->x = luaL_checkinteger(L, 3);
}

static am_property x_property = {get_x, set_x};

static void get_y(lua_State *L, void *obj) {
    am_viewport_node *node = (am_viewport_node*)obj;
    lua_pushinteger(L, node->y);
}

static void set_y(lua_State *L, void *obj) {
    am_viewport_node *node = (am_viewport_node*)obj;
    node->y = luaL_checkinteger(L, 3);
}

static am_property y_property = {get_y, set_y};

static void get_w(lua_State *L, void *obj) {
    am_viewport_node *node = (am_viewport_node*)obj;
    lua_pushinteger(L, node->w);
}

static void set_w(lua_State *L, void *obj) {
    am_viewport_node *node = (am_viewport_node*)obj;
    node->w = luaL_checkinteger(L, 3);
}

static am_property w_property = {get_w, set_w};

static void get_h(lua_State *L, void *obj) {
    am_viewport_node *node = (am_viewport_node*)obj;
    lua_pushinteger(L, node->h);
}

static void set_h(lua_State *L, void *obj) {
    am_viewport_node *node = (am_viewport_node*)obj;
    node->h = luaL_checkinteger(L, 3);
}

static am_property h_property = {get_h, set_h};

static void register_viewport_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "left", &x_property);
    am_register_property(L, "bottom", &y_property);
    am_register_property(L, "width", &w_property);
    am_register_property(L, "height", &h_property);

    am_register_metatable(L, "viewport", MT_am_viewport_node, MT_am_scene_node);
}

void am_open_framebuffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"framebuffer",    create_framebuffer},
        {"viewport",       create_viewport_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_framebuffer_mt(L);
    register_viewport_node_mt(L);
}
