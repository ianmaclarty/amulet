#include "amulet.h"

static int current_time(lua_State *L) {
    lua_pushnumber(L, am_get_current_time());
    return 1;
}

static int frame_time(lua_State *L) {
    lua_pushnumber(L, am_get_frame_time());
    return 1;
}

static int delta_time(lua_State *L) {
    lua_pushnumber(L, am_get_delta_time());
    return 1;
}

static int average_fps(lua_State *L) {
    lua_pushnumber(L, am_get_average_fps());
    return 1;
}

void am_open_time_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"current_time", current_time},
        {"frame_time", frame_time},
        {"delta_time", delta_time},
        {"average_fps", average_fps},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
