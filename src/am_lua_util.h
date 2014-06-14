void am_set_globals_metatable(lua_State *L);
void am_requiref(lua_State *L, const char *modname, lua_CFunction openf);
void am_open_module(lua_State *L, const char *name, luaL_Reg *funcs);
void am_register_metatable(lua_State *L, int id);
void am_push_metatable(lua_State *L, int metatable_id);
void am_set_metatable(lua_State *L, int metatable_id, int idx);
bool am_has_metatable_id(lua_State *L, int metatable_id, int idx);
void *am_check_metatable_id(lua_State *L, int metatable_id, int idx);

void *am_new_nonatomic_userdata(lua_State *L, size_t sz);
int am_new_ref(lua_State *L, int from, int to);
void am_delete_ref(lua_State *L, int obj, int ref);
void am_push_ref(lua_State *L, int obj, int ref);
void am_replace_ref(lua_State *L, int obj, int ref, int new_val);

#ifdef AM_LUAJIT
void lua_setuservalue(lua_State *L, int idx);
void lua_getuservalue(lua_State *L, int idx);
int lua_rawlen(lua_State *L, int idx);
int lua_absindex(lua_State *L, int idx);
#endif
