#include "amulet.h"

void am_rand::init(int seed) {
    tinymt32_init(&state, (uint32_t)seed);
}

double am_rand::get_rand() {
    return tinymt32_generate_32double(&state);
}

float am_rand::get_randf() {
    return tinymt32_generate_float(&state);
}

static int create_rand(lua_State *L) {
    am_check_nargs(L, 1);
    int seed = lua_tointeger(L, 1);
    am_rand *rnd = am_new_userdata(L, am_rand);
    rnd->init(seed);
    return 1;
}

static int get_random(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_rand *rnd = am_get_userdata(L, am_rand, 1);
    double r = rnd->get_rand();
    switch (nargs) {
        case 1:
            lua_pushnumber(L, r);
            break;
        case 2: {  /* only upper limit */
            int u = luaL_checkint(L, 2);
            luaL_argcheck(L, 1<=u, 2, "interval is empty");
            lua_pushnumber(L, floor(r*u)+1);  /* int between 1 and `u' */
            break;
        }
        case 3: {  /* lower and upper limits */
            int l = luaL_checkint(L, 2);
            int u = luaL_checkint(L, 3);
            luaL_argcheck(L, l<=u, 3, "interval is empty");
            lua_pushnumber(L, floor(r*(u-l+1))+l);  /* int between `l' and `u' */
            break;
        }
        default:
            return luaL_error(L, "wrong number of arguments");
    }
    return 1;
}

am_rand* am_get_default_rand(lua_State *L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_DEFAULT_RAND);
    am_rand *r = am_get_userdata(L, am_rand, -1);
    lua_pop(L, 1);
    return r;
}

static void register_rand_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L, am_default_index_func, 0);
    lua_setfield(L, -2, "__index");

    lua_pushcclosure(L, get_random, 0);
    lua_setfield(L, -2, "__call");

    am_register_metatable(L, "rand", MT_am_rand, 0);
}

void am_open_rand_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"rand", create_rand},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_rand_mt(L);

    am_rand *default_rand = am_new_userdata(L, am_rand);
    default_rand->init(1);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_DEFAULT_RAND);
}
