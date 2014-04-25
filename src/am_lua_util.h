void am_no_new_globals(lua_State *L);
void am_requiref(lua_State *L, const char *modname, lua_CFunction openf);
void am_open_module(lua_State *L, const char *name, luaL_Reg *funcs);
void am_register_metatable(lua_State *L, int id);
void am_push_metatable(lua_State *L, int metatable_id);
void am_set_metatable(lua_State *L, int metatable_id, int idx);
int am_has_metatable_id(lua_State *L, int metatable_id, int idx);
void *am_check_metatable_id(lua_State *L, int metatable_id, int idx);

#ifdef AM_LUAJIT
void lua_setuservalue(lua_State *L, int idx);
int lua_rawlen(lua_State *L, int idx);
#endif
