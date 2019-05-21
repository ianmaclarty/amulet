struct am_window : am_nonatomic_userdata {
    bool                needs_closing;
    am_native_window   *native_win;
    am_scene_node      *scene;
    int                 scene_ref;
    am_scene_node      *overlay;
    int                 overlay_ref;
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
    double              user_width;
    double              user_height;
    double              user_left;
    double              user_right;
    double              user_bottom;
    double              user_top;
    double              user_safe_left;
    double              user_safe_right;
    double              user_safe_bottom;
    double              user_safe_top;
    glm::dmat4          projection;
    bool                user_projection; // did the user set the projection?
    bool                has_depth_buffer;
    bool                has_stencil_buffer;
    bool                letterbox;
    glm::dvec4          clear_color;
    int                 stencil_clear_value;
    am_window_mode      mode;
    bool                dirty;

    void compute_position(double x, double y, double *usr_x, double *usr_y, double *norm_x, double *norm_y, double *px_x, double *px_y);
    void mouse_move(lua_State *L, double x, double y);
    void mouse_down(lua_State *L, am_mouse_button button);
    void mouse_up(lua_State *L, am_mouse_button button);
    void mouse_wheel(lua_State *L, double x, double y);
    void touch_begin(lua_State *L, void* touchid, double x, double y, double force);
    void touch_end(lua_State *L, void* touchid, double x, double y, double force);
    void touch_move(lua_State *L, void* touchid, double x, double y, double force);
};

void am_open_window_module(lua_State *L);
bool am_update_windows(lua_State *L);
bool am_execute_actions(lua_State *L, double dt);

void am_handle_window_close(am_native_window *window);
void am_destroy_windows(lua_State *L);

am_window* am_find_window(am_native_window *nwin);
