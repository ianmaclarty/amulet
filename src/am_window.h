struct am_window : am_nonatomic_userdata {
    bool                needs_closing;
    am_native_window   *native_win;
    am_scene_node      *scene;
    int                 scene_ref;
    int                 window_ref;
    int                 requested_width;
    int                 requested_height;
    int                 curr_width; // actual pixel width
    int                 curr_height; // actual pixel height
    int                 prev_width;
    int                 prev_height;
    bool                has_depth_buffer;
    bool                has_stencil_buffer;
    glm::vec4           clear_color;
    bool                auto_clear;
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
