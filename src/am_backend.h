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
    bool highdpi,
    bool resizable,
    bool borderless,
    bool depth_buffer,
    bool stencil_buffer,
    int msaa_samples);

void am_get_native_window_size(am_native_window *window, int *pw, int *ph, int *sw, int *sh);
bool am_set_native_window_size_and_mode(am_native_window *window, int w, int h, am_window_mode mode);
void am_set_native_window_lock_pointer(am_native_window *window, bool lock);
void am_destroy_native_window(am_native_window *window);

void am_native_window_bind_framebuffer(am_native_window *win);
void am_native_window_swap_buffers(am_native_window *window);

double am_get_current_time();

// returned pointer and errmsg should be freed with free()
void *am_read_resource(const char *filename, int *len, char** errmsg);

int am_next_video_capture_frame();
void am_copy_video_frame_to_texture();

void am_capture_audio(am_audio_bus *bus);

char *am_get_base_path();
char *am_get_data_path();
