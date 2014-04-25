#include "amulet.h"

static int new_global_error(lua_State *L) {
    const char *field = lua_tostring(L, 2);
    if (field == NULL) {
        return luaL_error(L, "attempt to set global");
    } else {
        return luaL_error(L, "attempt to set global '%s'", field);
    }
}

void am_no_new_globals(lua_State *L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_newtable(L);
    lua_pushcfunction(L, new_global_error);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
}

void am_open_module(lua_State *L, const char *name, luaL_Reg *funcs) {
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, name);
    }
    luaL_setfuncs(L, funcs, 0);
    lua_pop(L, 2);
}

void am_register_metatable(lua_State *L, int id) {
    lua_rawseti(L, LUA_REGISTRYINDEX, id);
}

void am_set_metatable(lua_State *L, int metatable_id, int idx) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    lua_setmetatable(L, idx < 0 ? idx-1 : idx);
}

void am_push_metatable(lua_State *L, int metatable_id) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
}

int am_has_metatable_id(lua_State *L, int metatable_id, int idx) {
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

void *am_check_metatable_id(lua_State *L, int metatable_id, int idx) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    if (!lua_getmetatable(L, idx)) {
        lua_pushnil(L);
    }
    if (!lua_rawequal(L, -1, -2)) {
        const char *tname;
        const char *argtname;
        int argtype;
        lua_pushstring(L, "tname");
        lua_rawget(L, -3);
        tname = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (tname == NULL) tname = "userdata";
        argtype = lua_type(L, idx);
        if (argtype == LUA_TUSERDATA && !lua_isnil(L, -1)) {
            lua_pushstring(L, "tname");
            lua_rawget(L, -2);
            argtname = lua_tostring(L, -1);
            lua_pop(L, 1);
            if (argtname == NULL) argtname = "userdata";
        } else {
            argtname = lua_typename(L, argtype);
        }
        lua_pop(L, 2);
        luaL_error(L, "expecting a value of type '%s' at position %d (got '%s')", tname, idx, argtname);
        return NULL;
    }
    lua_pop(L, 2);
    return lua_touserdata(L, idx);
}
