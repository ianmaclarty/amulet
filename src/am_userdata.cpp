#include "amulet.h"

am_userdata::am_userdata() {
    ud_flags = 0;
}

void am_userdata::push(lua_State *L) {
    lua_unsafe_pushuserdata(L, this);
}

am_nonatomic_userdata::am_nonatomic_userdata() {
    num_refs = -1;
    freelist = 0;
    ud_flags = AM_NONATOMIC_MASK;
}

int am_nonatomic_userdata::ref(lua_State *L, int idx) {
    idx = am_absindex(L, idx);
    pushuservalue(L);
    num_refs++;
    int ref;
    if (freelist == 0) {
        // no free cells, so add to end
        ref = num_refs;
    } else {
        ref = freelist;
        lua_rawgeti(L, -1, freelist);
        freelist = lua_tointeger(L, -1);
        lua_pop(L, 1); // pop freelist value
    }
    lua_pushvalue(L, idx);
    lua_rawseti(L, -2, ref);
    lua_pop(L, 1); // uservalue table
    return ref;
}

void am_nonatomic_userdata::unref(lua_State *L, int ref) {
    assert(ref != LUA_NOREF);
    pushuservalue(L);
    lua_pushinteger(L, freelist);
    lua_rawseti(L, -2, ref);
    freelist = ref;
    lua_pop(L, 1); // uservalue table
}

void am_nonatomic_userdata::reref(lua_State *L, int ref, int idx) {
    assert(ref != LUA_NOREF);
    idx = am_absindex(L, idx);
    pushuservalue(L);
    lua_pushvalue(L, idx);
    lua_rawseti(L, -2, ref);
    lua_pop(L, 1); // uservalue table
}

void am_nonatomic_userdata::pushref(lua_State *L, int ref) {
    assert(ref != LUA_NOREF);
    pushuservalue(L);
    lua_rawgeti(L, -1, ref);
    lua_replace(L, -2); // replace uservalue table with ref value
}

void am_nonatomic_userdata::pushuservalue(lua_State *L) {
    push(L); // push userdata
    if (num_refs == -1) {
        // create uservalue table
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setuservalue(L, -3);
        num_refs = 0;
    } else {
        lua_getuservalue(L, -1);
    }
    lua_remove(L, -2); // remove userdata
}

am_userdata *am_init_userdata(lua_State *L, am_userdata *ud, int metatable_id) {
    ud->ud_flags |= metatable_id;
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    assert(lua_istable(L, -1));
    lua_setmetatable(L, -2);
    return ud;
}

static int mt_parent[AM_RESERVED_REFS_END];

static void ensure_mt_parent_initialized() {
    static bool mt_parent_initialized = false;
    if (!mt_parent_initialized) {
        for (int i = 0; i < AM_RESERVED_REFS_END; i++) {
            mt_parent[i] = 0;
        }
        mt_parent_initialized = true;
    }
}

void am_register_metatable(lua_State *L, const char *tname, int metatable_id, int parent_mt_id) {
    ensure_mt_parent_initialized();

    lua_pushstring(L, tname);
    lua_setfield(L, -2, "tname");

    if (parent_mt_id > 0) {
        mt_parent[metatable_id] = parent_mt_id;
        lua_newtable(L);
        am_push_metatable(L, parent_mt_id);
        if (!lua_istable(L, -1)) {
            am_abort("attempt to register metatable %s before parent", tname);
        }
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_METATABLE_REGISTRY);
    lua_pushstring(L, tname);
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1); // metatable registry

    lua_rawseti(L, LUA_REGISTRYINDEX, metatable_id);
}

void am_push_metatable(lua_State *L, int metatable_id) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
}

int am_get_type(lua_State *L, int idx) {
    int t = lua_type(L, idx);
    if (t == LUA_TUSERDATA) {
        am_userdata *ud = (am_userdata*)lua_touserdata(L, idx);
        uint32_t mtid = ud->ud_flags & AM_METATABLE_ID_MASK;
        assert(mtid < AM_RESERVED_REFS_END && mtid > AM_RESERVED_REFS_START);
        return mtid;
    } else {
        return t;
    }
}

const char* am_get_typename(lua_State *L, int metatable_id) {
    if (metatable_id <= LUA_TTHREAD) {
        return lua_typename(L, metatable_id);
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return "unregistered metatable";
    }
    lua_pushstring(L, "tname");
    lua_rawget(L, -2);
    if (lua_type(L, -1) == LUA_TSTRING) {
        const char *tname = lua_tostring(L, -1);
        lua_pop(L, 2);
        return tname;
    } else {
        lua_pop(L, 2);
        return "missing tname";
    }
}

