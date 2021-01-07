#include "amulet.h"

#ifdef AM_BACKEND_SDL

#define SDL_MAIN_HANDLED 1
#include "SDL.h"
#include "SDL_syswm.h"

#ifdef AM_OSX
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#endif

// Create variables for OpenGL ES 2 functions
#ifdef AM_NEED_GL_FUNC_PTRS
#include <SDL_opengl.h>
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
    bool show_cursor;
    int mouse_x;
    int mouse_y;
    int mouse_delta_x;
    int mouse_delta_y;
    int mouse_wheel_x;
    int mouse_wheel_y;
};

struct controller_info {
    bool active;
    SDL_JoystickID joyid;
    SDL_Haptic *haptic;
    SDL_GameController *controller;
};

#define MAX_CONTROLLERS 8

static controller_info controller_infos[MAX_CONTROLLERS];

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
static bool sdl_initialized = false;
static bool restart_triggered = false;
static lua_State *global_lua_state = NULL;

#ifdef AM_USE_METAL
extern NSWindow *am_metal_window;
extern bool am_metal_use_highdpi;
extern bool am_metal_window_depth_buffer;
extern bool am_metal_window_stencil_buffer;
extern int am_metal_window_msaa_samples;
extern int am_metal_window_pwidth;
extern int am_metal_window_pheight;
extern int am_metal_window_swidth;
extern int am_metal_window_sheight;
#else
static SDL_GLContext gl_context;
static bool gl_context_initialized = false;
#endif

static am_package *package = NULL;

static void init_sdl();
static void init_audio();
static void init_audio_capture();
static void init_controllers();
static bool handle_events(lua_State *L);
static am_key convert_key_scancode(SDL_Scancode key);
static am_mouse_button convert_mouse_button(Uint8 button);
static bool check_for_package();
static win_info *win_from_id(Uint32 winid);

static am_controller_button convert_controller_button(Uint8 button);
static am_controller_axis convert_controller_axis(Uint8 axis);

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
    if (restart_triggered && main_window != NULL) {
        // restarting, return existing window
        return (am_native_window*)main_window;
    }
    if (!sdl_initialized) {
        init_sdl();
    }
#ifdef AM_USE_METAL
    Uint32 flags = 0;
    if (main_window) {
        am_log0("%s", "sorry, only one window is supported");
        return NULL;
    }
#else
    // using gl
    if (main_window != NULL) {
        SDL_GL_MakeCurrent(main_window, gl_context);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    }
    if (am_conf_d3dangle) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    } else {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    }
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
#endif
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
#ifdef AM_USE_METAL
    SDL_SysWMinfo sdl_info;
    SDL_VERSION(&sdl_info.version);
    SDL_GetWindowWMInfo(win, &sdl_info);
    am_metal_window = sdl_info.info.cocoa.window;
    am_metal_use_highdpi = highdpi;
    am_metal_window_depth_buffer = depth_buffer;
    am_metal_window_stencil_buffer = stencil_buffer;
    am_metal_window_msaa_samples = msaa_samples;
#else
    if (!gl_context_initialized) {
        gl_context = SDL_GL_CreateContext(win);
#ifdef AM_NEED_GL_FUNC_PTRS
        init_gl_func_ptrs();
#endif
        gl_context_initialized = true;
    }
    SDL_GL_MakeCurrent(win, gl_context);
#endif
    if (!am_gl_is_initialized()) {
        am_init_gl();
    }
    if (main_window == NULL) {
        main_window = win;
    }
    win_info winfo;
    winfo.window = win;
    winfo.lock_pointer = false;
    winfo.show_cursor = true;
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
#ifdef AM_USE_METAL
    *pw = am_metal_window_pwidth;
    *ph = am_metal_window_pheight;
    *sw = am_metal_window_swidth;
    *sh = am_metal_window_sheight;
#else
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (windows[i].window == (SDL_Window*)window) {
            SDL_GetWindowSize(windows[i].window, sw, sh);
            SDL_GL_GetDrawableSize(windows[i].window, pw, ph);
            return;
        }
    }
#endif
}

