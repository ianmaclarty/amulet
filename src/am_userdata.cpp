#include "amulet.h"

am_nonatomic_userdata::am_nonatomic_userdata() {
    num_refs = -1;
    freelist = 0;
}

void am_nonatomic_userdata::push(lua_State *L) {
    lua_unsafe_pushuserdata(L, this);
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
    if (ref == LUA_NOREF) {
        lua_pushnil(L);
    } else {
        pushuservalue(L);
        lua_rawgeti(L, -1, ref);
        lua_replace(L, -2); // replace uservalue table with ref value
    }
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

void *am_set_metatable(lua_State *L, void *ud, int metatable_id) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    assert(lua_istable(L, -1));
    lua_setmetatable(L, -2);
    return ud;
}

static int mt_parent[AM_RESERVED_REFS_END];
static bool mt_parent_initialized = false;

static void ensure_mt_parent_initialized() {
    if (!mt_parent_initialized) {
        for (int i = 0; i < AM_RESERVED_REFS_END; i++) {
            mt_parent[i] = 0;
        }
        mt_parent_initialized = true;
    }
}

void am_register_metatable(lua_State *L, const char *tname, int metatable_id, int parent_mt_id) {
    ensure_mt_parent_initialized();
    int mt_idx = am_absindex(L, -1);

    lua_pushinteger(L, metatable_id);
    lua_rawseti(L, mt_idx, AM_METATABLE_ID_INDEX);

    lua_pushstring(L, tname);
    lua_setfield(L, mt_idx, "tname");

    if (parent_mt_id > 0) {
        mt_parent[metatable_id] = parent_mt_id;
        am_push_metatable(L, parent_mt_id);
        if (!lua_istable(L, -1)) {
            am_abort("attempt to register metatable %s before parent", tname);
        }
        int parent_idx = am_absindex(L, -1);
        lua_pushstring(L, "_parent_mt");
        lua_pushvalue(L, parent_idx);
        lua_rawset(L, mt_idx);
        lua_pushnil(L);
        while (lua_next(L, parent_idx)) {
            int key_idx = am_absindex(L, -2);
            int val_idx = am_absindex(L, -1);
            lua_pushvalue(L, key_idx);
            lua_rawget(L, mt_idx);
            if (lua_isnil(L, -1)) {
                lua_pushvalue(L, key_idx);
                lua_pushvalue(L, val_idx);
                lua_rawset(L, mt_idx);
            }
            lua_pop(L, 2);
        }
        lua_pop(L, 1); // parent
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
        if (lua_getmetatable(L, idx)) {
            lua_rawgeti(L, -1, AM_METATABLE_ID_INDEX);
            int mt = lua_tointeger(L, -1);
            lua_pop(L, 2); // mt, metatable
            if (mt != 0) {
                assert(mt < AM_RESERVED_REFS_END && mt > AM_RESERVED_REFS_START);
                return mt;
            } else {
                return t;
            }
        } else {
            return t;
        }
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

void *am_check_metatable_id_no_err(lua_State *L, int metatable_id, int idx) {
    if (lua_getmetatable(L, idx)) {
        lua_rawgeti(L, -1, AM_METATABLE_ID_INDEX);
        int mt = lua_tointeger(L, -1);
        lua_pop(L, 2); // mt, metatable
        if (mt != 0) {
            while (mt > AM_RESERVED_REFS_START && mt < AM_RESERVED_REFS_END) {
                if (mt == metatable_id) return lua_touserdata(L, idx);
                mt = mt_parent[mt];
            }
        }
    }
    return NULL;
}

void *am_check_metatable_id(lua_State *L, int metatable_id, int idx) {
    void *ud = am_check_metatable_id_no_err(L, metatable_id, idx);
    if (ud != NULL) return ud;

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
        am_abort("property getter for '%s' is NULL", field);
        return;
    }
    lua_pushlightuserdata(L, (void*)property);
    lua_setfield(L, -2, field);
}

static bool try_getter(lua_State *L) {
    if (lua_islightuserdata(L, -1)) {
        am_property *property = (am_property*)lua_touserdata(L, -1);
        if (property != NULL) {
            void *obj = lua_touserdata(L, 1);
            lua_pop(L, 2); // property, metatable
            assert(property->getter != NULL);
            property->getter(L, obj);
        } else {
            // custom lua getter
            lua_pop(L, 1); // property (NULL)
            lua_pushlightuserdata(L, (void*)lua_tostring(L, 2));
            lua_rawget(L, -2);
            lua_remove(L, -2); // remove metatable
            if (lua_isfunction(L, -1)) {
                lua_pushvalue(L, 1);
                lua_call(L, 1, 1);
            }
        }
        return true;
    } 
    return false;
}

int am_default_index_func(lua_State *L) {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (try_getter(L)) {
        return 1;
    } else if (lua_isnil(L, -1)) {
        lua_pop(L, 2); // nil, metatable
        lua_getuservalue(L, 1);
        if (!lua_istable(L, -1)) { // lua5.2 initializes uservalues to nil
            lua_pop(L, 1); // uservalue
            lua_pushnil(L);
            return 1;
        }
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (try_getter(L)) {
            return 1;
        } else {
            lua_remove(L, -2); // uservalue table
            return 1;
        }
    } else {
        lua_remove(L, -2); // remove metatable
        return 1;
    }
}

static bool try_setter(lua_State *L) {
    if (lua_islightuserdata(L, -1)) {
        am_property *property = (am_property*)lua_touserdata(L, -1);
        if (property != NULL) {
            void *obj = lua_touserdata(L, 1);
            lua_pop(L, 2); // property, metatable
            if (property->setter != NULL) {
                property->setter(L, obj);
                return true;
            } else {
                const char *field = lua_tostring(L, 2);
                if (field == NULL) field = "<unknown>";
                luaL_error(L, "field '%s' is readonly");
                return false;
            }
        } else {
            // custom lua setter
            lua_pop(L, 1); // property (NULL)
            lua_pushlightuserdata(L, (void*)(lua_tostring(L, 2) + 1));
            lua_rawget(L, -2);
            lua_remove(L, -2); // remove metatable
            if (lua_isfunction(L, -1)) {
                lua_pushvalue(L, 1);
                lua_pushvalue(L, 3);
                lua_call(L, 2, 0);
                return true;
            } else {
                const char *field = lua_tostring(L, 2);
                if (field == NULL) field = "<unknown>";
                luaL_error(L, "field '%s' is readonly", field);
                return false;
            }
        }
    }
    return false;
}

static void set_custom_getter_or_setter(lua_State *L) {
    if (!lua_isfunction(L, 3)) return;
    size_t len;
    const char *field_name = lua_tolstring(L, 2, &len);
    if (field_name == NULL) return;
    if (len < 5) return;
    if (field_name[0] == 'g' &&
        field_name[1] == 'e' &&
        field_name[2] == 't' &&
        field_name[3] == '_')
    {
        const char *prop_name = field_name + 4;
        lua_pushstring(L, prop_name);
        prop_name = lua_tostring(L, -1);
        lua_pushlightuserdata(L, NULL);
        lua_rawset(L, -3);
        lua_pushlightuserdata(L, (void*)prop_name);
        lua_pushvalue(L, 3);
        lua_rawset(L, -3);
    } else if (
        field_name[0] == 's' &&
        field_name[1] == 'e' &&
        field_name[2] == 't' &&
        field_name[3] == '_')
    {
        const char *prop_name = field_name + 4;
        lua_pushstring(L, prop_name);
        prop_name = lua_tostring(L, -1);
        lua_pushlightuserdata(L, NULL);
        lua_rawset(L, -3);
        lua_pushlightuserdata(L, (void*)(prop_name + 1));
        lua_pushvalue(L, 3);
        lua_rawset(L, -3);
    }
}

int am_default_newindex_func(lua_State *L) {
    if (!lua_isstring(L, 2)) goto fail;
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (try_setter(L)) {
        lua_pop(L, 2); // field val, metatable
        return 0;
    } else if (lua_isnil(L, -1)) {
        lua_pop(L, 2); // nil, metatable
        // this should be ok, because am_default_newindex_func should only
        // be on am_nonatomic_userdata values:
        am_nonatomic_userdata *ud = (am_nonatomic_userdata*)lua_touserdata(L, 1);
        ud->pushuservalue(L);
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (try_setter(L)) {
            lua_pop(L, 2); // field val, uservalue
            return 0;
        } else {
            // set in uservalue
            lua_pop(L, 1); // existing field val
            // maybe set custom getter/setter
            set_custom_getter_or_setter(L);
            // set field in uservalue table
            lua_pushvalue(L, 2); // field
            lua_pushvalue(L, 3); // value
            lua_rawset(L, -3); // set in uservalue table
            lua_pop(L, 1); // uservalue
            return true;
        }
    }
    // key exists in metatable, and is not a property
    lua_pop(L, 2); // field value, metatable
fail:
    const char *field = lua_tostring(L, 2);
    if (field == NULL) field = "<unknown>";
    return luaL_error(L, "attempt set field '%s' to value of type %s", field, am_get_typename(L, am_get_type(L, 3)));
}

void am_set_default_index_func(lua_State *L) {
    lua_pushcclosure(L, am_default_index_func, 0);
    lua_setfield(L, -2, "__index");
}

void am_set_default_newindex_func(lua_State *L) {
    lua_pushcclosure(L, am_default_newindex_func, 0);
    lua_setfield(L, -2, "__newindex");
}

static int getter_key(lua_State *L) {
    am_check_nargs(L, 1);
    lua_pushlightuserdata(L, (void*)lua_tostring(L, 1));
    return 1;
}

static int setter_key(lua_State *L) {
    am_check_nargs(L, 1);
    lua_pushlightuserdata(L, (void*)(lua_tostring(L, 1) + 1));
    return 1;
}

static int custom_property_marker(lua_State *L) {
    lua_pushlightuserdata(L, NULL);
    return 1;
}

static int setmetatable(lua_State *L) {
    am_check_nargs(L, 2);
    lua_pushvalue(L, 2);
    lua_setmetatable(L, 1);
    return 0;
}

void am_open_userdata_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"_getter_key",             getter_key},
        {"_setter_key",             setter_key},
        {"_custom_property_marker", custom_property_marker},
        {"_setmetatable",           setmetatable},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
