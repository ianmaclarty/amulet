#include "amulet.h"

#ifdef AM_HAVE_GLOB
#include "SimpleGlob.h"
#endif

static int glob(lua_State *L) {
    am_check_nargs(L, 1);
    if (!lua_istable(L, 1)) return luaL_typerror(L, 1, "table");
#ifdef AM_HAVE_GLOB
    CSimpleGlobTempl<char> glob(SG_GLOB_FULLSORT | SG_GLOB_MARK);
    int i = 1;
    while (true) {
        lua_rawgeti(L, 1, i++);
        const char *pattern = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (pattern == NULL) break;
        glob.Add(pattern);
    }
    lua_newtable(L);
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *file = glob.File(n);
        if (file[strlen(file)-1] == '\\') {
            file[strlen(file)-1] = '/';
        }
        lua_pushstring(L, file);
        lua_rawseti(L, -2, n + 1);
    }
#else
    lua_newtable(L);
#endif
    return 1;
}

void am_open_glob_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"glob", glob},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
