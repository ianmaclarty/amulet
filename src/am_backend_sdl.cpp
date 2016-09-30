#include "amulet.h"

#ifdef AM_BACKEND_SDL

#define SDL_MAIN_HANDLED 1
#include "SDL.h"

#ifdef AM_OSX
#import <AppKit/AppKit.h>
#endif

// Create variables for OpenGL ES 2 functions
#ifdef AM_NEED_GL_FUNC_PTRS
#if defined(AM_GLPROFILE_ES)
#include <SDL_opengles2.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#endif
#define AM_GLPROC(ret,func,params) \
    typedef ret (APIENTRY *func##_ptr_t) params; \
    extern ret (APIENTRY *func##_ptr) params;
#include "am_glfuncs.h"
#undef AM_GLPROC
static void init_gl_func_ptrs();
#endif

struct win_info {
    SDL_Window *window;
    bool lock_pointer;
    int mouse_x;
    int mouse_y;
    int mouse_delta_x;
    int mouse_delta_y;
    int mouse_wheel_x;
    int mouse_wheel_y;
};

static bool have_focus = true;
static SDL_AudioDeviceID audio_device = 0;
static SDL_AudioDeviceID capture_device = 0;
static float *audio_buffer = NULL;
static float *capture_buffer = NULL;
static int capture_write_offset = 0;
static int capture_read_offset = 0;
static int capture_ring_size;
static bool capture_initialized = false;
static bool should_init_capture = false;
static std::vector<win_info> windows;
SDL_Window *main_window = NULL;
static SDL_GLContext gl_context;
static bool gl_context_initialized = false;
static bool sdl_initialized = false;
static bool restart_triggered = false;

static am_package *package = NULL;

static void init_sdl();
static void init_audio();
static void init_audio_capture();
static bool handle_events(lua_State *L);
static am_key convert_key(int key);
static am_mouse_button convert_mouse_button(Uint8 button);
static bool check_for_package();
static win_info *win_from_id(Uint32 winid);

static am_controller_button convert_controller_button(Uint8 button);
static am_controller_axis convert_controller_axis(Uint8 axis);
static bool controller_present_at_start = false;
static void load_controller_mappings();
static void init_gamepad();

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
    int msaa_samples)
{
    if (windows.size() == 0 && main_window != NULL) {
        win_info winfo;
        memset(&winfo, 0, sizeof(win_info));
        winfo.window = main_window;
        SDL_GetMouseState(&winfo.mouse_x, &winfo.mouse_y);
        windows.push_back(winfo);
        return (am_native_window*)main_window;
    }
    if (!sdl_initialized) {
        init_sdl();
    }
    if (main_window != NULL) {
        SDL_GL_MakeCurrent(main_window, gl_context);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    }
#if defined(AM_GLPROFILE_ES)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#elif defined(AM_GLPROFILE_DESKTOP)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#else
#error unknown gl profile
#endif
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    if (depth_buffer) {
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    } else {
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    }
    if (stencil_buffer) {
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    } else {
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    }
    if (msaa_samples > 0) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa_samples);
    } else {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    }
    Uint32 flags = SDL_WINDOW_OPENGL;
    switch (mode) {
        case AM_WINDOW_MODE_WINDOWED: break;
        case AM_WINDOW_MODE_FULLSCREEN: flags |= SDL_WINDOW_FULLSCREEN; break;
        case AM_WINDOW_MODE_FULLSCREEN_DESKTOP: flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; break;
    }
    if (borderless) flags |= SDL_WINDOW_BORDERLESS;
    if (highdpi) flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    if (resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (left == -1) left = SDL_WINDOWPOS_CENTERED;
    else if (left < -1) left = SDL_WINDOWPOS_UNDEFINED;
    if (top == -1) top = SDL_WINDOWPOS_CENTERED;
    else if (top < -1) top = SDL_WINDOWPOS_UNDEFINED;
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;
    SDL_Window *win = SDL_CreateWindow(
        title,
        left, top,
        width, height,
        flags
    );
    if (win == NULL) {
        am_log0("%s", SDL_GetError());
        return NULL;
    }
    if (!gl_context_initialized) {
        gl_context = SDL_GL_CreateContext(win);
#ifdef AM_NEED_GL_FUNC_PTRS
        init_gl_func_ptrs();
#endif
        gl_context_initialized = true;
    }
    SDL_GL_MakeCurrent(win, gl_context);
    if (!am_gl_is_initialized()) {
        am_init_gl();
    }
    if (main_window == NULL) {
        main_window = win;
    }
    win_info winfo;
    winfo.window = win;
    winfo.lock_pointer = false;
    SDL_GetMouseState(&winfo.mouse_x, &winfo.mouse_y);
    winfo.mouse_delta_x = 0;
    winfo.mouse_delta_y = 0;
    winfo.mouse_wheel_x = 0;
    winfo.mouse_wheel_y = 0;
    windows.push_back(winfo);
    return (am_native_window*)win;
}

void am_get_native_window_size(am_native_window *window, int *pw, int *ph, int *sw, int *sh) {
    *pw = 0;
    *ph = 0;
    *sw = 0;
    *sh = 0;
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (windows[i].window == (SDL_Window*)window) {
            SDL_GL_GetDrawableSize(windows[i].window, pw, ph);
            SDL_GetWindowSize(windows[i].window, sw, sh);
            return;
        }
    }
}