am_userdata *am_check_metatable_id(lua_State *L, int metatable_id, int idx) {
    int type = lua_type(L, idx);
    am_userdata *ud = NULL;
    if (type == LUA_TUSERDATA) {
        ud = (am_userdata*)lua_touserdata(L, idx);
        if (ud != NULL) {
            int mt = ud->ud_flags & AM_METATABLE_ID_MASK;
            while (mt > 0 && mt < AM_RESERVED_REFS_END) {
                if (mt == metatable_id) return ud;
                mt = mt_parent[mt];
            }
        }
    }

    // fail
    idx = am_absindex(L, idx);
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    int expected_type_mt_idx = am_absindex(L, -1);
    if (!lua_getmetatable(L, idx)) {
        lua_pushnil(L);
    }
    int actual_type_mt_idx = am_absindex(L, -1);
    const char *expected_tname;
    const char *actual_tname;
    int argtype;
    if (lua_istable(L, expected_type_mt_idx)) {
        lua_pushstring(L, "tname");
        lua_rawget(L, expected_type_mt_idx);
        expected_tname = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (expected_tname == NULL) expected_tname = "<missing mt tname entry>";
    } else {
        expected_tname = "<missing mt entry>";
    }
    argtype = lua_type(L, idx);
    if (argtype == LUA_TUSERDATA && lua_istable(L, actual_type_mt_idx)) {
        lua_pushstring(L, "tname");
        lua_rawget(L, actual_type_mt_idx);
        actual_tname = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (actual_tname == NULL) actual_tname = "userdata";
    } else {
        actual_tname = lua_typename(L, argtype);
    }
    lua_pop(L, 2); // actual, expected metatables
    luaL_error(L, "expecting a value of type '%s' at position %d (got '%s')",
        expected_tname, idx, actual_tname);
    return NULL;
}

void am_register_property(lua_State *L, const char *field, const am_property *property) {
    if (property->getter == NULL) {
        am_abort("property getter for '%s' is NULL");
        return;
    }
    lua_pushlightuserdata(L, (void*)property);
    lua_setfield(L, -2, field);
}

int am_default_index_func(lua_State *L) {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
    if (lua_islightuserdata(L, -1)) {
        void *obj = lua_touserdata(L, 1);
        am_property *property = (am_property*)lua_touserdata(L, -1);
        lua_pop(L, 2); // property, metatable
        assert(property->getter != NULL);
        property->getter(L, obj);
        return 1;
    } else {
        lua_remove(L, -2); // remove metatable
        return 1;
    }
}

int am_default_newindex_func(lua_State *L) {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
    if (lua_islightuserdata(L, -1)) {
        void *obj = lua_touserdata(L, 1);
        am_property *property = (am_property*)lua_touserdata(L, -1);
        lua_pop(L, 2); // property, metatable
        if (property->setter != NULL) {
            property->setter(L, obj);
            return 0;
        } else {
            const char *field = lua_tostring(L, 2);
            if (field == NULL) field = "<unknown>";
            return luaL_error(L, "field '%s' is readonly");
        }
    }
    const char *field = lua_tostring(L, 2);
    if (field == NULL) field = "<unknown>";
    return luaL_error(L, "no such field: '%s'", field);
}

void am_set_default_index_func(lua_State *L) {
    lua_pushcclosure(L, am_default_index_func, 0);
    lua_setfield(L, -2, "__index");
}

void am_set_default_newindex_func(lua_State *L) {
    lua_pushcclosure(L, am_default_newindex_func, 0);
    lua_setfield(L, -2, "__newindex");
}

static int getusertable(lua_State *L) {
    am_check_nargs(L, 1);
    if (lua_type(L, 1) != LUA_TUSERDATA) {
        return luaL_error(L, "expecting a userdata argument (in fact a %s)", lua_typename(L, lua_type(L, 1)));
    }
    am_userdata *ud = (am_userdata*)lua_touserdata(L, 1);
    if (!(ud->ud_flags & AM_NONATOMIC_MASK)) {
        lua_pushnil(L);
        return 1;
    }
    am_nonatomic_userdata *naud = (am_nonatomic_userdata*)ud;
    naud->pushuservalue(L);
    return 1;
}

void am_open_userdata_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"_getusertable",    getusertable},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
