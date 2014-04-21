void am_lua_load_module(lua_State *L, const char *name, luaL_Reg *funcs);

void am_lua_register_metatable(lua_State *L, int id);
void am_lua_set_metatable(lua_State *L, int metatable_id, int idx);
int am_lua_has_metatable_id(lua_State *L, int metatable_id, int idx);
void *am_lua_check_metatable_id(lua_State *L, int metatable_id, int idx);