void am_get_native_window_safe_area_margin(am_native_window *window, 
    int *left, int *right, int *bottom, int *top)
{
    *left = 0;
    *right = 0;
    *bottom = 0;
    *top = 0;
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

bool am_get_native_window_show_cursor(am_native_window *window) {
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (windows[i].window == (SDL_Window*)window) {
            return windows[i].show_cursor;
        }
    }
    return false;
}

void am_set_native_window_show_cursor(am_native_window *window, bool show) {
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (windows[i].window == (SDL_Window*)window) {
            windows[i].show_cursor = show;
            SDL_Window *sdl_win = (SDL_Window*)window;
            if (SDL_GetWindowFlags(sdl_win) & SDL_WINDOW_INPUT_FOCUS) {
                SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
            }
            return;
        }
    }
}

void am_destroy_native_window(am_native_window* window) {
    am_log_gl("// destroy window");
    SDL_Window *sdl_win = (SDL_Window*)window;
    for (unsigned int i = 0; i < windows.size(); i++) {
        if (windows[i].window == sdl_win) {
            SDL_DestroyWindow(sdl_win);
            windows.erase(windows.begin()+i);
            break;
        }
    }
}

void am_native_window_bind_framebuffer(am_native_window* window) {
#if !defined(AM_USE_METAL)
    SDL_GL_MakeCurrent((SDL_Window*)window, gl_context);
#endif
    am_bind_framebuffer(0);
}

void am_native_window_swap_buffers(am_native_window* window) {
    am_gl_end_frame(true);
#if !defined(AM_USE_METAL)
    SDL_GL_SwapWindow((SDL_Window*)window);
#endif
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
    char *path;
    bool force_filesystem = filename[0] == '@';
    if (force_filesystem) {
        path = (char*)(filename + 1);
    } else {
        path = (char*)malloc(strlen(am_opt_data_dir) + 1 + strlen(filename) + 1);
        sprintf(path, "%s%c%s", am_opt_data_dir, AM_PATH_SEP, filename);
    }
    SDL_RWops *f = NULL;
    f = SDL_RWFromFile(path, "r");
    if (f == NULL) {
        snprintf(tmpbuf, ERR_MSG_SZ, "unable to read file %s", path);
        if (!force_filesystem) free(path);
        *errmsg = (char*)malloc(strlen(tmpbuf) + 1);
        strcpy(*errmsg, tmpbuf);
        return NULL;
    }
    if (!force_filesystem) free(path);
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
    bool force_filesystem = filename[0] == '@';
    if (!force_filesystem && package != NULL) {
        return am_read_resource_package(filename, len, errmsg);
    } else {
        return am_read_resource_file(filename, len, errmsg);
    }
}

#if !(defined (AM_OSX) && !defined(AM_USE_METAL))// see am_videocapture_osx.cpp for OSX (opengl) definition
int am_next_video_capture_frame() {
    return 0;
}
void am_copy_video_frame_to_texture() {
}
#endif

#if defined(AM_WINDOWS) && !defined(AM_MINGW) 

static char *from_wstr(const wchar_t *str) {
    int len8 = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    char *str8 = (char*)malloc(len8);
    WideCharToMultiByte(CP_UTF8, 0, str, -1, str8, len8, NULL, NULL);
    return str8;
}

char *am_open_file_dialog() {
    OPENFILENAME ofn;
    const int maxlen = 1000;
    wchar_t str[maxlen];
    for (int i = 0; i < maxlen; i++) {
        str[i] = 0;
    }

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = str;
    ofn.lpstrFile[0] = 0;
    ofn.nMaxFile = maxlen;
    ofn.lpstrFilter = NULL;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 0;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    char* result = NULL;

    if (GetOpenFileName(&ofn)==TRUE) {
        result = from_wstr(str);
    }
    return result;
}
#else
char *am_open_file_dialog() {
    return NULL;
}
#endif

const char *am_preferred_language() {
#if defined(AM_STEAMWORKS)
    return am_get_steam_lang();
#elif defined(AM_OSX)
    NSString *bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
    bool in_osx_bundle = (bundleIdentifier != nil);
    if (in_osx_bundle) {
        NSString *languageID = [[NSBundle mainBundle] preferredLocalizations].firstObject;
        return [languageID UTF8String];
    } else {
        return "en";
    }
#else
    return "en";
#endif
}

