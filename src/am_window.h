void am_open_window_module(lua_State *L);
bool am_update_windows(lua_State *L);
bool am_execute_actions(lua_State *L, double dt);

void am_handle_window_close(am_native_window *window);
void am_destroy_windows(lua_State *L);

extern am_framebuffer_id am_default_framebuffer;
