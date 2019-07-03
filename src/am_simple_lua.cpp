#include "amulet.h"

enum am_simple_lua_value_type {
    AM_SIMPLE_LUA_STRING,
    AM_SIMPLE_LUA_NUMBER,
    AM_SIMPLE_LUA_BOOLEAN,
    AM_SIMPLE_LUA_NIL,
};

struct am_simple_lua_value {
    am_simple_lua_value_type type;
    union {
        const char *string_value;
        double number_value;
        bool boolean_value;
    } value;
};

typedef am_simple_lua_value (*am_simple_lua_func)(int nargs, am_simple_lua_value *args);

#define MAXARGS 20

static int call_custom_func(lua_State *L) {
    int nargs = am_min(MAXARGS, am_check_nargs(L, 0));
    am_simple_lua_func f = (am_simple_lua_func)lua_touserdata(L, lua_upvalueindex(1));
    am_simple_lua_value arg_values[MAXARGS];
    for (int i = 0; i < nargs; ++i) {
        switch (lua_type(L, i+1)) {
            case LUA_TNUMBER: {
                arg_values[i].type = AM_SIMPLE_LUA_NUMBER;
                arg_values[i].value.number_value = lua_tonumber(L, i+1);
                break;
            }
            case LUA_TSTRING: {
                arg_values[i].type = AM_SIMPLE_LUA_STRING;
                arg_values[i].value.string_value = lua_tostring(L, i+1);
                break;
            }
            case LUA_TBOOLEAN: {
                arg_values[i].type = AM_SIMPLE_LUA_BOOLEAN;
                arg_values[i].value.boolean_value = lua_toboolean(L, i+1);
                break;
            }
            default: {
                arg_values[i].type = AM_SIMPLE_LUA_NIL;
                break;
            }
        }
    }
    am_simple_lua_value rv = f(nargs, arg_values);
    switch (rv.type) {
        case AM_SIMPLE_LUA_NUMBER: {
            lua_pushnumber(L, rv.value.number_value);
            break;
        }
        case AM_SIMPLE_LUA_STRING: {
            lua_pushstring(L, rv.value.string_value);
            break;
        }
        case AM_SIMPLE_LUA_BOOLEAN: {
            lua_pushboolean(L, rv.value.boolean_value);
            break;
        }
        case AM_SIMPLE_LUA_NIL: {
            lua_pushnil(L);
            break;
        }
    }
    return 1;
}

void am_simple_lua_register_function(const char *name, am_simple_lua_func func) {
    lua_State *L = am_get_global_lua_state();
    if (L == NULL) return;

    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, AMULET_LUA_MODULE_NAME);

    lua_pushlightuserdata(L, (void*)func);
    lua_pushcclosure(L, call_custom_func, 1);
    lua_setfield(L, -2, name);

    lua_pop(L, 2);
}
