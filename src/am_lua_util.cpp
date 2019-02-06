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
#if defined(AM_LUA52) || defined(AM_LUA53)
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
#endif
    lua_newtable(L);
    lua_pushcfunction(L, global_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, global_index);
    lua_setfield(L, -2, "__index");
#if defined(AM_LUA52) || defined(AM_LUA53)
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
#else
    lua_setmetatable(L, LUA_GLOBALSINDEX);
#endif
}

static void setfuncs(lua_State *L, const luaL_Reg *l) {
    for (; l->name != NULL; l++) {
        lua_pushcfunction(L, l->func);
        lua_setfield(L, -2, l->name);
    }
}

void am_open_module(lua_State *L, const char *name, luaL_Reg *funcs) {
    if (name == NULL) {
        // global namespace
#if defined(AM_LUA52) || defined(AM_LUA53)
        lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
#else
        lua_pushvalue(L, LUA_GLOBALSINDEX);
#endif
        setfuncs(L, funcs);
        lua_pop(L, 1);
    } else {
        lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
        lua_getfield(L, -1, name);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, name);
            lua_pushvalue(L, -1);
            lua_setglobal(L, name);
        }
        setfuncs(L, funcs);
        lua_pop(L, 2);
    }
}

void am_requiref(lua_State *L, const char *modname, lua_CFunction openf) {
#if defined(AM_LUA52) || defined(AM_LUA53)
    luaL_requiref(L, modname, openf, 1);
    lua_pop(L, 1);
#else
    lua_pushcfunction(L, openf);
    lua_pushstring(L, modname);  /* argument to open function */
    lua_call(L, 1, 1);  /* open module */
    luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 16);
    lua_pushvalue(L, -2);  /* make copy of module (call result) */
    lua_setfield(L, -2, modname);  /* _LOADED[modname] = module */
    lua_pop(L, 1); // _LOADED
    lua_setglobal(L, modname); // pops call result
#endif
}

void am_register_enum(lua_State *L, int enum_id, am_enum_value *values) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, enum_id);
    if (!(lua_isboolean(L, -1) && lua_toboolean(L, -1))) {
        am_abort("enum %d already initialized", enum_id);
    }
    lua_pop(L, 1);
    int count = 0;
    am_enum_value *ptr = values;
    while (ptr->str != NULL) {
        ptr++;
        count++;
    }
    lua_createtable(L, count, count);
    for (int i = 0; i < count; i++) {
        lua_pushstring(L, values[i].str);
        lua_pushinteger(L, values[i].val);
        lua_rawset(L, -3);
        lua_pushstring(L, values[i].str);
        lua_rawseti(L, -2, values[i].val+1);
    }
    lua_rawseti(L, LUA_REGISTRYINDEX, enum_id);
}

int am_get_enum_raw(lua_State *L, int enum_id, int idx) {
    idx = am_absindex(L, idx);
    lua_rawgeti(L, LUA_REGISTRYINDEX, enum_id);
    if (!lua_istable(L, -1)) {
        return luaL_error(L, "INTERNAL ERROR: enum %d not initialized");
    }
    lua_pushvalue(L, idx);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1)) {
        const char *name = lua_tostring(L, idx);
        if (name == NULL) {
            name = lua_typename(L, lua_type(L, idx));
        }
        return luaL_error(L, "invalid enum value '%s'", name);
    }
    int val = lua_tointeger(L, -1);
    lua_pop(L, 2);
    return val;
}

void am_push_enum_raw(lua_State *L, int enum_id, int value) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, enum_id);
    if (!lua_istable(L, -1)) {
        luaL_error(L, "INTERNAL ERROR: enum %d not initialized");
        return;
    }
    lua_rawgeti(L, -1, value+1);
    lua_remove(L, -2);
}

int am_check_nargs(lua_State *L, int n) {
    int nargs = lua_gettop(L);
    if (nargs < n) {
        if (n == 1) {
            luaL_error(L, "expecting at least 1 argument");
        } else {
            luaL_error(L, "expecting at least %d arguments", n);
        }
    }
    return nargs;
}

static int check_call_status(lua_State *L, int status) {
    if (status) {
        const char *msg = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (msg == NULL) msg = "unknown error";
        am_log0("%s", msg);
    }
    return status;
}

static int traceback(lua_State *L) {
    am_call_amulet(L, "_traceback", 1, 1);
    return 1;
}

bool am_call(lua_State *L, int nargs, int nresults) {
    int status;
    int base = lua_gettop(L) - nargs;  /* function index */
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_TRACEBACK_FUNC);
    lua_insert(L, base);  /* put it under chunk and args */
    status = lua_pcall(L, nargs, nresults, base);
    lua_remove(L, base);  /* remove traceback function */
    return (check_call_status(L, status) == 0);
}

bool am_call_amulet(lua_State *L, const char *func, int nargs, int nresults) {
    lua_getglobal(L, AMULET_LUA_MODULE_NAME);
    lua_getfield(L, -1, func);
    lua_remove(L, -2); // 'amulet' global
    if (nargs > 0) {
        // move function above args
        lua_insert(L, -nargs - 1);
    }
    return am_call(L, nargs, nresults);
}

static bool load_script(lua_State *L, const char *script, int len, const char *name) {
    char lname[128];
    snprintf(lname, 128, "@%s", name);
    lname[127] = 0;
    int status = luaL_loadbuffer(L, script, len, lname);
    return (check_call_status(L, status) == 0);
}