#if defined (AM_OSX)
static int launch_url(lua_State *L) {
    am_check_nargs(L, 1);
    const char *url = lua_tostring(L, 1);
    if (url == NULL) return 0;
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
    return 0;
}
#endif

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

#ifdef AM_WINDOWS
    SDL_SetMainReady();
#endif

    am_expand_args(&argc, &argv);
    int expanded_argc = argc;
    char **expanded_argv = argv;

#ifdef AM_OSX
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#endif

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

restart:
    eng = am_init_engine(false, argc, argv);
    if (eng == NULL) {
        exit_status = EXIT_FAILURE;
        goto quit;
    }

    L = eng->L;
    global_lua_state = L;

    frame_time = am_get_current_time();

    lua_pushcclosure(L, am_require, 0);
    lua_pushstring(L, am_opt_main_module);
    if (!am_call(L, 1, 0)) {
        if (windows.size() == 0) {
            exit_status = EXIT_FAILURE;
        }
    }
    if (windows.size() == 0) goto quit;

    if (restart_triggered) {
        // re "attach" the controllers on the lua side after reloading the
        // world.
        for (int index = 0; index < MAX_CONTROLLERS; ++index) {
            if (controller_infos[index].active) {
                lua_pushinteger(L, index);
                lua_pushinteger(L, controller_infos[index].joyid);
                am_call_amulet(L, "_controller_attached", 2, 0);
            }
        }
    }

    restart_triggered = false;

    t0 = am_get_current_time();
    frame_time = t0;
    t_debt = 0.0;
    vsync = -2;

    while (windows.size() > 0 && !restart_triggered) {
#if !defined(AM_USE_METAL)
        if (vsync != (am_conf_vsync ? 1 : 0)) {
            vsync = (am_conf_vsync ? 1 : 0);
            SDL_GL_SetSwapInterval(vsync);
        }
#endif

        if (!am_update_windows(L)) {
            goto quit;
        }

#ifdef AM_OSX
        // release pool after updating windows, so that some metal objects, such as the layer,
        // persist for the entire frame.
        [pool release];
        pool = [[NSAutoreleasePool alloc] init];
#endif
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

#ifdef AM_STEAMWORKS
        am_steam_step();
#endif
        if (am_conf_fixed_delta_time > 0.0) {
            while (t_debt > 0.0) {
                double t0 = am_get_current_time();
#ifdef AM_STEAMWORKS
                if (!am_steam_overlay_enabled) {
#endif
                am_execute_actions(L, am_conf_fixed_delta_time);
#ifdef AM_STEAMWORKS
                }
#endif
                double t = am_get_current_time() - t0;
                t_debt -= am_max(t, am_conf_fixed_delta_time);
            }
        } else {
            if (t_debt > am_conf_min_delta_time) {
                if (am_conf_delta_time_step > 0.0) {
                    t_debt = floor(t_debt / am_conf_delta_time_step + 0.5) * am_conf_delta_time_step;
                }
#ifdef AM_STEAMWORKS
                if (!am_steam_overlay_enabled) {
#endif
                am_execute_actions(L, t_debt);
#ifdef AM_STEAMWORKS
                }
#endif
                t_debt = 0.0;
            }
        }

        t0 = frame_time;

        if (!have_focus) {
#if defined(AM_OSX)
            // throttle framerate when in background on osx, otherwise cpu usage becomes very high for unknown reasons
            usleep(50 * 1000); // 50 milliseconds
#endif
        }
    }

    if (restart_triggered) {
        SDL_LockAudioDevice(audio_device);
        am_destroy_engine(eng);
        global_lua_state = NULL;
        L = NULL;
        SDL_UnlockAudioDevice(audio_device);
        goto restart;
    }


