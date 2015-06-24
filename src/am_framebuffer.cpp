#include "amulet.h"

static int create_framebuffer(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_framebuffer *fb = am_new_userdata(L, am_framebuffer);
    fb->width = -1;
    fb->height = -1;
    fb->framebuffer_id = am_create_framebuffer();
    am_texture2d *texture = am_get_userdata(L, am_texture2d, 1);
    if (texture->buffer != NULL) {
        texture->buffer->update_if_dirty();
    }
    fb->color_attachment0 = texture;
    fb->color_attachment0_ref = fb->ref(L, 1);
    am_bind_framebuffer(fb->framebuffer_id);
    am_set_framebuffer_texture2d(AM_FRAMEBUFFER_COLOR_ATTACHMENT0, AM_TEXTURE_COPY_TARGET_2D, texture->texture_id);
    fb->width = texture->width;
    fb->height = texture->height;
    fb->depth_renderbuffer_id = 0;
    if (nargs > 1 && lua_toboolean(L, 2)) {
        fb->depth_renderbuffer_id = am_create_renderbuffer();
        am_bind_renderbuffer(fb->depth_renderbuffer_id);
        am_set_renderbuffer_storage(AM_RENDERBUFFER_FORMAT_DEPTH_COMPONENT24,
            fb->width, fb->height);
        am_set_framebuffer_renderbuffer(AM_FRAMEBUFFER_DEPTH_ATTACHMENT, fb->depth_renderbuffer_id);
        am_clear_framebuffer(false, true, false); // clear depth buffer
    }
    am_framebuffer_status status = am_check_framebuffer_status();
    if (status != AM_FRAMEBUFFER_STATUS_COMPLETE) {
        return luaL_error(L, "framebuffer incomplete");
    }
    fb->clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return 1;
}

static int render_node_to_framebuffer(lua_State *L) {
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 2);
    am_render_state *rstate = &am_global_render_state;
    if (fb->color_attachment0->buffer != NULL) {
        fb->color_attachment0->buffer->update_if_dirty();
    }
    rstate->do_render(node, fb->framebuffer_id, false, fb->width, fb->height, fb->depth_renderbuffer_id != 0);
    return 0;
}

static int framebuffer_gc(lua_State *L) {
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    am_delete_framebuffer(fb->framebuffer_id);
    if (fb->depth_renderbuffer_id != 0) {
        am_delete_renderbuffer(fb->depth_renderbuffer_id);
    }
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
        am_set_framebuffer_clear_color(fb->clear_color.r, fb->clear_color.g, fb->clear_color.b, fb->clear_color.a);
    }
    if (nargs > 2) {
        clear_depth = lua_toboolean(L, 3);
        am_set_framebuffer_depth_mask(true);
    }
    if (nargs > 3) {
        clear_stencil = lua_toboolean(L, 4);
    }
    am_clear_framebuffer(clear_color, clear_depth, clear_stencil);
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
        return luaL_error(L, "framebuffer texture has no buffer");
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

static void register_framebuffer_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, framebuffer_gc, 0);
    lua_setfield(L, -2, "__gc");

    am_register_property(L, "clear_color", &clear_color_property);

    lua_pushcclosure(L, render_node_to_framebuffer, 0);
    lua_setfield(L, -2, "render");

    lua_pushcclosure(L, clear_framebuffer, 0);
    lua_setfield(L, -2, "clear");

    lua_pushcclosure(L, read_back, 0);
    lua_setfield(L, -2, "read_back");

    am_register_metatable(L, "framebuffer", MT_am_framebuffer, 0);
}

void am_open_framebuffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"framebuffer",    create_framebuffer},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_framebuffer_mt(L);
}