bool am_run_script(lua_State *L, const char *script, int len, const char *name) {
    if (load_script(L, script, len, name)) {
        lua_newtable(L); // module table - discarded afterwards.
        return am_call(L, 1, 0);
    } else {
        return false;
    }
}

void am_init_traceback_func(lua_State *L) {
    lua_pushcclosure(L, traceback, 0);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_TRACEBACK_FUNC);
}

#define TMP_BUF_SZ 512

int am_require(lua_State *L) {
    am_check_nargs(L, 1);
    char tmpbuf1[TMP_BUF_SZ];
    char tmpbuf2[TMP_BUF_SZ];
    size_t len;
    const char *modname = lua_tolstring(L, 1, &len);

    // check for string argument
    if (modname == NULL) {
        return luaL_error(L, "require expects a string as its single argument");
    }

    // check module name not too long
    if (len > TMP_BUF_SZ - 10) {
        return luaL_error(L, "module name '%s' too long", modname);
    }

    // check if module has already been loaded
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_MODULE_TABLE);
    lua_pushvalue(L, 1); // module name
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1)) {
        // already loaded, or busy loading
        lua_remove(L, -2); // module table
        return 1;
    }
    lua_pop(L, 1); // nil

    // create exports table and add to module table:
    lua_newtable(L);
    lua_pushvalue(L, 1); // module name
    lua_pushvalue(L, -2); // export table
    lua_rawset(L, -4); // set it
    // export table now on top

    // read in the module
    int sz;
    strncpy(tmpbuf1, modname, TMP_BUF_SZ);
    am_replchr(tmpbuf1, '.', '/'); // always use forward slash, even on windows, in case we're looking in data.pak (/ works for normal files on windows anyway)
    snprintf(tmpbuf2, TMP_BUF_SZ, "%s.lua", tmpbuf1);
    char *errmsg;
    void *buf = am_read_resource(tmpbuf2, &sz, &errmsg);
    if (buf == NULL) {
        lua_pushfstring(L, "unable to load module '%s': %s", modname, errmsg);
        free(errmsg);
        return lua_error(L);
    }

    // replace "#!" at start with "--"
    char *cbuf = (char*)buf;
    if (sz >= 2 && cbuf[0] == '#' && cbuf[1] == '!') {
        cbuf[0] = '-';
        cbuf[1] = '-';
    }

    // parse and load the module
    snprintf(tmpbuf2, TMP_BUF_SZ, "@%s.lua", tmpbuf1);
    int res = luaL_loadbuffer(L, (const char*)buf, sz, tmpbuf2);
    free(buf);
    if (res != 0) return lua_error(L);

    // pass export table as arg
    lua_pushvalue(L, -2); // export table
    lua_call(L, 1, 1);

    if (!lua_isnil(L, -1)) {
        // replace export table with returned value in module table
        lua_pushvalue(L, 1); // module name
        lua_pushvalue(L, -2); // returned value
        lua_rawset(L, -5);
        lua_remove(L, -2); // export table
        lua_remove(L, -2); // module table
        // return value now on top
    } else {
        lua_pop(L, 1); // nil
        lua_remove(L, -2); // module table
        // export table now on top
    }
    return 1;
}

void am_setfenv(lua_State *L, int index) {
    assert(lua_isfunction(L, index));
    assert(lua_istable(L, -1));
#if defined(AM_LUAJIT) || defined(AM_LUA51)
    lua_setfenv(L, index);
#else
    const char *name = lua_setupvalue(L, index, 1);
    assert(strcmp(name, "_ENV") == 0);
    AM_UNUSED(name);
#endif
}

#if defined(AM_LUAJIT) || defined(AM_LUA51)

void lua_setuservalue(lua_State *L, int idx) {
    lua_setfenv(L, idx);
}

void lua_getuservalue(lua_State *L, int idx) {
    lua_getfenv(L, idx);
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

lua_Number lua_tonumberx(lua_State *L, int idx, int *isnum) {
    if (lua_isnumber(L, idx)) {
        *isnum = 1;
        return lua_tonumber(L, idx);
    } else {
        *isnum = 0;
        return 0;
    }
}

#else

int luaL_typerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}

#endif 

#ifdef AM_LUAJIT

#define AM_AVOID_MSVC_LJ_ERRORS 1
#include "lj_gc.h"
#include "lj_obj.h"
#include "lj_state.h"
#undef AM_AVOID_MSVC_LJ_ERRORS

void lua_unsafe_pushuserdata(lua_State *L, void *v) {
    assert(v != NULL);
    GCudata *ud = ((GCudata*)v)-1;
    // push nil to advance stack top
    // (we don't have access to incr_top here)
    lua_pushnil(L);
    // replace the pushed nil with the userdata value
    setudataV(L, L->top-1, ud);
}

#else
// standard lua

#include "lapi.h"
#include "lgc.h"
#include "lobject.h"
#ifdef AM_LUA51
#include "lstate.h"
#endif

void lua_unsafe_pushuserdata(lua_State *L, void *v) {
    assert(v != NULL);
    Udata *u = ((Udata*)v)-1;
    lua_lock(L);
    setuvalue(L, L->top, u);
#ifdef AM_LUA51
    L->top++;
#else
    api_incr_top(L);
#endif
    lua_unlock(L);
}

#endif
