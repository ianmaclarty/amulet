#include "amulet.h"

am_userdata::am_userdata() {
    metatable_id = 0;
}

void am_userdata::push(lua_State *L) {
    lua_unsafe_pushuserdata(L, this);
}

am_nonatomic_userdata::am_nonatomic_userdata() {
    num_refs = -1;
    freelist = 0;
}

int am_nonatomic_userdata::ref(lua_State *L, int idx) {
    idx = am_absindex(L, idx);
    push(L);
    if (num_refs == -1) {
        // create uservalue table
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setuservalue(L, -3);
        num_refs = 0;
    } else {
        lua_getuservalue(L, -1);
    }
    // uservalue table now at top
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
    lua_pop(L, 2); // userdata, uservalue table
    return ref;
}

void am_nonatomic_userdata::unref(lua_State *L, int ref) {
    assert(ref != LUA_NOREF);
    push(L);
    lua_getuservalue(L, -1);
    lua_pushinteger(L, freelist);
    lua_rawseti(L, -2, ref);
    freelist = ref;
    lua_pop(L, 2); // userdata, uservalue table
}

void am_nonatomic_userdata::reref(lua_State *L, int ref, int idx) {
    assert(ref != LUA_NOREF);
    idx = am_absindex(L, idx);
    push(L);
    lua_getuservalue(L, -1);
    lua_pushvalue(L, idx);
    lua_rawseti(L, -2, ref);
    lua_pop(L, 2); // userdata, uservalue table
}

void am_nonatomic_userdata::pushref(lua_State *L, int ref) {
    assert(ref != LUA_NOREF);
    push(L);
    lua_getuservalue(L, -1);
    lua_rawgeti(L, -1, ref);
    lua_replace(L, -3); // replace userdata value with ref value
    lua_pop(L, 1); // pop uservalue table
}

am_userdata *am_init_userdata(lua_State *L, am_userdata *ud, int metatable_id) {
    ud->metatable_id = metatable_id;
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
    assert(lua_istable(L, -1));
    lua_setmetatable(L, -2);
    return ud;
}

static bool mt_parent_initialized = false;
static int mt_parent[AM_RESERVED_REFS_END];

void am_register_metatable(lua_State *L, int metatable_id, int parent_mt_id) {
    if (!mt_parent_initialized) {
        for (int i = 0; i < AM_RESERVED_REFS_END; i++) {
            mt_parent[i] = 0;
        }
        mt_parent_initialized = true;
    }
    if (parent_mt_id > 0) {
        am_push_metatable(L, parent_mt_id);
        lua_setmetatable(L, -2);
        mt_parent[metatable_id] = parent_mt_id;
    }
    lua_rawseti(L, LUA_REGISTRYINDEX, metatable_id);
}

void am_push_metatable(lua_State *L, int metatable_id) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, metatable_id);
}

int am_get_metatable_id(lua_State *L, int idx) {
    am_userdata *ud = (am_userdata*)lua_touserdata(L, idx);
    if (ud != NULL) {
        return ud->metatable_id;
    } else {
        return 0;
    }
}

am_userdata *am_check_metatable_id(lua_State *L, int metatable_id, int idx) {
    int type = lua_type(L, idx);
    am_userdata *ud = NULL;
    if (type == LUA_TUSERDATA) {
        ud = (am_userdata*)lua_touserdata(L, idx);
        if (ud != NULL) {
            int mt = ud->metatable_id;
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
    luaL_error(L, "expecting a value of type '%s' at position %d (got '%s') [%p]",
        expected_tname, idx, actual_tname, ud);
    return NULL;
}

void am_register_property(lua_State *L, const char *field, const am_property *property) {
    lua_pushlightuserdata(L, (void*)property);
    lua_setfield(L, -2, field);
}

static int default_index_func(lua_State *L) {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (lua_islightuserdata(L, -1)) {
        void *obj = lua_touserdata(L, 1);
        am_property *property = (am_property*)lua_touserdata(L, -1);
        lua_pop(L, 2); // property, metatable
        if (property->getter != NULL) {
            property->getter(L, obj);
            return 1;
        } else {
            lua_pushnil(L);
            return 1;
        }
    } else {
        lua_remove(L, -2); // remove metatable
        return 1;
    }
}

static int default_newindex_func(lua_State *L) {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
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
    return luaL_error(L, "no such field: '%s'");
}

void am_set_default_index_func(lua_State *L) {
    lua_pushcclosure(L, default_index_func, 0);
    lua_setfield(L, -2, "__index");
}

void am_set_default_newindex_func(lua_State *L) {
    lua_pushcclosure(L, default_newindex_func, 0);
    lua_setfield(L, -2, "__newindex");
}
