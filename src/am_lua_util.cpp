#include "amulet.h"

static int global_newindex(lua_State *L) {
    const char *field = lua_tostring(L, 2);
    if (field == NULL) {
        return luaL_error(L, "attempt to set global");
    } else {
        return luaL_error(L, "attempt to set global '%s'", field);
    }
}

static int global_index(lua_State *L) {
    const char *field = lua_tostring(L, 2);
    if (field == NULL) {
        return luaL_error(L, "attempt to reference missing global");
    } else {
        return luaL_error(L, "attempt to reference missing global '%s'", field);
    }
}

void am_set_globals_metatable(lua_State *L) {
#ifndef AM_LUAJIT
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
#endif
    lua_newtable(L);
    lua_pushcfunction(L, global_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, global_index);
    lua_setfield(L, -2, "__index");
#ifdef AM_LUAJIT
    lua_setmetatable(L, LUA_GLOBALSINDEX);
#else
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
#endif
}

static void setfuncs(lua_State *L, const luaL_Reg *l) {
    for (; l->name != NULL; l++) {
        lua_pushcfunction(L, l->func);
        lua_setfield(L, -2, l->name);
    }
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
    setfuncs(L, funcs);
    lua_pop(L, 2);
}

void am_requiref(lua_State *L, const char *modname, lua_CFunction openf) {
#ifdef AM_LUAJIT
    lua_pushcfunction(L, openf);
    lua_pushstring(L, modname);  /* argument to open function */
    lua_call(L, 1, 1);  /* open module */
    luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 16);
    lua_pushvalue(L, -2);  /* make copy of module (call result) */
    lua_setfield(L, -2, modname);  /* _LOADED[modname] = module */
    lua_pop(L, 2);  /* remove _LOADED table and call result */
#else
    luaL_requiref(L, modname, openf, 0);
    lua_pop(L, 1);
#endif
}

#define AM_METATABLE_ID_INDEX 1

void am_register_metatable(lua_State *L, int metatable_id, int parent_mt_id) {
    if (parent_mt_id > 0) {
        am_push_metatable(L, parent_mt_id);
        lua_setmetatable(L, -2);
    }
    lua_pushinteger(L, metatable_id);
    lua_rawseti(L, -2, AM_METATABLE_ID_INDEX);
    lua_rawseti(L, LUA_REGISTRYINDEX, metatable_id);
}

void am_set_metatable(lua_State *L, int metatable_id, int idx) {
    idx = lua_absindex(L, idx);
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    lua_setmetatable(L, idx);
}

void am_push_metatable(lua_State *L, int metatable_id) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
}

inline int am_get_metatable_id(lua_State *L, int idx) {
    int id = 0;
    if (lua_getmetatable(L, idx)) {
        lua_rawgeti(L, -1, AM_METATABLE_ID_INDEX);
        id = lua_tointeger(L, -1);
        lua_pop(L, 2);
    }
    return id;
}

void *am_check_metatable_id(lua_State *L, int metatable_id, int idx) {
    int top = lua_gettop(L);
    int v = idx;
    while (lua_getmetatable(L, v)) {
        lua_rawgeti(L, -1, AM_METATABLE_ID_INDEX);
        if (lua_tointeger(L, -1) == metatable_id) {
            lua_pop(L, lua_gettop(L) - top);
            return lua_touserdata(L, idx);
        }
        lua_pop(L, 1);
        v = -1;
    }

    // fail
    lua_pop(L, lua_gettop(L) - top);
    idx = lua_absindex(L, idx);
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    if (!lua_getmetatable(L, idx)) {
        lua_pushnil(L);
    }
    const char *expected_tname;
    const char *got_tname;
    int argtype;
    if (lua_istable(L, -2)) {
        lua_pushstring(L, "tname");
        lua_rawget(L, -3);
        expected_tname = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (expected_tname == NULL) expected_tname = "<missing mt tname entry>";
    } else {
        expected_tname = "<missing mt entry>";
    }
    argtype = lua_type(L, idx);
    if (argtype == LUA_TUSERDATA && !lua_isnil(L, -1)) {
        lua_pushstring(L, "tname");
        lua_rawget(L, -1);
        got_tname = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (got_tname == NULL) got_tname = "userdata";
    } else {
        got_tname = lua_typename(L, argtype);
    }
    lua_pop(L, 2);
    luaL_error(L, "expecting a value of type '%s' at position %d (got '%s')",
        expected_tname, idx, got_tname);
    return NULL;
}

void *am_new_nonatomic_userdata(lua_State *L, size_t sz) {
    void *ud = lua_newuserdata(L, sz);
    lua_newtable(L);
    lua_setuservalue(L, -2);
    return ud;
}

int am_new_ref(lua_State *L, int from, int to) {
    lua_getuservalue(L, from);
    lua_pushvalue(L, to);
    int ref = luaL_ref(L, -2);
    lua_pop(L, 1);
    return ref;
}

void am_delete_ref(lua_State *L, int obj, int ref) {
    lua_getuservalue(L, obj);
    luaL_unref(L, -2, ref);
    lua_pop(L, 1);
}

void am_push_ref(lua_State *L, int obj, int ref) {
    lua_getuservalue(L, obj);
    lua_rawgeti(L, -1, ref);
    lua_remove(L, -2); // uservalue
}

void am_replace_ref(lua_State *L, int obj, int ref, int new_val) {
    new_val = lua_absindex(L, new_val);
    lua_getuservalue(L, obj);
    lua_pushvalue(L, new_val);
    lua_rawseti(L, -2, ref);
    lua_pop(L, 1); // uservalue
}

int am_check_nargs(lua_State *L, int n) {
    int nargs = lua_gettop(L);
    if (nargs < n) {
        luaL_error(L, "expecting at least %d arguments", n);
    }
    return nargs;
}

#ifdef AM_LUAJIT
void lua_setuservalue(lua_State *L, int idx) {
    lua_setfenv(L, idx);
}

void lua_getuservalue(lua_State *L, int idx) {
    lua_getfenv(L, idx);
}

int lua_rawlen(lua_State *L, int idx) {
    return lua_objlen(L, idx);
}

int lua_absindex(lua_State *L, int idx) {
    return idx > 0 ? idx : lua_gettop(L) + idx + 1;
}

lua_Integer lua_tointegerx(lua_State *L, int idx, int *isnum) {
    if (lua_isnumber(L, idx)) {
        *isnum = 1;
        return lua_tointeger(L, idx);
    } else {
        *isnum = 0;
        return 0;
    }
}
#endif
