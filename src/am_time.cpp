#include "amulet.h"

bool am_record_perf_timings = false;

double am_last_frame_lua_time = 0.0;
double am_last_frame_draw_time = 0.0;
double am_last_frame_audio_time = 0.0;

static int current_time(lua_State *L) {
    lua_pushnumber(L, am_get_current_time());
    return 1;
}

static int last_frame_lua_time(lua_State *L) {
    lua_pushnumber(L, am_last_frame_lua_time);
    return 1;
}

static int last_frame_draw_time(lua_State *L) {
    lua_pushnumber(L, am_last_frame_draw_time);
    return 1;
}

static int last_frame_audio_time(lua_State *L) {
    lua_pushnumber(L, am_last_frame_audio_time);
    return 1;
}

static int enable_perf_timings(lua_State *L) {
    am_record_perf_timings = true;
    return 0;
}

void am_open_time_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"current_time", current_time},
        {"last_frame_lua_time", last_frame_lua_time},
        {"last_frame_draw_time", last_frame_draw_time},
        {"last_frame_audio_time", last_frame_audio_time},
        {"enable_perf_timings", enable_perf_timings},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
