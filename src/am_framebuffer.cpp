#include "amulet.h"

static int create_framebuffer(lua_State *L) {
    am_check_nargs(L, 2);
    if (!lua_istable(L, 1)) return luaL_error(L, "expecting a table in position 1");
    am_framebuffer *fb = am_new_userdata(L, am_framebuffer);
    fb->width = -1;
    fb->height = -1;
    fb->fbo = am_create_framebuffer();
    am_texture2d *texture = am_get_userdata(L, am_texture2d, 2);
    fb->color_attachment0_ref = fb->ref(L, 2);
    am_bind_framebuffer(fb->fbo);
    am_set_framebuffer_texture2d(AM_FRAMEBUFFER_COLOR_ATTACHMENT0, AM_TEXTURE_COPY_TARGET_2D, texture->texture_id);
    fb->width = texture->width;
    fb->height = texture->height;
    return 1;
}

static int framebuffer_gc(lua_State *L) {
    am_framebuffer *fb = am_get_userdata(L, am_framebuffer, 1);
    am_delete_framebuffer(fb->fbo);
    return 0;
}

static void register_framebuffer_mt(lua_State *L) {
    lua_newtable(L);

    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, framebuffer_gc, 0);
    lua_setfield(L, -2, "__gc");

    lua_pushstring(L, "framebuffer");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_framebuffer, 0);
}

void am_open_framebuffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"create_framebuffer",    create_framebuffer},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_framebuffer_mt(L);
}
