#include "amulet.h"

static void init_metatable_ref_slots(lua_State *L);
static void open_stdlualibs(lua_State *L);

lua_State *am_init_engine(bool worker) {
    lua_State *L = luaL_newstate();
    init_metatable_ref_slots(L);
    open_stdlualibs(L);

    if (!worker) {
        am_open_window_module(L);
        am_open_gl_module(L);
    }

    am_no_new_globals(L);

    return L;
}

void am_destroy_engine(lua_State *L) {
    lua_close(L);
}

static void init_metatable_ref_slots(lua_State *L) {
    int i, j;
    do {
        lua_pushboolean(L, 1);
        i = luaL_ref(L, LUA_REGISTRYINDEX);
    } while (i < AM_METATABLE_START_ID - 1);
    if (i != AM_METATABLE_START_ID - 1) {
        fprintf(stderr, "Internal Error: more refs than AM_METATABLE_START_ID\n");
        exit(1);
    }
    for (i = AM_METATABLE_START_ID; i < AM_METATABLE_END_ID; i++) {
        lua_pushboolean(L, 1);
        j = luaL_ref(L, LUA_REGISTRYINDEX);
        if (i != j) {
            fprintf(stderr, "Internal Error: non-congiguous refs after AM_METATABLE_START_ID\n");
            exit(1);
        }
        j++;
    }
}

static void open_stdlualibs(lua_State *L) {
    luaL_requiref(L, "base", luaopen_base, 0);
    lua_pop(L, 1);
    luaL_requiref(L, "package", luaopen_package, 0);
    lua_pop(L, 1);
    luaL_requiref(L, "math", luaopen_math, 0);
    lua_pop(L, 1);
    luaL_requiref(L, "coroutine", luaopen_coroutine, 0);
    lua_pop(L, 1);
    luaL_requiref(L, "string", luaopen_string, 0);
    lua_pop(L, 1);
    luaL_requiref(L, "table", luaopen_table, 0);
    lua_pop(L, 1);
    luaL_requiref(L, "os", luaopen_os, 0);
    lua_pop(L, 1);
    luaL_requiref(L, "debug", luaopen_debug, 0);
    lua_pop(L, 1);
}