bool am_set_native_window_size_and_mode(am_native_window *window, int w, int h, am_window_mode mode) {
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (windows[i].window == (SDL_Window*)window) {
            SDL_Window *sdl_win = (SDL_Window*)window;
            switch (mode) {
                case AM_WINDOW_MODE_FULLSCREEN: {
                    int display = SDL_GetWindowDisplayIndex(sdl_win);
                    if (display < 0) {
                        am_log0("unable to get window display: %s", SDL_GetError());
                        return false;
                    }
                    SDL_DisplayMode desired_mode;
                    SDL_DisplayMode closest_mode;
                    desired_mode.format = 0;
                    desired_mode.w = w;
                    desired_mode.h = h;
                    desired_mode.refresh_rate = 0;
                    desired_mode.driverdata = 0;
                    if (SDL_GetClosestDisplayMode(display, &desired_mode, &closest_mode)) {
                        SDL_SetWindowDisplayMode(sdl_win, &closest_mode);
                    } else {
                        am_log0("unable to query window display modes: %s", SDL_GetError());
                        return false;
                    }
                    SDL_SetWindowFullscreen(sdl_win, SDL_WINDOW_FULLSCREEN);
                    break;
                }
                case AM_WINDOW_MODE_FULLSCREEN_DESKTOP: {
                    SDL_SetWindowFullscreen(sdl_win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    break;
                }
                case AM_WINDOW_MODE_WINDOWED: {
                    SDL_SetWindowFullscreen(sdl_win, 0);
                    SDL_SetWindowSize(sdl_win, w, h);
                    break;
                }
            }
            return true;
        }
    }
    return false;
}

bool am_get_native_window_lock_pointer(am_native_window *window) {
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (windows[i].window == (SDL_Window*)window) {
            SDL_Window *sdl_win = (SDL_Window*)window;
            if (SDL_GetWindowFlags(sdl_win) & SDL_WINDOW_INPUT_FOCUS) {
                return SDL_GetRelativeMouseMode() == SDL_TRUE;
            } else {
                return false;
            }
        }
    }
    return false;
}

void am_set_native_window_lock_pointer(am_native_window *window, bool enabled) {
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (windows[i].window == (SDL_Window*)window) {
            windows[i].lock_pointer = enabled;
            SDL_Window *sdl_win = (SDL_Window*)window;
            if (SDL_GetWindowFlags(sdl_win) & SDL_WINDOW_INPUT_FOCUS) {
                SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
            }
            return;
        }
    }
}

void am_destroy_native_window(am_native_window* window) {
    SDL_Window *sdl_win = (SDL_Window*)window;
    for (int i = windows.size() - 1; i >= 0; i--) {
        // if main window closed, close all windows.
        if (windows[i].window == sdl_win || main_window == sdl_win) {
            if (windows[i].window != main_window) {
                SDL_DestroyWindow((SDL_Window*)window);
            }
            windows.erase(windows.begin() + i);
        }
    }
}

void am_native_window_bind_framebuffer(am_native_window* window) {
    SDL_GL_MakeCurrent((SDL_Window*)window, gl_context);
    am_bind_framebuffer(0);
}

void am_native_window_swap_buffers(am_native_window* window) {
    SDL_GL_SwapWindow((SDL_Window*)window);
}

double am_get_current_time() {
    return ((double)SDL_GetTicks())/1000.0;
}

#define ERR_MSG_SZ 1024

char *am_get_base_path() {
    char *path = SDL_GetBasePath();
    if (path != NULL) {
        // copy so calling code can just use free
        char *path2 = am_format("%s", path);
        SDL_free(path);
        return path2;
    }
    return am_format(".%c", AM_PATH_SEP);
}

char *am_get_data_path() {
    if (am_conf_app_author == NULL || am_conf_app_shortname == NULL) {
        return am_format(".%c", AM_PATH_SEP);
    }
    char *path = SDL_GetPrefPath(am_conf_app_author, am_conf_app_shortname);
    if (path != NULL) {
        // copy so calling code can just use free
        char *path2 = am_format("%s", path);
        SDL_free(path);
        return path2;
    }
    return am_format(".%c", AM_PATH_SEP);
}

static void *am_read_resource_package(const char *filename, int *len, char **errmsg) {
    *errmsg = NULL;
    return am_read_package_resource(package, filename, len, errmsg);
}

static void *am_read_resource_file(const char *filename, int *len, char **errmsg) {
    *errmsg = NULL;
    char tmpbuf[ERR_MSG_SZ];
    char *path = (char*)malloc(strlen(am_opt_data_dir) + 1 + strlen(filename) + 1);
    sprintf(path, "%s%c%s", am_opt_data_dir, AM_PATH_SEP, filename);
    SDL_RWops *f = NULL;
    f = SDL_RWFromFile(path, "r");
    if (f == NULL) {
        snprintf(tmpbuf, ERR_MSG_SZ, "unable to read file %s", path);
        free(path);
        *errmsg = (char*)malloc(strlen(tmpbuf) + 1);
        strcpy(*errmsg, tmpbuf);
        return NULL;
    }
    free(path);
    Sint64 sz = (size_t)SDL_RWsize(f);
    size_t capacity = 1024;
    if (sz > 0) {
        capacity = (size_t)sz;
    }
    char *buf = (char*)malloc(capacity);
    char *ptr = buf;
    size_t total = 0;
    while (true) {
        size_t n = SDL_RWread(f, (void*)ptr, 1, capacity - total);
        total += n;
        if (n == 0) {
            *len = (int)total;
            break;
        } else if (capacity - total == 0) {
            capacity *= 2;
            buf = (char*)realloc(buf, capacity);
            ptr = buf + total;
        }
    }
    assert(*len <= (int)capacity);
    SDL_RWclose(f);
    return buf;
}

void *am_read_resource(const char *filename, int *len, char **errmsg) {
    if (package != NULL) {
        return am_read_resource_package(filename, len, errmsg);
    } else {
        return am_read_resource_file(filename, len, errmsg);
    }
}

#if !defined (AM_OSX) // see am_videocapture_osx.cpp for OSX definition
int am_next_video_capture_frame() {
    return 0;
}
void am_copy_video_frame_to_texture() {
}
#endif

const char *am_preferred_language() {
    return "en";
}

