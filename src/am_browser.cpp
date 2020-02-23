#include "amulet.h"

static int eval_js(lua_State *L) {
#ifdef AM_BACKEND_EMSCRIPTEN
    am_check_nargs(L, 1);
    const char *js = lua_tostring(L, 1);
    if (js == NULL) return luaL_error(L, "expecting a string at position 1");
    int res_ptr = EM_ASM_INT({
        var js_str = UTF8ToString($0);
        var res = eval(js_str);
        var res_str;
        if (res === undefined) {
            res_str = "null";
        } else {
            res_str = JSON.stringify(res);
        }
        var len = Module.lengthBytesUTF8(res_str) + 1;
        var buffer = Module._malloc(len);
        Module.stringToUTF8(res_str, buffer, len);
        return buffer;
    }, (int)js);
    char *res_str = (char*)res_ptr;
    lua_pushstring(L, res_str);
    free(res_str);
    lua_replace(L, 1);
    return am_parse_json(L);
#else
    lua_pushnil(L);
    return 1;
#endif
}

void am_open_browser_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"eval_js", eval_js},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
