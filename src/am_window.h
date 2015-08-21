struct am_window : am_nonatomic_userdata {
    bool                needs_closing;
    am_native_window   *native_win;
    am_scene_node      *root;
    int                 root_ref;
    int                 window_ref;
    int                 width;
    int                 height;
    // used when restoring windowed mode from fullscreen mode
    int                 restore_windowed_width;
    int                 restore_windowed_height;
    bool                has_depth_buffer;
    bool                has_stencil_buffer;
    glm::vec4           clear_color;
    bool                lock_pointer;
    am_window_mode      mode;
    bool                dirty;
};

void am_open_window_module(lua_State *L);
bool am_update_windows(lua_State *L);
bool am_execute_actions(lua_State *L, double dt);

void am_handle_window_close(am_native_window *window);
void am_destroy_windows(lua_State *L);

am_window* am_find_window(am_native_window *nwin);