quit:
    if (audio_device != 0 || capture_device != 0) {
        am_log_gl("// close audio");
        if (audio_device != 0) {
            SDL_CloseAudioDevice(audio_device);
        }
        if (capture_device != 0) {
            SDL_CloseAudioDevice(capture_device);
        }
    }
    am_log_gl("// destroy engine");
    // destroy lua state before main window, so gl context not
    // destroyed before textures and vbos deleted.
    if (eng != NULL) am_destroy_engine(eng);
    for (unsigned int i = 0; i < windows.size(); i++) {
        // destroy main window last, because it owns the gl context.
        if (windows[i].window != main_window) {
            am_log_gl("// destroy non-main window");
            SDL_DestroyWindow(windows[i].window);
        }
    }
    if (main_window != NULL) {
        am_log_gl("// destroy main window");
        SDL_DestroyWindow(main_window);
        main_window = NULL;
    }
    if (sdl_initialized) {
        am_log_gl("// quit sdl");
        SDL_Quit();
    }
    if (am_gl_is_initialized()) {
        am_log_gl("// destroy gl");
        am_destroy_gl();
    }
    if (package != NULL) {
        am_log_gl("// close package");
        am_close_package(package);
    }
    if (audio_buffer != NULL) {
        am_log_gl("// free audio buffer");
        free(audio_buffer);
        audio_buffer = NULL;
    }
    if (capture_buffer != NULL) {
        am_log_gl("// free capture buffer");
        free(capture_buffer);
        capture_buffer = NULL;
    }
    am_log_gl("// free expanded args");
    am_free_expanded_args(expanded_argc, expanded_argv);
    am_log_gl("// exit");
    am_close_gllog();
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

static void add_extra_controller_mappings() {
    SDL_GameControllerAddMapping("03000000c0160000e105000000040000,Xinmoteck 1 Player,a:b1,b:b2,y:b3,x:b0,start:b9,guide:b12,back:b8,leftx:a0,lefty:a1,rightx:a2,righty:a3,leftshoulder:b4,rightshoulder:b5,lefttrigger:b6,righttrigger:b7");
}

static void init_sdl() {
    add_extra_controller_mappings();
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER);
    init_audio();
    init_controllers();
    sdl_initialized = true;
    if (am_conf_d3dangle) {
        if (!SDL_SetHint(SDL_HINT_VIDEO_WIN_D3DCOMPILER, "d3dcompiler_47.dll")) {
            am_abort("unable to set SDL_HINT_VIDEO_WIN_D3DCOMPILER");
        }
    }
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

