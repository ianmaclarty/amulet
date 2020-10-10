#include "amulet.h"

void am_framebuffer::init(lua_State *L, am_texture2d *texture, int tex_idx, bool depth_buf, bool stencil_buf, 
        glm::dvec4 clear_color, int stencil_clear_value) 
{
    framebuffer_id = am_create_framebuffer();
    color_attachment0 = texture;
    color_attachment0_ref = ref(L, tex_idx);
    if (color_attachment0->image_buffer != NULL) {
        color_attachment0->image_buffer->buffer->update_if_dirty();
    }
    am_bind_framebuffer(framebuffer_id);
    am_set_framebuffer_texture2d(AM_FRAMEBUFFER_COLOR_ATTACHMENT0, AM_TEXTURE_COPY_TARGET_2D, color_attachment0->texture_id);
    width = color_attachment0->width;
    height = color_attachment0->height;
    depth_renderbuffer_id = 0;
    stencil_renderbuffer_id = 0;
    depthstencil_renderbuffer_id = 0;
    has_depth_buf = depth_buf;
    has_stencil_buf = stencil_buf;
    if (depth_buf && stencil_buf && am_gl_requires_combined_depthstencil()) {
        depthstencil_renderbuffer_id = am_create_renderbuffer();
        am_bind_renderbuffer(depthstencil_renderbuffer_id);
        am_set_renderbuffer_storage(AM_RENDERBUFFER_FORMAT_DEPTHSTENCIL, width, height);
        am_set_framebuffer_renderbuffer(AM_FRAMEBUFFER_DEPTHSTENCIL_ATTACHMENT, depthstencil_renderbuffer_id);
        if (depth_buf) {
            am_set_framebuffer_depth_mask(true);
        }
    } else {
        if (depth_buf) {
            depth_renderbuffer_id = am_create_renderbuffer();
            am_bind_renderbuffer(depth_renderbuffer_id);
            am_set_renderbuffer_storage(AM_RENDERBUFFER_FORMAT_DEPTH, width, height);
            am_set_framebuffer_renderbuffer(AM_FRAMEBUFFER_DEPTH_ATTACHMENT, depth_renderbuffer_id);
            am_set_framebuffer_depth_mask(true);
        }
        if (stencil_buf) {
            stencil_renderbuffer_id = am_create_renderbuffer();
            am_bind_renderbuffer(stencil_renderbuffer_id);
            am_set_renderbuffer_storage(AM_RENDERBUFFER_FORMAT_STENCIL, width, height);
            am_set_framebuffer_renderbuffer(AM_FRAMEBUFFER_STENCIL_ATTACHMENT, stencil_renderbuffer_id);
        }
    }
    am_framebuffer_status status = am_check_framebuffer_status();
    if (status != AM_FRAMEBUFFER_STATUS_COMPLETE) {
        luaL_error(L, "framebuffer incomplete");
    }
    am_framebuffer::clear_color = clear_color;
    am_framebuffer::stencil_clear_value = stencil_clear_value;
    user_projection = false;
    double w = (double)width;
    double h = (double)height;
    projection = glm::ortho(-w*0.5, w*0.5, -h*0.5, h*0.5, -1.0, 1.0);
}

