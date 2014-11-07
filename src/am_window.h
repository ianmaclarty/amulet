extern bool am_vsync;

void am_open_window_module(lua_State *L);
bool am_update_windows(lua_State *L);

void am_handle_window_close(am_native_window *window);
