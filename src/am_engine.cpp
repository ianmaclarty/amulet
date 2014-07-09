#include "amulet.h"

am_program_param_name_info *am_param_name_map = NULL;

static void init_reserved_refs(lua_State *L);
static void open_stdlualibs(lua_State *L);

lua_State *am_init_engine(bool worker) {
    lua_State *L = luaL_newstate();
    init_reserved_refs(L);
    open_stdlualibs(L);
    am_open_math_module(L);
    am_open_buffer_module(L);
    if (!worker) {
        am_init_param_name_map(L);
        am_open_window_module(L);
        am_open_node_module(L);
        am_open_program_module(L);
    }
    am_set_globals_metatable(L);
    return L;
}

void am_destroy_engine(lua_State *L) {
    lua_close(L);
}

static void init_reserved_refs(lua_State *L) {
    int i, j;
    do {
        lua_pushboolean(L, 1);
        i = luaL_ref(L, LUA_REGISTRYINDEX);
    } while (i < AM_RESERVED_REFS_START - 1);
    if (i != AM_RESERVED_REFS_START - 1) {
        am_abort("Internal Error: AM_RESERVED_REFS_START too low\n");
    }
    for (i = AM_RESERVED_REFS_START; i < AM_RESERVED_REFS_END; i++) {
        lua_pushboolean(L, 1);
        j = luaL_ref(L, LUA_REGISTRYINDEX);
        if (i != j) {
            am_abort("Internal Error: non-contiguous refs\n");
        }
        j++;
    }
}

static void open_stdlualibs(lua_State *L) {
    am_requiref(L, "base",      luaopen_base);
    am_requiref(L, "package",   luaopen_package);
    am_requiref(L, "math",      luaopen_math);
#ifdef AM_LUAJIT
    // luajit loads the coroutine module into the global
    // table in luaopen_base, so unset the global.
    lua_pushnil(L);
    lua_setglobal(L, "coroutine");
    am_requiref(L, "ffi",       luaopen_ffi);
    am_requiref(L, "jit",       luaopen_jit);
#else
    am_requiref(L, "coroutine", luaopen_coroutine);
#endif
    am_requiref(L, "string",    luaopen_string);
    am_requiref(L, "table",     luaopen_table);
    am_requiref(L, "os",        luaopen_os);
    am_requiref(L, "debug",     luaopen_debug);
}