int main( int argc, char *argv[] )
{
    int vsync;
    double t0;
    double t_debt;
    double frame_time = 0.0;
    double delta_time;
    double real_delta_time;

    am_engine *eng = NULL;
    lua_State *L = NULL;

    int exit_status = EXIT_SUCCESS;

#ifdef AM_OSX
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#endif
#ifdef AM_WINDOWS
    SDL_SetMainReady();
#endif

    am_expand_args(&argc, &argv);
    int expanded_argc = argc;
    char **expanded_argv = argv;

    if (!am_process_args(&argc, &argv, &exit_status)) {
        goto quit;
    }
    check_for_package();
    if (am_opt_main_module == NULL) {
        am_opt_main_module = "main";
    }

    if (!am_load_config()) {
        exit_status = EXIT_FAILURE;
        goto quit;
    }
    eng = am_init_engine(false, argc, argv);
    if (eng == NULL) {
        exit_status = EXIT_FAILURE;
        goto quit;
    }

restart:
    L = eng->L;

    frame_time = am_get_current_time();

    lua_pushcclosure(L, am_require, 0);
    lua_pushstring(L, am_opt_main_module);
    if (!am_call(L, 1, 0)) goto quit;
    if (windows.size() == 0) goto quit;

    t0 = am_get_current_time();
    frame_time = t0;
    t_debt = 0.0;
    vsync = -2;

    while (windows.size() > 0 && !restart_triggered) {
        if (vsync != (am_conf_vsync ? 1 : 0)) {
            vsync = (am_conf_vsync ? 1 : 0);
            SDL_GL_SetSwapInterval(vsync);
        }

        if (!am_update_windows(L)) {
            goto quit;
        }

        SDL_LockAudioDevice(audio_device);
        am_sync_audio_graph(L);
        if (should_init_capture) {
            init_audio_capture();
            should_init_capture = false;
            capture_initialized = true;
        }
        SDL_UnlockAudioDevice(audio_device);

        if (!handle_events(L)) goto quit;

        frame_time = am_get_current_time();
        
        real_delta_time = frame_time - t0;
        if (have_focus && am_conf_warn_delta_time > 0.0 && real_delta_time > am_conf_warn_delta_time) {
            am_log0("WARNING: FPS dropped to %0.2f (%fs)", 1.0/real_delta_time, real_delta_time);
        }
        delta_time = am_min(am_conf_max_delta_time, real_delta_time); // fmin in case process was suspended, or last frame took very long
        t_debt += delta_time;

        if (am_conf_fixed_delta_time > 0.0) {
            while (t_debt > 0.0) {
                double t0 = am_get_current_time();
                if (!am_execute_actions(L, am_conf_fixed_delta_time)) {
                    exit_status = EXIT_FAILURE;
                    goto quit;
                }
                double t = am_get_current_time() - t0;
                t_debt -= am_max(t, am_conf_fixed_delta_time);
            }
        } else {
            if (t_debt > am_conf_min_delta_time) {
                if (am_conf_delta_time_step > 0.0) {
                    t_debt = floor(t_debt / am_conf_delta_time_step + 0.5) * am_conf_delta_time_step;
                }
                if (!am_execute_actions(L, t_debt)) {
                    exit_status = EXIT_FAILURE;
                    goto quit;
                }
                t_debt = 0.0;
            }
        }

        t0 = frame_time;

        if (!have_focus) {
            // throttle framerate when in background on osx
#ifdef AM_OSX
            usleep(10 * 1000); // 10 milliseconds
#endif
        }
    }

    if (restart_triggered) {
        restart_triggered = false;
        am_destroy_engine(eng);
        eng = am_init_engine(false, argc, argv);
        goto restart;
    }


quit:
    if (audio_device != 0 || capture_device != 0) {
        if (audio_device != 0) {
            SDL_CloseAudioDevice(audio_device);
        }
        if (capture_device != 0) {
            SDL_CloseAudioDevice(capture_device);
        }
    }
    // destroy lua state before main window, so gl context not
    // destroyed before textures and vbos deleted.
    if (eng != NULL) am_destroy_engine(eng);
    for (unsigned int i = 0; i < windows.size(); i++) {
        // destroy main window last, because it owns the gl context.
        if (windows[i].window != main_window) {
            SDL_DestroyWindow(windows[i].window);
        }
    }
    if (main_window != NULL) {
        SDL_DestroyWindow(main_window);
        main_window = NULL;
    }
    if (sdl_initialized) {
        SDL_Quit();
    }
    if (am_gl_is_initialized()) {
        am_destroy_gl();
    }
    if (package != NULL) {
        am_close_package(package);
    }
    if (audio_buffer != NULL) {
        free(audio_buffer);
        audio_buffer = NULL;
    }
    if (capture_buffer != NULL) {
        free(capture_buffer);
        capture_buffer = NULL;
    }
    am_free_expanded_args(expanded_argc, expanded_argv);
#ifdef AM_OSX
    [pool release];
#endif
    exit(exit_status);
    return exit_status;
}

#ifdef AM_WINDOWS
static void
UnEscapeQuotes(char *arg)
{
    char *last = NULL;

    while (*arg) {
        if (*arg == '"' && (last != NULL && *last == '\\')) {
            char *c_curr = arg;
            char *c_last = last;

            while (*c_curr) {
                *c_last = *c_curr;
                c_last = c_curr;
                c_curr++;
            }
            *c_last = '\0';
        }
        last = arg;
        arg++;
    }
}

