#ifdef AM_STEAMWORKS
void am_open_steamworks_module(lua_State *L);
void am_steam_teardown();
void am_steam_step();
const char* am_get_steam_lang();
#endif
