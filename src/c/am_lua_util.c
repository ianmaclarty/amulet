#include "amulet.h"

void am_lua_load_module(lua_State *L, const char *name, luaL_Reg *funcs) {
    lua_getglobal(L, name);
    if (lua_isnil(L, -1)) {
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, name);
    }
    luaL_setfuncs(L, funcs, 0);
    lua_pop(L, 1);
}

void am_lua_register_metatable(lua_State *L, int id) {
    lua_rawseti(L, LUA_REGISTRYINDEX, id);
}

void am_lua_set_metatable(lua_State *L, int metatable_id, int idx) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    lua_setmetatable(L, idx < 0 ? idx-1 : idx);
}

int am_lua_has_metatable_id(lua_State *L, int metatable_id, int idx) {
    int r;
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    if (lua_getmetatable(L, idx)) {
        r = lua_rawequal(L, -1, -2);
        lua_pop(L, 2);
        return r;
    } else {
        lua_pop(L, 1);
        return 0;
    }
}

void *am_lua_check_metatable_id(lua_State *L, int metatable_id, int idx) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    if (!lua_getmetatable(L, idx)) {
        lua_pushnil(L);
    }
    if (!lua_rawequal(L, -1, -2)) {
        const char *typename;
        const char *argtypename;
        int argtype;
        lua_pushstring(L, "typename");
        lua_rawget(L, -3);
        typename = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (typename == NULL) typename = "userdata";
        argtype = lua_type(L, idx);
        if (argtype == LUA_TUSERDATA && !lua_isnil(L, -1)) {
            lua_pushstring(L, "typename");
            lua_rawget(L, -2);
            argtypename = lua_tostring(L, -1);
            lua_pop(L, 1);
            if (argtypename == NULL) argtypename = "userdata";
        } else {
            argtypename = lua_typename(L, argtype);
        }
        lua_pop(L, 2);
        luaL_error(L, "Expecting a value of type '%s' at position %d (got '%s')", typename, idx, argtypename);
        return NULL;
    }
    lua_pop(L, 2);
    return lua_touserdata(L, idx);
}