static int
ParseCommandLine(char *cmdline, char **argv)
{
    char *bufp;
    char *lastp = NULL;
    int argc, last_argc;

    argc = last_argc = 0;
    for (bufp = cmdline; *bufp;) {
        /* Skip leading whitespace */
        while (SDL_isspace(*bufp)) {
            ++bufp;
        }
        /* Skip over argument */
        if (*bufp == '"') {
            ++bufp;
            if (*bufp) {
                if (argv) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
            /* Skip over word */
            lastp = bufp;
            while (*bufp && (*bufp != '"' || *lastp == '\\')) {
                lastp = bufp;
                ++bufp;
            }
        } else {
            if (*bufp) {
                if (argv) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
            /* Skip over word */
            while (*bufp && !SDL_isspace(*bufp)) {
                ++bufp;
            }
        }
        if (*bufp) {
            if (argv) {
                *bufp = '\0';
            }
            ++bufp;
        }

        /* Strip out \ from \" sequences */
        if (argv && last_argc != argc) {
            UnEscapeQuotes(argv[last_argc]);
        }
        last_argc = argc;
    }
    if (argv) {
        argv[argc] = NULL;
    }
    return (argc);
}

int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    char **argv;
    int argc;
    char *cmdline;

    /* Grab the command line */
    TCHAR *text = GetCommandLine();
    cmdline = SDL_iconv_string("UTF-8", "UCS-2-INTERNAL", (char *)(text), (SDL_wcslen(text)+1)*sizeof(WCHAR));
    if (cmdline == NULL) {
        fprintf(stderr, "out of memory\n");
        return EXIT_FAILURE;
    }

    /* Parse it into argv and argc */
    argc = ParseCommandLine(cmdline, NULL);
    argv = SDL_stack_alloc(char *, argc + 1);
    if (argv == NULL) {
        fprintf(stderr, "out of memory\n");
        return EXIT_FAILURE;
    }
    ParseCommandLine(cmdline, argv);
    int res = main(argc, argv);
    SDL_stack_free(argv);
    SDL_free(cmdline);
    return res;
}
#endif

static void init_sdl() {
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    init_gamepad();
    init_audio();
    sdl_initialized = true;
#if defined(AM_WINDOWS) && defined(AM_GLPROFILE_ES)
    if (!SDL_SetHint(SDL_HINT_VIDEO_WIN_D3DCOMPILER, "d3dcompiler_47.dll")) {
        am_abort("unable to set SDL_HINT_VIDEO_WIN_D3DCOMPILER");
    }
#endif
}

#ifdef AM_NEED_GL_FUNC_PTRS
static void init_gl_func_ptrs() {
#define AM_GLPROC(ret,func,params) \
    do { \
        func##_ptr = (func##_ptr_t)SDL_GL_GetProcAddress(#func); \
        if ( ! func##_ptr ) { \
            am_abort("Couldn't load GL function %s: %s", #func, SDL_GetError()); \
        } \
    } while ( 0 );
#include "am_glfuncs.h"
#undef AM_GLPROC
}
#endif

static void audio_callback(void *ud, Uint8 *stream, int len) {
    //if (capture_device != 0) SDL_LockAudioDevice(capture_device);
    am_always_assert(audio_buffer != NULL);
    am_always_assert(len % ((int)sizeof(float) * am_conf_audio_channels) == 0);
    if (len != (int)sizeof(float) * am_conf_audio_buffer_size * am_conf_audio_channels) {
        // This will cause the buffer pool to be reset. See push_buffer in am_audio.cpp.
        am_conf_audio_buffer_size = len / ((int)sizeof(float) * am_conf_audio_channels);
    }
    int num_channels = am_conf_audio_channels;
    int num_samples = am_conf_audio_buffer_size;
    memset(audio_buffer, 0, len);
    am_audio_bus bus(num_channels, num_samples, audio_buffer);
    am_fill_audio_bus(&bus);
    // SDL expects channels to be interleaved
    am_interleave_audio((float*)stream, bus.buffer, num_channels, num_samples, 0, num_samples);

    int new_offset = capture_read_offset + am_conf_audio_buffer_size * am_conf_audio_channels;
    if (new_offset >= capture_ring_size * am_conf_audio_channels) {
        capture_read_offset = 0;
    } else {
        capture_read_offset = new_offset;
    }
    //am_debug("read_offset = %d", capture_read_offset);
    //if (capture_device != 0) SDL_UnlockAudioDevice(capture_device);
}

static void capture_callback(void *ud, Uint8 *stream, int len) {
    SDL_LockAudioDevice(audio_device);

    assert(capture_buffer != NULL);
    assert(len == (int)sizeof(float) * am_conf_audio_buffer_size * am_conf_audio_channels);
    assert(capture_ring_size * am_conf_audio_channels - capture_write_offset >= am_conf_audio_buffer_size * am_conf_audio_channels);
    memcpy(capture_buffer + capture_write_offset, stream, len);
    capture_write_offset += am_conf_audio_buffer_size * am_conf_audio_channels;
    if (capture_write_offset >= capture_ring_size * am_conf_audio_channels) {
        capture_write_offset = 0;
    }
    /*
    float *data = (float*)stream;
    float sum = 0.0f;
    for (int i = 0; i < am_conf_audio_buffer_size * am_conf_audio_channels; i++) {
        sum += fabsf(data[i]);
    }
    am_debug("sum = %f", sum);
    am_debug("write_offset = %d", capture_write_offset);
    */
    SDL_UnlockAudioDevice(audio_device);
}

void am_capture_audio(am_audio_bus *bus) {
    // this is called from the audio thread, so don't initialize the
    // capture device here. Instead set a flag to initialize it in the main loop.
    if (!capture_initialized && !should_init_capture) {
        should_init_capture = true;
    }
    if (capture_initialized) {
        int samples = am_min(bus->num_samples, am_conf_audio_buffer_size);
        int channels = am_min(bus->num_channels, am_conf_audio_channels);
        am_uninterleave_audio(bus->buffer, capture_buffer + capture_read_offset, channels, samples);
    }
}

static void init_audio() {
    audio_buffer = (float*)malloc(sizeof(float) * am_conf_audio_channels * am_conf_audio_buffer_size);
    SDL_AudioSpec desired;
    desired.freq = am_conf_audio_sample_rate;
    desired.format = AUDIO_F32SYS;
    desired.channels = am_conf_audio_channels;
#ifdef AM_LINUX // XXX Needed for 2nd assert in audio_callback to pass. SDL2 bug?
    desired.samples = am_conf_audio_channels * am_conf_audio_buffer_size;
#else
    desired.samples = am_conf_audio_buffer_size;
#endif
    desired.callback = audio_callback;
    desired.userdata = NULL;
    SDL_AudioSpec obtained;
    //am_debug("desired: freq = %d, channels = %d, samples = %d, format = %d",
    //    desired.freq, desired.channels, desired.samples, desired.format);
    audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    //am_debug("obtained: freq = %d, channels = %d, samples = %d, format = %d",
    //    obtained.freq, obtained.channels, obtained.samples, obtained.format);
    if (audio_device == 0) {
        am_log0("Failed to open audio device: %s", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(audio_device, 0);
}

static void init_audio_capture() {
    capture_ring_size = am_conf_audio_buffer_size;
    capture_write_offset = 0;
    capture_read_offset = 0;
    int sz = sizeof(float) * am_conf_audio_channels * capture_ring_size;
    assert(capture_buffer == NULL);
    capture_buffer = (float*)malloc(sz);
    memset(capture_buffer, 0, sz);
    SDL_AudioSpec desired;
    desired.freq = am_conf_audio_sample_rate;
    desired.format = AUDIO_F32SYS;
    desired.channels = am_conf_audio_channels;
#ifdef AM_LINUX // SDL2 bug?
    desired.samples = am_conf_audio_channels * am_conf_audio_buffer_size;
#else
    desired.samples = am_conf_audio_buffer_size;
#endif
    desired.callback = capture_callback;
    desired.userdata = NULL;
    SDL_AudioSpec obtained;
    //am_debug("desired: freq = %d, channels = %d, samples = %d, format = %d",
    //    desired.freq, desired.channels, desired.samples, desired.format);
    capture_device = SDL_OpenAudioDevice(NULL, 1, &desired, &obtained, 0);
    //am_debug("obtained: freq = %d, channels = %d, samples = %d, format = %d",
    //    obtained.freq, obtained.channels, obtained.samples, obtained.format);
    if (capture_device == 0) {
        am_log0("Failed to open audio capture device: %s", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(capture_device, 0);
}

static void update_window_mouse_state(lua_State *L, win_info *info) {
#ifdef AM_LINUX
    // there seems to be a bug on linux where xrel and yrel are always
    // multiples of 2.
    info->mouse_x += info->mouse_delta_x / 2;
    info->mouse_y += info->mouse_delta_y / 2;
#else
    info->mouse_x += info->mouse_delta_x;
    info->mouse_y += info->mouse_delta_y;
#endif

    info->mouse_delta_x = 0;
    info->mouse_delta_y = 0;

    am_window *win = am_find_window((am_native_window*)info->window);
    //am_debug("update_window_mouse_state %d %d", info->mouse_x, info->mouse_y);
    win->mouse_move(L, (double)info->mouse_x, (double)info->mouse_y);
    win->mouse_wheel(L, (double)info->mouse_wheel_x, (double)info->mouse_wheel_y);
}

static bool handle_events(lua_State *L) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: return false;
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:
                        for (unsigned int i = 0; i < windows.size(); i++) {
                            if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
                                am_handle_window_close((am_native_window*)windows[i].window);
                                break;
                            }
                        }
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        for (unsigned int i = 0; i < windows.size(); i++) {
                            if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
                                SDL_SetRelativeMouseMode(windows[i].lock_pointer ? SDL_TRUE : SDL_FALSE);
                                break;
                            }
                        }
                        have_focus = true;
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        for (unsigned int i = 0; i < windows.size(); i++) {
                            if (SDL_GetWindowID(windows[i].window) == event.window.windowID) {
                                SDL_SetRelativeMouseMode(SDL_FALSE);
                                break;
                            }
                        }
                        have_focus = false;
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                        // ignored, since we recompute window size each frame
                        break;
                }
                break;
            }
            case SDL_KEYDOWN: {
                if (!event.key.repeat) {
                    win_info *info = win_from_id(event.key.windowID);
                    if (info) {
                        am_find_window((am_native_window*)info->window)->push(L);
                        am_key key = convert_key(event.key.keysym.sym);
                        lua_pushstring(L, am_key_name(key));
                        am_call_amulet(L, "_key_down", 2, 0);
                    }
                }
                break;
            }
            case SDL_KEYUP: {
                if (!event.key.repeat) {
                    if (am_conf_allow_restart && event.key.keysym.sym == SDLK_F5) {
                        restart_triggered = true;
                    } else {
                        win_info *info = win_from_id(event.key.windowID);
                        if (info) {
                            am_find_window((am_native_window*)info->window)->push(L);
                            am_key key = convert_key(event.key.keysym.sym);
                            lua_pushstring(L, am_key_name(key));
                            am_call_amulet(L, "_key_up", 2, 0);
                        }
                    }
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                win_info *info = win_from_id(event.motion.windowID);
                if (info) {
                    if (info->lock_pointer) {
                        info->mouse_delta_x += event.motion.xrel;
                        info->mouse_delta_y += event.motion.yrel;
                    } else {
                        info->mouse_x = event.motion.x;
                        info->mouse_y = event.motion.y;
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                if (event.button.which != SDL_TOUCH_MOUSEID) {
                    win_info *info = win_from_id(event.button.windowID);
                    if (info) {
                        am_find_window((am_native_window*)info->window)
                            ->mouse_down(L, convert_mouse_button(event.button.button));
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                if (event.button.which != SDL_TOUCH_MOUSEID) {
                    win_info *info = win_from_id(event.button.windowID);
                    if (info) {
                        am_find_window((am_native_window*)info->window)
                            ->mouse_up(L, convert_mouse_button(event.button.button));
                    }
                }
                break;
            }
            case SDL_MOUSEWHEEL: {
                win_info *info = win_from_id(event.motion.windowID);
                if (info) {
                    info->mouse_wheel_x += event.wheel.x;
                    info->mouse_wheel_y += event.wheel.y;
                }
                break;
            }
            case SDL_CONTROLLERDEVICEADDED: {
                int joyid = event.cdevice.which;
                SDL_GameController *controller = SDL_GameControllerOpen(joyid);
                if (controller) {
                    lua_pushinteger(L, joyid);
                    am_call_amulet(L, "_controller_attached", 1, 0);
                }
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED: {
                int joyid = event.cdevice.which;
                lua_pushinteger(L, joyid);
                am_call_amulet(L, "_controller_detached", 1, 0);
                break;
            }
            case SDL_CONTROLLERBUTTONDOWN: {
                int joyid = event.cbutton.which;
                lua_pushinteger(L, joyid);
                lua_pushstring(L, am_controller_button_name(convert_controller_button(event.cbutton.button)));
                am_call_amulet(L, "_controller_button_down", 2, 0);
                break;
            }
            case SDL_CONTROLLERBUTTONUP: {
                int joyid = event.cbutton.which;
                lua_pushinteger(L, joyid);
                lua_pushstring(L, am_controller_button_name(convert_controller_button(event.cbutton.button)));
                am_call_amulet(L, "_controller_button_up", 2, 0);
                break;
            }
            case SDL_CONTROLLERAXISMOTION: {
                int joyid = event.caxis.which;
                double val = am_clamp((double)event.caxis.value / 32767.0, -1.0, 1.0);
                lua_pushinteger(L, joyid);
                lua_pushstring(L, am_controller_axis_name(convert_controller_axis(event.caxis.axis)));
                lua_pushnumber(L, val);
                am_call_amulet(L, "_controller_axis_motion", 3, 0);
                break;
            }
        }
    }
    for (unsigned i = 0; i < windows.size(); i++) {
        update_window_mouse_state(L, &windows[i]);
    }
    return true;
}

static am_key convert_key(SDL_Keycode key) {
    switch(key) {
        case SDLK_TAB: return AM_KEY_TAB;
        case SDLK_RETURN: return AM_KEY_ENTER;
        case SDLK_ESCAPE: return AM_KEY_ESCAPE;
        case SDLK_SPACE: return AM_KEY_SPACE;
        case SDLK_QUOTE: return AM_KEY_QUOTE;
        case SDLK_EQUALS: return AM_KEY_EQUALS;
        case SDLK_COMMA: return AM_KEY_COMMA;
        case SDLK_MINUS: return AM_KEY_MINUS;
        case SDLK_PERIOD: return AM_KEY_PERIOD;
        case SDLK_SLASH: return AM_KEY_SLASH;
        case SDLK_0: return AM_KEY_0;
        case SDLK_1: return AM_KEY_1;
        case SDLK_2: return AM_KEY_2;
        case SDLK_3: return AM_KEY_3;
        case SDLK_4: return AM_KEY_4;
        case SDLK_5: return AM_KEY_5;
        case SDLK_6: return AM_KEY_6;
        case SDLK_7: return AM_KEY_7;
        case SDLK_8: return AM_KEY_8;
        case SDLK_9: return AM_KEY_9;
        case SDLK_SEMICOLON: return AM_KEY_SEMICOLON;
        case SDLK_LEFTBRACKET: return AM_KEY_LEFTBRACKET;
        case SDLK_BACKSLASH: return AM_KEY_BACKSLASH;
        case SDLK_RIGHTBRACKET: return AM_KEY_RIGHTBRACKET;
        case SDLK_BACKQUOTE: return AM_KEY_BACKQUOTE;
        case SDLK_a: return AM_KEY_A;
        case SDLK_b: return AM_KEY_B;
        case SDLK_c: return AM_KEY_C;
        case SDLK_d: return AM_KEY_D;
        case SDLK_e: return AM_KEY_E;
        case SDLK_f: return AM_KEY_F;
        case SDLK_g: return AM_KEY_G;
        case SDLK_h: return AM_KEY_H;
        case SDLK_i: return AM_KEY_I;
        case SDLK_j: return AM_KEY_J;
        case SDLK_k: return AM_KEY_K;
        case SDLK_l: return AM_KEY_L;
        case SDLK_m: return AM_KEY_M;
        case SDLK_n: return AM_KEY_N;
        case SDLK_o: return AM_KEY_O;
        case SDLK_p: return AM_KEY_P;
        case SDLK_q: return AM_KEY_Q;
        case SDLK_r: return AM_KEY_R;
        case SDLK_s: return AM_KEY_S;
        case SDLK_t: return AM_KEY_T;
        case SDLK_u: return AM_KEY_U;
        case SDLK_v: return AM_KEY_V;
        case SDLK_w: return AM_KEY_W;
        case SDLK_x: return AM_KEY_X;
        case SDLK_y: return AM_KEY_Y;
        case SDLK_z: return AM_KEY_Z;
        case SDLK_DELETE: return AM_KEY_DELETE;
        case SDLK_BACKSPACE: return AM_KEY_BACKSPACE;
        case SDLK_UP: return AM_KEY_UP;
        case SDLK_DOWN: return AM_KEY_DOWN;
        case SDLK_RIGHT: return AM_KEY_RIGHT;
        case SDLK_LEFT: return AM_KEY_LEFT;
        case SDLK_LALT: return AM_KEY_LALT;
        case SDLK_RALT: return AM_KEY_RALT;
        case SDLK_LCTRL: return AM_KEY_LCTRL;
        case SDLK_RCTRL: return AM_KEY_RCTRL;
        case SDLK_LSHIFT: return AM_KEY_LSHIFT;
        case SDLK_RSHIFT: return AM_KEY_RSHIFT;
        case SDLK_F1: return AM_KEY_F1;
        case SDLK_F2: return AM_KEY_F2;
        case SDLK_F3: return AM_KEY_F3;
        case SDLK_F4: return AM_KEY_F4;
        case SDLK_F5: return AM_KEY_F5;
        case SDLK_F6: return AM_KEY_F6;
        case SDLK_F7: return AM_KEY_F7;
        case SDLK_F8: return AM_KEY_F8;
        case SDLK_F9: return AM_KEY_F9;
        case SDLK_F10: return AM_KEY_F10;
        case SDLK_F11: return AM_KEY_F11;
        case SDLK_F12: return AM_KEY_F12;
        case SDLK_CAPSLOCK    : return AM_KEY_CAPSLOCK;
        case SDLK_PRINTSCREEN : return AM_KEY_PRINTSCREEN;
        case SDLK_SCROLLLOCK  : return AM_KEY_SCROLLLOCK;
        case SDLK_PAUSE       : return AM_KEY_PAUSE;
        case SDLK_INSERT      : return AM_KEY_INSERT;
        case SDLK_HOME        : return AM_KEY_HOME;
        case SDLK_PAGEUP      : return AM_KEY_PAGEUP;
        case SDLK_END         : return AM_KEY_END;
        case SDLK_PAGEDOWN    : return AM_KEY_PAGEDOWN;
        case SDLK_KP_DIVIDE   : return AM_KEY_KP_DIVIDE;
        case SDLK_KP_MULTIPLY : return AM_KEY_KP_MULTIPLY;
        case SDLK_KP_MINUS    : return AM_KEY_KP_MINUS;
        case SDLK_KP_PLUS     : return AM_KEY_KP_PLUS;
        case SDLK_KP_ENTER    : return AM_KEY_KP_ENTER;
        case SDLK_KP_1        : return AM_KEY_KP_1;
        case SDLK_KP_2        : return AM_KEY_KP_2;
        case SDLK_KP_3        : return AM_KEY_KP_3;
        case SDLK_KP_4        : return AM_KEY_KP_4;
        case SDLK_KP_5        : return AM_KEY_KP_5;
        case SDLK_KP_6        : return AM_KEY_KP_6;
        case SDLK_KP_7        : return AM_KEY_KP_7;
        case SDLK_KP_8        : return AM_KEY_KP_8;
        case SDLK_KP_9        : return AM_KEY_KP_9;
        case SDLK_KP_0        : return AM_KEY_KP_0;
        case SDLK_KP_PERIOD   : return AM_KEY_KP_PERIOD;
        case SDLK_LGUI        : return AM_KEY_LGUI;
        case SDLK_RGUI        : return AM_KEY_RGUI;
    }
    return AM_KEY_UNKNOWN;
}

static am_mouse_button convert_mouse_button(Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT: return AM_MOUSE_BUTTON_LEFT;
        case SDL_BUTTON_MIDDLE: return AM_MOUSE_BUTTON_MIDDLE;
        case SDL_BUTTON_RIGHT: return AM_MOUSE_BUTTON_RIGHT;
        case SDL_BUTTON_X1: return AM_MOUSE_BUTTON_X1;
        case SDL_BUTTON_X2: return AM_MOUSE_BUTTON_X2;
    }
    return AM_MOUSE_BUTTON_UNKNOWN;
}

static am_controller_button convert_controller_button(Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return AM_CONTROLLER_BUTTON_A;
        case SDL_CONTROLLER_BUTTON_B: return AM_CONTROLLER_BUTTON_B;
        case SDL_CONTROLLER_BUTTON_X: return AM_CONTROLLER_BUTTON_X;
        case SDL_CONTROLLER_BUTTON_Y: return AM_CONTROLLER_BUTTON_Y;
        case SDL_CONTROLLER_BUTTON_BACK: return AM_CONTROLLER_BUTTON_BACK;
        case SDL_CONTROLLER_BUTTON_GUIDE: return AM_CONTROLLER_BUTTON_GUIDE;
        case SDL_CONTROLLER_BUTTON_START: return AM_CONTROLLER_BUTTON_START;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return AM_CONTROLLER_BUTTON_LEFTSTICK;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return AM_CONTROLLER_BUTTON_RIGHTSTICK;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return AM_CONTROLLER_BUTTON_LEFTSHOULDER;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return AM_CONTROLLER_BUTTON_RIGHTSHOULDER;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return AM_CONTROLLER_BUTTON_DPAD_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return AM_CONTROLLER_BUTTON_DPAD_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return AM_CONTROLLER_BUTTON_DPAD_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return AM_CONTROLLER_BUTTON_DPAD_RIGHT;
    }
    return AM_CONTROLLER_BUTTON_UNKNOWN;
}

static am_controller_axis convert_controller_axis(Uint8 axis) {
    switch (axis) {
        case SDL_CONTROLLER_AXIS_LEFTX: return AM_CONTROLLER_AXIS_LEFTX;
        case SDL_CONTROLLER_AXIS_LEFTY: return AM_CONTROLLER_AXIS_LEFTY;
        case SDL_CONTROLLER_AXIS_RIGHTX: return AM_CONTROLLER_AXIS_RIGHTX;
        case SDL_CONTROLLER_AXIS_RIGHTY: return AM_CONTROLLER_AXIS_RIGHTY;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return AM_CONTROLLER_AXIS_TRIGGERLEFT;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return AM_CONTROLLER_AXIS_TRIGGERRIGHT;
    }
    return AM_CONTROLLER_AXIS_UNKNOWN;
}

static bool check_for_package() {
    if (am_opt_main_module != NULL) {
        return true;
    }
    char *package_filename = am_format("%s%c%s", am_opt_data_dir, AM_PATH_SEP, "data.pak");
    if (am_file_exists(package_filename)) {
        char *errmsg;
        package = am_open_package(package_filename, &errmsg);
        if (package == NULL) {
            am_log0("%s", errmsg);
            free(errmsg);
            free(package_filename);
            return false;
        }
    }
    free(package_filename);
    return true;
}

static win_info *win_from_id(Uint32 winid) {
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (SDL_GetWindowID(windows[i].window) == winid) {
            return &windows[i];
        }
    }
    // return window with focus (for mouse events that happen outside
    // window and so don't have a winid)
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (SDL_GetWindowFlags(windows[i].window) & SDL_WINDOW_INPUT_FOCUS) {
            return &windows[i];
        }
    }
    // return first window if present (catches key presses before
    // first window has been fully opened)
    if (windows.size() > 0) {
        return &windows[0];
    }
    return NULL;
}

static const char *controller_mappings[] =
{
#if SDL_JOYSTICK_XINPUT
    "xinput,XInput Controller,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b10,leftshoulder:b4,leftstick:b8,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b9,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,",
#endif
#if SDL_JOYSTICK_DINPUT
    "341a3608000000000000504944564944,Afterglow PS3 Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
    "ffff0000000000000000504944564944,GameStop Gamepad,a:b0,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b2,y:b3,",
    "6d0416c2000000000000504944564944,Generic DirectInput Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
	"6d0419c2000000000000504944564944,Logitech F710 Gamepad,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,", /* Guide button doesn't seem to be sent in DInput mode. */
    "4d6963726f736f66742050432d6a6f79,OUYA Controller,a:b0,b:b3,dpdown:b9,dpleft:b10,dpright:b11,dpup:b8,guide:b14,leftshoulder:b4,leftstick:b6,lefttrigger:b12,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b7,righttrigger:b13,rightx:a5,righty:a4,x:b1,y:b2,",
    "88880803000000000000504944564944,PS3 Controller,a:b2,b:b1,back:b8,dpdown:h0.8,dpleft:h0.4,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b9,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:b7,rightx:a3,righty:a4,start:b11,x:b0,y:b3,",
    "4c056802000000000000504944564944,PS3 Controller,a:b14,b:b13,back:b0,dpdown:b6,dpleft:b7,dpright:b5,dpup:b4,guide:b16,leftshoulder:b10,leftstick:b1,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b11,rightstick:b2,righttrigger:b9,rightx:a2,righty:a3,start:b3,x:b15,y:b12,",
    "25090500000000000000504944564944,PS3 DualShock,a:b2,b:b1,back:b9,dpdown:h0.8,dpleft:h0.4,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b6,leftstick:b10,lefttrigger:b4,leftx:a0,lefty:a1,rightshoulder:b7,rightstick:b11,righttrigger:b5,rightx:a2,righty:a3,start:b8,x:b0,y:b3,",
    "4c05c405000000000000504944564944,PS4 Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,",
#endif
#if defined(__MACOSX__)
    "0500000047532047616d657061640000,GameStop Gamepad,a:b0,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b2,y:b3,",
    "6d0400000000000016c2000000000000,Logitech F310 Gamepad (DInput),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,", /* Guide button doesn't seem to be sent in DInput mode. */
    "6d0400000000000018c2000000000000,Logitech F510 Gamepad (DInput),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
    "6d040000000000001fc2000000000000,Logitech F710 Gamepad (XInput),a:b0,b:b1,back:b9,dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,guide:b10,leftshoulder:b4,leftstick:b6,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b7,righttrigger:a5,rightx:a3,righty:a4,start:b8,x:b2,y:b3,",
    "6d0400000000000019c2000000000000,Logitech Wireless Gamepad (DInput),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,", /* This includes F710 in DInput mode and the "Logitech Cordless RumblePad 2", at the very least. */
    "4c050000000000006802000000000000,PS3 Controller,a:b14,b:b13,back:b0,dpdown:b6,dpleft:b7,dpright:b5,dpup:b4,guide:b16,leftshoulder:b10,leftstick:b1,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b11,rightstick:b2,righttrigger:b9,rightx:a2,righty:a3,start:b3,x:b15,y:b12,",
    "4c05000000000000c405000000000000,PS4 Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,",
    "5e040000000000008e02000000000000,X360 Controller,a:b0,b:b1,back:b9,dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,guide:b10,leftshoulder:b4,leftstick:b6,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b7,righttrigger:a5,rightx:a3,righty:a4,start:b8,x:b2,y:b3,",
#endif
#if defined(__LINUX__)
    "0500000047532047616d657061640000,GameStop Gamepad,a:b0,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b2,y:b3,",
    "03000000ba2200002010000001010000,Jess Technology USB Game Controller,a:b2,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,righttrigger:b7,rightx:a3,righty:a2,start:b9,x:b3,y:b0,",
    "030000006d04000019c2000010010000,Logitech Cordless RumblePad 2,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,",
    "030000006d0400001dc2000014400000,Logitech F310 Gamepad (XInput),a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,",
    "030000006d0400001ec2000020200000,Logitech F510 Gamepad (XInput),a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,",
    "030000006d04000019c2000011010000,Logitech F710 Gamepad (DInput),a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b4,leftstick:b10,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:b7,rightx:a2,righty:a3,start:b9,x:b0,y:b3,", /* Guide button doesn't seem to be sent in DInput mode. */
    "030000006d0400001fc2000005030000,Logitech F710 Gamepad (XInput),a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,",
    "050000003620000100000002010000,OUYA Game Controller,a:b0,b:b3,dpdown:b9,dpleft:b10,dpright:b11,dpup:b8,guide:b14,leftshoulder:b4,leftstick:b6,lefttrigger:a2,leftx:a0,lefty:a1,platform:Linux,rightshoulder:b5,rightstick:b7,righttrigger:a5,rightx:a3,righty:a4,x:b1,y:b2,",
    "030000004c0500006802000011010000,PS3 Controller,a:b14,b:b13,back:b0,dpdown:b6,dpleft:b7,dpright:b5,dpup:b4,guide:b16,leftshoulder:b10,leftstick:b1,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b11,rightstick:b2,righttrigger:b9,rightx:a2,righty:a3,start:b3,x:b15,y:b12,",
    "030000004c050000c405000011010000,PS4 Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,",
    "050000004c050000c405000000010000,PS4 Controller,a:b1,b:b2,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b4,leftstick:b10,lefttrigger:a3,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b11,righttrigger:a4,rightx:a2,righty:a5,start:b9,x:b0,y:b3,",
    "03000000de280000ff11000001000000,Valve Streaming Gamepad,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,",
    "030000005e0400008e02000014010000,X360 Controller,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,",
    "030000005e0400008e02000010010000,X360 Controller,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,",
    "030000005e0400001907000000010000,X360 Wireless Controller,a:b0,b:b1,back:b6,dpdown:b14,dpleft:b11,dpright:b12,dpup:b13,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,",
    "030000005e0400009102000007010000,X360 Wireless Controller,a:b0,b:b1,back:b6,dpdown:b14,dpleft:b11,dpright:b12,dpup:b13,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,righty:a4,start:b7,x:b2,y:b3,",
#endif
#if defined(__ANDROID__)
    "4e564944494120436f72706f72617469,NVIDIA Controller,a:b0,b:b1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftshoulder:b9,leftstick:b7,lefttrigger:a4,leftx:a0,lefty:a1,rightshoulder:b10,rightstick:b8,righttrigger:a5,rightx:a2,righty:a3,start:b6,x:b2,y:b3,",
#endif
    NULL
};

static void init_gamepad() {
    load_controller_mappings();
    if (SDL_NumJoysticks() > 0) {
        //am_debug("detected joysticks = %d", SDL_NumJoysticks());
        char guid[100];
        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(0),
                                          guid, sizeof (guid));
        //am_debug("guid = %s", guid);
        SDL_GameController *controller = SDL_GameControllerOpen(0);
        if (controller != NULL) {
            //am_debug("detected controller");
            controller_present_at_start = true;
        } else {
            //am_debug("controller error: %s", SDL_GetError());
        }
    }
}

static void load_controller_mappings() {
    const char **mapping = &controller_mappings[0];
    while (*mapping != NULL) {
        SDL_GameControllerAddMapping(*mapping);
        mapping++;
    }
}

#endif // AM_BACKEND_SDL
