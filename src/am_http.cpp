#include "amulet.h"

#ifdef AM_BACKEND_EMSCRIPTEN
static int next_http_req_id = 0;

struct am_http_request : am_nonatomic_userdata {
    int id;
};
#else
struct am_http_request : am_nonatomic_userdata {
};
#endif

static int http_request(lua_State *L) {
#ifdef AM_BACKEND_EMSCRIPTEN
    int nargs = am_check_nargs(L, 1);
    const char *url = lua_tostring(L, 1);
    if (url == NULL) return luaL_error(L, "expecting a string at position 1");
    const char *data = NULL;
    if (nargs > 1) {
        data = lua_tostring(L, 2);
        if (data == NULL) return luaL_error(L, "expecting a string at position 2");
    }
    am_http_request *req = am_new_userdata(L, am_http_request);
    req->id = next_http_req_id++;
    EM_ASM_INT({
        if (!window.amulet.http_reqs) window.amulet.http_reqs = {};
        var xhr = new XMLHttpRequest();
        window.amulet.http_reqs[$0] = xhr;
        var url = UTF8ToString($1);
        var data = null;
        if ($2 == 0) {
            xhr.open('GET', url, true);
            xhr.send();
        } else {
            data = UTF8ToString($2);
            xhr.open('POST', url, true);
            xhr.send(data);
        }
    }, req->id, (int)url, (int)data);
    return 1;
#else
    // NYI
    lua_pushnil(L);
    return 1;
#endif
}

static int http_req_gc(lua_State *L) {
#ifdef AM_BACKEND_EMSCRIPTEN
    am_http_request *req = am_get_userdata(L, am_http_request, 1);
    EM_ASM_INT({
        delete window.amulet.http_reqs[$0];
    }, req->id);
#endif
    return 0;
}

static void get_status(lua_State *L, void *obj) {
#ifdef AM_BACKEND_EMSCRIPTEN
    am_http_request *req = (am_http_request*)obj;
    int ready_state = EM_ASM_INT({
        return window.amulet.http_reqs[$0].readyState;
    }, req->id);
    const char *status_str;
    switch (ready_state) {
        case 0:
        case 1:
        case 2:
        case 3:
            status_str = "pending";
            break;
        case 4:  {
            int code = EM_ASM_INT({
                return window.amulet.http_reqs[$0].status;
            }, req->id);
            if (code/100 == 2) {
                status_str = "success";
            } else {
                status_str = "error";
            }
            break;
        }
        default:
            status_str = "error";
    }
    lua_pushstring(L, status_str);
#else
    lua_pushnil(L);
#endif
}

static void get_response(lua_State *L, void *obj) {
#ifdef AM_BACKEND_EMSCRIPTEN
    am_http_request *req = (am_http_request*)obj;
    int text_ptr = EM_ASM_INT({
        var xhr = window.amulet.http_reqs[$0];
        var txt = xhr.responseText;
        if (txt == null) return 0;
        var len = Module.lengthBytesUTF8(txt) + 1;
        var buffer = Module._malloc(len);
        Module.stringToUTF8(txt, buffer, len);
        return buffer;
    }, req->id);
    if (text_ptr == 0) {
        lua_pushnil(L);
    } else {
        char *text = (char*)text_ptr;
        lua_pushstring(L, text);
        free(text);
    }
#else
    lua_pushnil(L);
#endif
}

static void get_code(lua_State *L, void *obj) {
#ifdef AM_BACKEND_EMSCRIPTEN
    am_http_request *req = (am_http_request*)obj;
    int code = EM_ASM_INT({
        return window.amulet.http_reqs[$0].status;
    }, req->id);
    lua_pushinteger(L, code);
#else
    lua_pushnil(L);
#endif
}

static am_property http_req_status_property = {get_status, NULL};
static am_property http_req_response_property = {get_response, NULL};
static am_property http_req_code_property = {get_code, NULL};

static void register_http_req_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);
    lua_pushcclosure(L, http_req_gc, 0);
    lua_setfield(L, -2, "__gc");

    am_register_property(L, "status", &http_req_status_property);
    am_register_property(L, "response", &http_req_response_property);
    am_register_property(L, "code", &http_req_code_property);

    am_register_metatable(L, "http_request", MT_am_http_request, 0);
}

void am_open_http_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"http", http_request},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_http_req_mt(L);
}