static void init_controllers() {
    for (int i = 0; i < MAX_CONTROLLERS; ++i) {
        controller_infos[i].active = false;
        controller_infos[i].joyid = 0;
        controller_infos[i].haptic = NULL;
    }
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
                                SDL_ShowCursor(windows[i].show_cursor ? SDL_ENABLE : SDL_DISABLE);
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
                        am_key key = convert_key_scancode(event.key.keysym.scancode);
                        if (key != AM_KEY_UNKNOWN) {
                            lua_pushstring(L, am_key_name(key));
                        } else {
                            lua_pushfstring(L, "#%d", event.key.keysym.scancode);
                        }
                        am_call_amulet(L, "_key_down", 2, 0);
                    }
                }
                break;
            }
            case SDL_KEYUP: {
                if (!event.key.repeat) {
                    if (!package && event.key.keysym.scancode == SDL_SCANCODE_F5) {
                        restart_triggered = true;
                    } else if (!package && event.key.keysym.scancode == SDL_SCANCODE_F6) {
                        am_call_amulet(L, "toggle_perf_overlay", 0, 0);
                    } else {
                        win_info *info = win_from_id(event.key.windowID);
                        if (info) {
                            am_find_window((am_native_window*)info->window)->push(L);
                            am_key key = convert_key_scancode(event.key.keysym.scancode);
                            if (key != AM_KEY_UNKNOWN) {
                                lua_pushstring(L, am_key_name(key));
                            } else {
                                lua_pushfstring(L, "#%d", event.key.keysym.scancode);
                            }
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
                win_info *info = win_from_id(event.button.windowID);
                if (info) {
                    Uint8 button;
                    if (event.button.which == SDL_TOUCH_MOUSEID) {
                        button = SDL_BUTTON_LEFT;
                    } else {
                        button = event.button.button;
                    }
                    am_find_window((am_native_window*)info->window)
                        ->mouse_down(L, convert_mouse_button(button));
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                win_info *info = win_from_id(event.button.windowID);
                if (info) {
                    Uint8 button;
                    if (event.button.which == SDL_TOUCH_MOUSEID) {
                        button = SDL_BUTTON_LEFT;
                    } else {
                        button = event.button.button;
                    }
                    am_find_window((am_native_window*)info->window)
                        ->mouse_up(L, convert_mouse_button(button));
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
                int index = event.cdevice.which;
                if (index < MAX_CONTROLLERS) {
                    SDL_GameController *controller = SDL_GameControllerOpen(index);
                    if (controller) {
                        int joyid = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
                        controller_infos[index].active = true;
                        controller_infos[index].joyid = joyid;
                        controller_infos[index].controller = controller;
                        SDL_Joystick *joy = SDL_JoystickFromInstanceID(joyid);
                        if (joy != NULL) {
                            controller_infos[index].haptic = SDL_HapticOpenFromJoystick(joy);
                        }
                        lua_pushinteger(L, index);
                        lua_pushinteger(L, joyid);
                        am_call_amulet(L, "_controller_attached", 2, 0);
                    }
                }
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED: {
                int joyid = event.cdevice.which;
                lua_pushinteger(L, joyid);
                am_call_amulet(L, "_controller_detached", 1, 0);
                for (int i = 0; i < MAX_CONTROLLERS; ++i) {
                    if (controller_infos[i].active && controller_infos[i].joyid == joyid) {
                        controller_infos[i].active = false;
                        controller_infos[i].joyid = 0;
                        if (controller_infos[i].haptic != NULL) {
                            SDL_HapticClose(controller_infos[i].haptic);
                            controller_infos[i].haptic = NULL;
                        }
                        SDL_GameControllerClose(controller_infos[i].controller);
                        controller_infos[i].controller = NULL;
                        break;
                    }
                }
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
                double val = am_clamp((double)event.caxis.value / 32768.0, -1.0, 1.0);
                if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY || event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTY) {
                    val = -val;
                }
                lua_pushinteger(L, joyid);
                lua_pushstring(L, am_controller_axis_name(convert_controller_axis(event.caxis.axis)));
                lua_pushnumber(L, val);
                am_call_amulet(L, "_controller_axis_motion", 3, 0);
                break;
            }
            /*
            case SDL_JOYDEVICEADDED: {
                int joy_index = event.jdevice.which;
                SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(joy_index);
                char name[100];
                SDL_JoystickGetGUIDString(guid, name, 100);
                am_debug("joy added '%s'", name);
                SDL_JoystickOpen(joy_index);
                break;
            }
            case SDL_JOYDEVICEREMOVED: {
                am_debug("%s", "joy removed");
                break;
            }
            case SDL_JOYBUTTONDOWN: {
                am_debug("joy button down %d", event.jbutton.button);
                break;
            }
            case SDL_JOYBUTTONUP: {
                am_debug("%s", "joy button up");
                break;
            }
            case SDL_JOYAXISMOTION: {
                am_debug("joy axis %d %d", event.jaxis.axis, event.jaxis.value);
                break;
            }
            case SDL_JOYBALLMOTION: {
                am_debug("%s", "joy ball");
                break;
            }
            case SDL_JOYHATMOTION: {
                am_debug("%s", "joy hat");
                break;
            }
            */
        }
    }
    for (unsigned i = 0; i < windows.size(); i++) {
        update_window_mouse_state(L, &windows[i]);
    }
    return true;
}

static am_key convert_key_scancode(SDL_Scancode key) {
    switch(key) {
        case SDL_SCANCODE_TAB: return AM_KEY_TAB;
        case SDL_SCANCODE_RETURN: return AM_KEY_ENTER;
        case SDL_SCANCODE_ESCAPE: return AM_KEY_ESCAPE;
        case SDL_SCANCODE_SPACE: return AM_KEY_SPACE;
        case SDL_SCANCODE_APOSTROPHE: return AM_KEY_QUOTE;
        case SDL_SCANCODE_EQUALS: return AM_KEY_EQUALS;
        case SDL_SCANCODE_COMMA: return AM_KEY_COMMA;
        case SDL_SCANCODE_MINUS: return AM_KEY_MINUS;
        case SDL_SCANCODE_PERIOD: return AM_KEY_PERIOD;
        case SDL_SCANCODE_SLASH: return AM_KEY_SLASH;
        case SDL_SCANCODE_0: return AM_KEY_0;
        case SDL_SCANCODE_1: return AM_KEY_1;
        case SDL_SCANCODE_2: return AM_KEY_2;
        case SDL_SCANCODE_3: return AM_KEY_3;
        case SDL_SCANCODE_4: return AM_KEY_4;
        case SDL_SCANCODE_5: return AM_KEY_5;
        case SDL_SCANCODE_6: return AM_KEY_6;
        case SDL_SCANCODE_7: return AM_KEY_7;
        case SDL_SCANCODE_8: return AM_KEY_8;
        case SDL_SCANCODE_9: return AM_KEY_9;
        case SDL_SCANCODE_SEMICOLON: return AM_KEY_SEMICOLON;
        case SDL_SCANCODE_LEFTBRACKET: return AM_KEY_LEFTBRACKET;
        case SDL_SCANCODE_BACKSLASH: return AM_KEY_BACKSLASH;
        case SDL_SCANCODE_RIGHTBRACKET: return AM_KEY_RIGHTBRACKET;
        case SDL_SCANCODE_GRAVE: return AM_KEY_BACKQUOTE;
        case SDL_SCANCODE_A: return AM_KEY_A;
        case SDL_SCANCODE_B: return AM_KEY_B;
        case SDL_SCANCODE_C: return AM_KEY_C;
        case SDL_SCANCODE_D: return AM_KEY_D;
        case SDL_SCANCODE_E: return AM_KEY_E;
        case SDL_SCANCODE_F: return AM_KEY_F;
        case SDL_SCANCODE_G: return AM_KEY_G;
        case SDL_SCANCODE_H: return AM_KEY_H;
        case SDL_SCANCODE_I: return AM_KEY_I;
        case SDL_SCANCODE_J: return AM_KEY_J;
        case SDL_SCANCODE_K: return AM_KEY_K;
        case SDL_SCANCODE_L: return AM_KEY_L;
        case SDL_SCANCODE_M: return AM_KEY_M;
        case SDL_SCANCODE_N: return AM_KEY_N;
        case SDL_SCANCODE_O: return AM_KEY_O;
        case SDL_SCANCODE_P: return AM_KEY_P;
        case SDL_SCANCODE_Q: return AM_KEY_Q;
        case SDL_SCANCODE_R: return AM_KEY_R;
        case SDL_SCANCODE_S: return AM_KEY_S;
        case SDL_SCANCODE_T: return AM_KEY_T;
        case SDL_SCANCODE_U: return AM_KEY_U;
        case SDL_SCANCODE_V: return AM_KEY_V;
        case SDL_SCANCODE_W: return AM_KEY_W;
        case SDL_SCANCODE_X: return AM_KEY_X;
        case SDL_SCANCODE_Y: return AM_KEY_Y;
        case SDL_SCANCODE_Z: return AM_KEY_Z;
        case SDL_SCANCODE_DELETE: return AM_KEY_DELETE;
        case SDL_SCANCODE_BACKSPACE: return AM_KEY_BACKSPACE;
        case SDL_SCANCODE_UP: return AM_KEY_UP;
        case SDL_SCANCODE_DOWN: return AM_KEY_DOWN;
        case SDL_SCANCODE_RIGHT: return AM_KEY_RIGHT;
        case SDL_SCANCODE_LEFT: return AM_KEY_LEFT;
        case SDL_SCANCODE_LALT: return AM_KEY_LALT;
        case SDL_SCANCODE_RALT: return AM_KEY_RALT;
        case SDL_SCANCODE_LCTRL: return AM_KEY_LCTRL;
        case SDL_SCANCODE_RCTRL: return AM_KEY_RCTRL;
        case SDL_SCANCODE_LSHIFT: return AM_KEY_LSHIFT;
        case SDL_SCANCODE_RSHIFT: return AM_KEY_RSHIFT;
        case SDL_SCANCODE_F1: return AM_KEY_F1;
        case SDL_SCANCODE_F2: return AM_KEY_F2;
        case SDL_SCANCODE_F3: return AM_KEY_F3;
        case SDL_SCANCODE_F4: return AM_KEY_F4;
        case SDL_SCANCODE_F5: return AM_KEY_F5;
        case SDL_SCANCODE_F6: return AM_KEY_F6;
        case SDL_SCANCODE_F7: return AM_KEY_F7;
        case SDL_SCANCODE_F8: return AM_KEY_F8;
        case SDL_SCANCODE_F9: return AM_KEY_F9;
        case SDL_SCANCODE_F10: return AM_KEY_F10;
        case SDL_SCANCODE_F11: return AM_KEY_F11;
        case SDL_SCANCODE_F12: return AM_KEY_F12;
        case SDL_SCANCODE_CAPSLOCK    : return AM_KEY_CAPSLOCK;
        case SDL_SCANCODE_PRINTSCREEN : return AM_KEY_PRINTSCREEN;
        case SDL_SCANCODE_SCROLLLOCK  : return AM_KEY_SCROLLLOCK;
        case SDL_SCANCODE_NUMLOCKCLEAR: return AM_KEY_NUMLOCK;
        case SDL_SCANCODE_PAUSE       : return AM_KEY_PAUSE;
        case SDL_SCANCODE_INSERT      : return AM_KEY_INSERT;
        case SDL_SCANCODE_HOME        : return AM_KEY_HOME;
        case SDL_SCANCODE_PAGEUP      : return AM_KEY_PAGEUP;
        case SDL_SCANCODE_END         : return AM_KEY_END;
        case SDL_SCANCODE_PAGEDOWN    : return AM_KEY_PAGEDOWN;
        case SDL_SCANCODE_KP_DIVIDE   : return AM_KEY_KP_DIVIDE;
        case SDL_SCANCODE_KP_MULTIPLY : return AM_KEY_KP_MULTIPLY;
        case SDL_SCANCODE_KP_MINUS    : return AM_KEY_KP_MINUS;
        case SDL_SCANCODE_KP_PLUS     : return AM_KEY_KP_PLUS;
        case SDL_SCANCODE_KP_ENTER    : return AM_KEY_KP_ENTER;
        case SDL_SCANCODE_KP_1        : return AM_KEY_KP_1;
        case SDL_SCANCODE_KP_2        : return AM_KEY_KP_2;
        case SDL_SCANCODE_KP_3        : return AM_KEY_KP_3;
        case SDL_SCANCODE_KP_4        : return AM_KEY_KP_4;
        case SDL_SCANCODE_KP_5        : return AM_KEY_KP_5;
        case SDL_SCANCODE_KP_6        : return AM_KEY_KP_6;
        case SDL_SCANCODE_KP_7        : return AM_KEY_KP_7;
        case SDL_SCANCODE_KP_8        : return AM_KEY_KP_8;
        case SDL_SCANCODE_KP_9        : return AM_KEY_KP_9;
        case SDL_SCANCODE_KP_0        : return AM_KEY_KP_0;
        case SDL_SCANCODE_KP_PERIOD   : return AM_KEY_KP_PERIOD;
        case SDL_SCANCODE_LGUI        : return AM_KEY_LGUI;
        case SDL_SCANCODE_RGUI        : return AM_KEY_RGUI;
        default                       : return AM_KEY_UNKNOWN;
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

static int rumble(lua_State *L) {
    am_check_nargs(L, 3);
    int joyid = lua_tointeger(L, 1);
    double duration = lua_tonumber(L, 2);
    double strength = lua_tonumber(L, 3);
    SDL_Haptic *haptic = NULL;
    for (int i = 0; i < MAX_CONTROLLERS; ++i) {
        if (controller_infos[i].active && controller_infos[i].joyid == joyid) {
            haptic = controller_infos[i].haptic;
        }
    }
    if (haptic == NULL) return 0;
    if (SDL_HapticRumbleInit(haptic) != 0) return 0;
    SDL_HapticRumblePlay(haptic, strength, (int)(duration * 1000.0));
    return 0;
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

lua_State *am_get_global_lua_state() {
    return global_lua_state;
}

void am_open_sdl_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"_rumble", rumble},
#if defined(AM_OSX)
        {"launch_url", launch_url},
#endif
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}

#endif // AM_BACKEND_SDL
