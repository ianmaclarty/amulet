#include "amulet.h"

static void init_metatable_ref_slots(lua_State *L);

lua_State *am_engine_init() {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    init_metatable_ref_slots(L);

    am_lua_window_module_setup(L);
    am_lua_gl_module_setup(L);

    return L;
}

void am_engine_destroy(lua_State *L) {
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
