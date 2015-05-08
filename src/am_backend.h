typedef void* am_native_window;

enum am_window_mode {
    AM_WINDOW_MODE_WINDOWED,
    AM_WINDOW_MODE_FULLSCREEN,
    AM_WINDOW_MODE_FULLSCREEN_DESKTOP,
};

enum am_display_orientation {
    AM_DISPLAY_ORIENTATION_PORTRAIT,
    AM_DISPLAY_ORIENTATION_LANDSCAPE,
    AM_DISPLAY_ORIENTATION_ANY,
};

am_native_window *am_create_native_window(
    am_window_mode mode,
    am_display_orientation orientation,
    int top, int left,
    int width, int height,
    const char *title,
    bool resizable,
    bool borderless,
    bool depth_buffer,
    bool stencil_buffer,
    int msaa_samples,
    am_framebuffer_id *framebuffer);

void am_get_native_window_size(am_native_window *window, int *w, int *h);
bool am_set_native_window_size_and_mode(am_native_window *window, int w, int h, am_window_mode mode);
void am_set_native_window_lock_pointer(am_native_window *window, bool lock);
void am_destroy_native_window(am_native_window *window);

void am_native_window_pre_render(am_native_window *window);
void am_native_window_post_render(am_native_window *window);

double am_get_current_time();

// returned pointer and errmsg should be freed with free()
void *am_read_resource(const char *filename, int *len, char** errmsg);
