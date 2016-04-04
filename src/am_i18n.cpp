#include "amulet.h"

static int language(lua_State *L) {
    lua_pushstring(L, am_preferred_language());
    return 1;
}

void am_open_i18n_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"language", language},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
