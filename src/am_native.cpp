#include "amulet.h"

static int open_file_dialog(lua_State *L) {
    char *filename = am_open_file_dialog();
    if (filename == NULL) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, filename);
        free(filename);
    }
    return 1;
}

void am_open_native_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"open_file_dialog", open_file_dialog},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}