void am_framebuffer::clear(bool clear_colorbuf, bool clear_depth, bool clear_stencil) {
    am_set_scissor_test_enabled(false);
    if (clear_colorbuf) {
        am_set_framebuffer_clear_color(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    }
    if (clear_depth) {
        am_set_framebuffer_depth_mask(true);
    }
    if (clear_stencil) {
        am_set_framebuffer_clear_stencil_val(stencil_clear_value);
    }
    if (clear_colorbuf || clear_depth || clear_stencil) {
        am_clear_framebuffer(clear_colorbuf, clear_depth, clear_stencil);
    }
}

void am_framebuffer::destroy(lua_State *L) {
    am_delete_framebuffer(framebuffer_id);
    if (depth_renderbuffer_id != 0) {
        am_delete_renderbuffer(depth_renderbuffer_id);
        depth_renderbuffer_id = 0;
    }
    if (stencil_renderbuffer_id != 0) {
        am_delete_renderbuffer(stencil_renderbuffer_id);
        stencil_renderbuffer_id = 0;
    }
    if (depthstencil_renderbuffer_id != 0) {
        am_delete_renderbuffer(depthstencil_renderbuffer_id);
        depthstencil_renderbuffer_id = 0;
    }
    color_attachment0 = NULL;
    unref(L, color_attachment0_ref);
    color_attachment0_ref = LUA_NOREF;
}

static int create_framebuffer(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_framebuffer *fb = am_new_userdata(L, am_framebuffer);
    am_texture2d *texture = am_get_userdata(L, am_texture2d, 1);
    bool depth_buf = nargs > 1 && lua_toboolean(L, 2);
    bool stencil_buf = nargs > 2 && lua_toboolean(L, 3);
    glm::dvec4 clear_color = glm::dvec4(0.0f, 0.0f, 0.0f, 1.0f);
    int stencil_clear_value = 0;
    fb->init(L, texture, 1, depth_buf, stencil_buf, clear_color, stencil_clear_value);
    fb->clear(true, depth_buf, stencil_buf);
    return 1;
}

static int render_node_to_framebuffer(lua_State *L) {
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 2);
    am_render_state *rstate = am_global_render_state;
    if (fb->color_attachment0->image_buffer != NULL) {
        fb->color_attachment0->image_buffer->buffer->update_if_dirty();
    }
    rstate->do_render(&node, 1, fb->framebuffer_id, false, fb->clear_color, fb->stencil_clear_value,
        0, 0, fb->width, fb->height, fb->width, fb->height, fb->projection, fb->has_depth_buf);
    if (fb->color_attachment0->has_mipmap) {
        am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, fb->color_attachment0->texture_id);
        am_generate_mipmap(AM_TEXTURE_BIND_TARGET_2D);
    }
    return 0;
}

static int render_children_to_framebuffer(lua_State *L) {
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 2);
    am_render_state *rstate = am_global_render_state;
    if (fb->color_attachment0->image_buffer != NULL) {
        fb->color_attachment0->image_buffer->buffer->update_if_dirty();
    }
    am_scene_node tmpnode;
    tmpnode.children = node->children;
    am_scene_node *tmparr = &tmpnode;
    rstate->do_render(&tmparr, 1, fb->framebuffer_id, false, fb->clear_color, fb->stencil_clear_value,
        0, 0, fb->width, fb->height, fb->width, fb->height, fb->projection, fb->has_depth_buf);
    if (fb->color_attachment0->has_mipmap) {
        am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, fb->color_attachment0->texture_id);
        am_generate_mipmap(AM_TEXTURE_BIND_TARGET_2D);
    }
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
    if (color_at->width == w && color_at->height == h) return 0; // nothing to do
    if (color_at->image_buffer != NULL) {
        return luaL_error(L, "cannot resize a framebuffer whose color attachment has a backing buffer");
    }
    if (color_at->has_mipmap) {
        return luaL_error(L, "cannot resize a framebuffer whose color attachment is mipmapped");
    }
    if (color_at->swrap == AM_TEXTURE_WRAP_REPEAT && !am_is_power_of_two(color_at->width)) {
        return luaL_error(L, "width must be a power of 2 when using repeat wrapping");
    }
    if (color_at->swrap == AM_TEXTURE_WRAP_MIRRORED_REPEAT && !am_is_power_of_two(color_at->width)) {
        return luaL_error(L, "width must be a power of 2 when using mirrored repeat wrapping");
    }
    if (color_at->twrap == AM_TEXTURE_WRAP_REPEAT && !am_is_power_of_two(color_at->height)) {
        return luaL_error(L, "height must be a power of 2 when using repeat wrapping");
    }
    if (color_at->twrap == AM_TEXTURE_WRAP_MIRRORED_REPEAT && !am_is_power_of_two(color_at->height)) {
        return luaL_error(L, "height must be a power of 2 when using mirrored repeat wrapping");
    }
    color_at->width = w;
    color_at->height = h;
    int tex_bytes = am_compute_pixel_size(color_at->format, color_at->type) * w * h;
    void *data = malloc(tex_bytes);
    memset(data, 0, tex_bytes);
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, color_at->texture_id);
    am_set_texture_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, color_at->format, w, h, color_at->type, data);
    free(data);
    bool depth_buf = fb->has_depth_buf;
    bool stencil_buf = fb->has_stencil_buf;
    glm::dvec4 clear_color = fb->clear_color;
    int stencil_clear_value = fb->stencil_clear_value;
    bool user_proj = fb->user_projection;
    glm::dmat4 old_proj = fb->projection;
    lua_unsafe_pushuserdata(L, color_at);
    int tex_idx = lua_gettop(L);
    fb->destroy(L);
    fb->init(L, color_at, tex_idx, depth_buf, stencil_buf, clear_color, stencil_clear_value);
    lua_remove(L, tex_idx);
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
    if (color_tex->format != AM_TEXTURE_FORMAT_RGBA || color_tex->type != AM_TEXTURE_TYPE_UBYTE) {
        return luaL_error(L, "sorry, read_back is only supported for textures with format RGBA8");
    }
    if (color_tex->image_buffer == NULL) {
        return luaL_error(L, "framebuffer texture has no backing image buffer to write to");
    }
    am_buffer *color_buffer = color_tex->image_buffer->buffer;
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

