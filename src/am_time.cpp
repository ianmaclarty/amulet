#include "amulet.h"

void am_update_times(lua_State *L, double dt) {
    lua_pushnumber(L, dt);
    lua_pushnumber(L, am_get_current_time());
    am_call_amulet(L, "_update_times", 2, 0);
}

static int current_time(lua_State *L) {
    lua_pushnumber(L, am_get_current_time());
    return 1;
}

void am_open_time_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"current_time", current_time},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
