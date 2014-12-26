void am_open_window_module(lua_State *L);
bool am_update_windows(lua_State *L);

void am_handle_window_close(am_native_window *window);
void am_destroy_windows(lua_State *L);
void am_handle_window_resize(am_native_window *window, int w, int h);
