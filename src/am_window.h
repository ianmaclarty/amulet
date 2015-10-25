struct am_window : am_nonatomic_userdata {
    bool                needs_closing;
    am_native_window   *native_win;
    am_scene_node      *scene;
    int                 scene_ref;
    int                 window_ref;
    int                 requested_width;
    int                 requested_height;
    int                 pixel_width;
    int                 pixel_height;
    int                 screen_width;  // as above but in "screen units"
    int                 screen_height;
    int                 viewport_x;
    int                 viewport_y;
    int                 viewport_width;
    int                 viewport_height;
    float               user_left;
    float               user_right;
    float               user_bottom;
    float               user_top;
    glm::mat4           projection;
    bool                user_projection; // did the user set the projection?
    bool                has_depth_buffer;
    bool                has_stencil_buffer;
    bool                letterbox;
    glm::vec4           clear_color;
    bool                lock_pointer;
    am_window_mode      mode;
    bool                dirty;

    void mouse_move(lua_State *L, double x, double y);
    void mouse_down(lua_State *L, am_mouse_button button);
    void mouse_up(lua_State *L, am_mouse_button button);
    void mouse_wheel(lua_State *L, double x, double y);
};

void am_open_window_module(lua_State *L);
bool am_update_windows(lua_State *L);
bool am_execute_actions(lua_State *L, double dt);

void am_handle_window_close(am_native_window *window);
void am_destroy_windows(lua_State *L);

am_window* am_find_window(am_native_window *nwin);
