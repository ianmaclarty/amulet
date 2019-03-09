int am_mathv_add(lua_State *L);
int am_mathv_sub(lua_State *L);
int am_mathv_mul(lua_State *L);
int am_mathv_div(lua_State *L);
int am_mathv_mod(lua_State *L);
int am_mathv_pow(lua_State *L);
int am_mathv_unm(lua_State *L);

void am_register_mathv_view_methods(lua_State *L);
void am_open_mathv_module(lua_State *L);