static void get_stencil_clear_value(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    lua_pushinteger(L, fb->stencil_clear_value);
}
static void set_stencil_clear_value(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    fb->stencil_clear_value = luaL_checkinteger(L, 3);
}

static am_property stencil_clear_value_property = {get_stencil_clear_value, set_stencil_clear_value};

static void get_projection(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    am_new_userdata(L, am_mat4)->m = fb->projection;
}

static void set_projection(lua_State *L, void *obj) {
    am_framebuffer *fb = (am_framebuffer*)obj;
    if (lua_isnil(L, 3)) {
        fb->user_projection = false;
        double w = (double)fb->width;
        double h = (double)fb->height;
        fb->projection = glm::ortho(-w*0.5, w*0.5, -h*0.5, h*0.5, -1.0, 1.0);
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
    am_register_property(L, "stencil_clear_value", &stencil_clear_value_property);
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

void am_color_mask_node::render(am_render_state *rstate) {
    am_color_mask_state old = rstate->active_color_mask_state;
    rstate->active_color_mask_state.set(r, g, b, a);
    render_children(rstate);
    rstate->active_color_mask_state.restore(&old);
}

static int create_color_mask_node(lua_State *L) {
    am_check_nargs(L, 4);
    am_color_mask_node *node = am_new_userdata(L, am_color_mask_node);
    node->tags.push_back(L, AM_TAG_COLOR_MASK);
    node->r = lua_toboolean(L, 1);
    node->g = lua_toboolean(L, 2);
    node->b = lua_toboolean(L, 3);
    node->a = lua_toboolean(L, 4);
    return 1;
}

static void get_r(lua_State *L, void *obj) {
    am_color_mask_node *node = (am_color_mask_node*)obj;
    lua_pushboolean(L, node->r);
}

static void set_r(lua_State *L, void *obj) {
    am_color_mask_node *node = (am_color_mask_node*)obj;
    node->r = lua_toboolean(L, 3);
}

static am_property r_property = {get_r, set_r};

static void get_g(lua_State *L, void *obj) {
    am_color_mask_node *node = (am_color_mask_node*)obj;
    lua_pushboolean(L, node->g);
}

static void set_g(lua_State *L, void *obj) {
    am_color_mask_node *node = (am_color_mask_node*)obj;
    node->g = lua_toboolean(L, 3);
}

static am_property g_property = {get_g, set_g};

static void get_b(lua_State *L, void *obj) {
    am_color_mask_node *node = (am_color_mask_node*)obj;
    lua_pushboolean(L, node->b);
}

static void set_b(lua_State *L, void *obj) {
    am_color_mask_node *node = (am_color_mask_node*)obj;
    node->b = lua_toboolean(L, 3);
}

static am_property b_property = {get_b, set_b};

static void get_a(lua_State *L, void *obj) {
    am_color_mask_node *node = (am_color_mask_node*)obj;
    lua_pushboolean(L, node->a);
}

static void set_a(lua_State *L, void *obj) {
    am_color_mask_node *node = (am_color_mask_node*)obj;
    node->a = lua_toboolean(L, 3);
}

static am_property a_property = {get_a, set_a};

static void register_color_mask_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "red", &r_property);
    am_register_property(L, "green", &g_property);
    am_register_property(L, "blue", &b_property);
    am_register_property(L, "alpha", &a_property);

    am_register_metatable(L, "color_mask", MT_am_color_mask_node, MT_am_scene_node);
}

void am_open_framebuffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"framebuffer",    create_framebuffer},
        {"viewport",       create_viewport_node},
        {"color_mask",     create_color_mask_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_framebuffer_mt(L);
    register_viewport_node_mt(L);
    register_color_mask_node_mt(L);
}
