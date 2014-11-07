#define AMULET_LUA_MODULE_NAME "amulet"

void am_set_globals_metatable(lua_State *L);
void am_requiref(lua_State *L, const char *modname, lua_CFunction openf);
void am_open_module(lua_State *L, const char *name, luaL_Reg *funcs);

int am_check_nargs(lua_State *L, int n);

void am_init_traceback_func(lua_State *L);
bool am_call(lua_State *L, int nargs, int nresults);
bool am_call_amulet(lua_State *L, const char *func, int nargs, int nresults);

// The caller must ensure v is live and was created
// with lua_newuserdata.
void lua_unsafe_pushuserdata(lua_State *L, void *v);

#ifdef AM_LUAJIT
void lua_setuservalue(lua_State *L, int idx);
void lua_getuservalue(lua_State *L, int idx);
int lua_rawlen(lua_State *L, int idx);
lua_Integer lua_tointegerx(lua_State *L, int idx, int *isnum);
#endif

#define am_absindex(L, idx) ((idx) > 0 ? (idx) : lua_gettop(L) + (idx) + 1)
