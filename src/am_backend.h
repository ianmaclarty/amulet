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
    AM_DISPLAY_ORIENTATION_HYBRID,
};

am_native_window *am_create_native_window(
    am_window_mode mode,
    am_display_orientation orientation,
    int top, int left,
    int width, int height,
    const char *title,
    bool highdpi,
    bool resizable,
    bool borderless,
    bool depth_buffer,
    bool stencil_buffer,
    int msaa_samples);

void am_get_native_window_size(am_native_window *window, int *pw, int *ph, int *sw, int *sh);
void am_get_native_window_safe_area_margin(am_native_window *window, int *left, int *right, int *bottom, int *top);
bool am_set_native_window_size_and_mode(am_native_window *window, int w, int h, am_window_mode mode);
bool am_get_native_window_lock_pointer(am_native_window *window);
void am_set_native_window_lock_pointer(am_native_window *window, bool lock);
bool am_get_native_window_show_cursor(am_native_window *window);
void am_set_native_window_show_cursor(am_native_window *window, bool show);
void am_destroy_native_window(am_native_window *window);

void am_native_window_bind_framebuffer(am_native_window *win);
void am_native_window_swap_buffers(am_native_window *window);

double am_get_current_time();

// returned pointer and errmsg should be freed with free()
void *am_read_resource(const char *filename, int *len, char** errmsg);

int am_next_video_capture_frame();
void am_copy_video_frame_to_texture();

struct am_audio_bus;
void am_capture_audio(am_audio_bus *bus);

char *am_get_base_path();
char *am_get_data_path();

const char *am_preferred_language();

lua_State *am_get_global_lua_state();

#if defined(AM_BACKEND_IOS)
void am_open_ios_module(lua_State *L);
#elif defined(AM_BACKEND_ANDROID)
void am_open_android_module(lua_State *L);
#elif defined(AM_BACKEND_SDL)
void am_open_sdl_module(lua_State *L);
#endif

char *am_open_file_dialog();