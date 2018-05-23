#include "amulet.h"

static int vbo_gc(lua_State *L) {
    am_vbo *vbo = am_get_userdata(L, am_vbo, 1);
    if (vbo->id != 0) {
        am_bind_buffer(vbo->target, 0);
        am_delete_buffer(vbo->id);
        vbo->id = 0;
    }
    return 0;
}

static void register_vbo_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L, vbo_gc, 0);
    lua_setfield(L, -2, "__gc");

    am_register_metatable(L, "vbo", MT_am_vbo, 0);
}

void am_open_vbo_module(lua_State *L) {
    register_vbo_mt(L);
}
