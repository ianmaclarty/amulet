#include "amulet.h"

#ifdef AM_BACKEND_EMSCRIPTEN

#include "emscripten.h"
#include "SDL/SDL.h"

static SDL_Surface* sdl_window;

static am_package *package = NULL;

static void init_sdl();
static void init_audio();
static bool handle_events();
static am_key convert_key(SDL_Keycode key);
static am_mouse_button convert_mouse_button(Uint8 button);
static void init_mouse_state();
static void open_package();
static void load_package(const char *url);
static void on_load_package_complete(unsigned int x, void *arg, const char *filename);
static void on_load_package_error(unsigned int x, void *arg, int http_status);
static void on_load_package_progress(unsigned int x, void *arg, int perc);

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
    if (sdl_window != NULL) {
        return (am_native_window*)sdl_window;
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
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    Uint32 flags = SDL_OPENGL;
    switch (mode) {
        case AM_WINDOW_MODE_WINDOWED: break;
        case AM_WINDOW_MODE_FULLSCREEN: flags |= SDL_FULLSCREEN; break;
        case AM_WINDOW_MODE_FULLSCREEN_DESKTOP: flags |= SDL_FULLSCREEN; break;
    }
    EM_ASM({
        window.amulet.highdpi = $0;
    }, highdpi ? 1 : 0);
    if (resizable) flags |= SDL_RESIZABLE;
    if (width < 0) width = 0;
    if (height < 0) height = 0;
    // We ignore the supplied width and height and use 0,0 instead.
    // This will cause the system to use the current canvas element size.
    sdl_window = SDL_SetVideoMode(0, 0, 16, flags);
    if (sdl_window == NULL) return NULL;
    if (!am_gl_is_initialized()) {
        am_init_gl();
    }
    init_mouse_state();
    EM_ASM_INT({
        var title_el = document.getElementById("title");
        if (title_el) title_el.innerHTML = Module.UTF8ToString($0);
    }, title);
    return (am_native_window*)sdl_window;
}

void am_get_native_window_size(am_native_window *window, int *pw, int *ph, int *sw, int *sh) {
    SDL_GetWindowSize(NULL /* unused */, pw, ph);
    *sw = *pw;
    *sh = *ph;
}

void am_get_native_window_safe_area_margin(am_native_window *window, 
    int *left, int *right, int *bottom, int *top)
{
    *left = 0;
    *right = 0;
    *bottom = 0;
    *top = 0;
}

void am_destroy_native_window(am_native_window* win) {
}

bool am_set_native_window_size_and_mode(am_native_window *window, int w, int h, am_window_mode mode) {
    return false;
}

void am_native_window_bind_framebuffer(am_native_window* win) {
    am_bind_framebuffer(0);
}

void am_native_window_swap_buffers(am_native_window* win) {
    SDL_GL_SwapBuffers();
}

static am_engine *eng = NULL;
static double t0;
static double frame_time = 0.0;
static double t_debt;
static double fps = 1.0;
static double fps_t0;
static double fps_max;
static double delta_time = 1.0/60.0;
static long long frame_count = 0;
static bool run_loop = true;
static bool was_error = false;
static bool printed_error = false;
static int script_loaded = 0;

double am_get_current_time() {
    return ((double)SDL_GetTicks())/1000.0;
}

static void reset_event_data_if_blurred(am_engine *eng) {
    int blurred = EM_ASM_INT({
        if (window.amulet.window_hidden || !window.amulet.window_has_focus) {
            return 1;
        } else {
            return 0;
        }
    }, 0);
    if (blurred) {
        am_window *win = am_find_window((am_native_window*)sdl_window);
        if (win) {
            win->push(eng->L);
            am_call_amulet(eng->L, "_reset_window_event_data", 1, 0);
        }
    }
}

static void main_loop() {
    if (eng == NULL) return;
    if (was_error) {
        if (!printed_error) {
            am_log0("%s", "ERROR");
            printed_error = true;
            am_update_windows(eng->L); // display error screen
        }
        return;
    }
    if (!run_loop || sdl_window == NULL) return;

    reset_event_data_if_blurred(eng);
    if (!handle_events()) {
        was_error = true;
        return;
    }

    frame_time = am_get_current_time();
    delta_time = fmin(am_conf_max_delta_time, frame_time - t0); // fmin in case process was suspended, or last frame took very long
    t_debt += delta_time;

    if (am_conf_fixed_delta_time > 0.0) {
        while (t_debt > 0.0) {
            if (!am_execute_actions(eng->L, am_conf_fixed_delta_time)) {
                was_error = true;
                return;
            }
            t_debt -= am_conf_fixed_delta_time;
        }
    } else {
        if (!am_execute_actions(eng->L, t_debt)) {
            was_error = true;
            return;
        }
        t_debt = 0.0;
    }

    fps_max = fmax(fps_max, delta_time);
    t0 = frame_time;

    frame_count++;

    if (frame_time - fps_t0 >= 2.0) {
        fps = (double)frame_count / (frame_time - fps_t0);
        //am_debug("fps = %0.2f", fps);
        fps_t0 = t0;
        fps_max = 0.0;
        frame_count = 0;
    }

    if (!am_update_windows(eng->L)) {
        was_error = true;
        return;
    }

    am_sync_audio_graph(eng->L);
}

static void start_main_loop(bool run_main, bool run_waiting) {
    init_sdl();

    if (eng != NULL) {
        am_log0("%s", "INTERNAL ERROR: eng != NULL");
        return;
    }
    if (!am_load_config()) return;
    eng = am_init_engine(false, 0, NULL);
    if (eng == NULL) return;

    frame_time = am_get_current_time();

    if (run_main) {
        script_loaded = 1;
        lua_pushcclosure(eng->L, am_require, 0);
        lua_pushstring(eng->L, "main");
        if (!am_call(eng->L, 1, 0)) {
            run_loop = false;
            was_error = true;
        }
        if (sdl_window == NULL) {
            run_loop = false;
        }
    } else if (run_waiting) {
        script_loaded = 1;
        am_embedded_file_record *rec = am_get_embedded_file("lua/waiting.lua");
        char *waiting_script = (char*)rec->data;
        int len = (int)rec->len;
        if (!am_run_script(eng->L, waiting_script, len, "main.lua")) {
            run_loop = false;
            was_error = true;
        }
    } else {
        run_loop = false;
    }

    t0 = am_get_current_time();
    frame_time = t0;
    t_debt = 0.0;
    fps_t0 = 0.0;
    fps_max = 0.0;
    frame_count = 0;

    emscripten_set_main_loop(main_loop, 0, 0);
}

int main( int argc, char *argv[] )
{
    int no_load_data = EM_ASM_INT({
        return window.amulet.no_load_data ? 1 : 0;
    }, 0);
    int run_waiting = EM_ASM_INT({
        return window.amulet.run_waiting ? 1 : 0;
    }, 0);
    if (run_waiting || no_load_data) {
        start_main_loop(false, run_waiting);
        EM_ASM(
            window.amulet.load_progress = 100;
            window.amulet.ready();
        );
    } else {
        char *url = (char*)EM_ASM_INT({
            var jsString = window.amulet_data_pak_url ? window.amulet_data_pak_url : "data.pak";
            var lengthBytes = lengthBytesUTF8(jsString)+1; // 'jsString.length' would return the length of the string as UTF-16 units, but Emscripten C strings operate as UTF-8.
            var stringOnWasmHeap = _malloc(lengthBytes);
            stringToUTF8(jsString, stringOnWasmHeap, lengthBytes+1);
            return stringOnWasmHeap;
        });
        load_package(url);
        free(url);
    }
}

static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_wheel_x = 0;
static int mouse_wheel_y = 0;

static void init_mouse_state() {
    SDL_GetMouseState(&mouse_x, &mouse_y);
    mouse_wheel_x = 0;
    mouse_wheel_y = 0;
}

static void init_sdl() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    init_audio();
}

static void update_mouse_state(lua_State *L) {
    am_window *win = am_find_window((am_native_window*)sdl_window);
    if (!win) return;
    win->mouse_move(eng->L, (float)mouse_x, (float)mouse_y);
    win->mouse_wheel(eng->L, (double)mouse_wheel_x, (double)mouse_wheel_y);
}

static bool handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: return false;
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:
                        break;
                }
                break;
            }
            case SDL_KEYDOWN: {
                if (!event.key.repeat) {
                    if (event.key.keysym.sym == SDLK_RETURN && (SDL_GetModState() & KMOD_ALT)) {
                        SDL_WM_ToggleFullScreen(sdl_window);
                    } else {
                        am_key key = convert_key(event.key.keysym.sym);
                        am_window *win = am_find_window((am_native_window*)sdl_window);
                        if (win) {
                            win->push(eng->L);
                            lua_pushstring(eng->L, am_key_name(key));
                            was_error = !am_call_amulet(eng->L, "_key_down", 2, 0);
                        }
                    }
                }
                break;
            }
            case SDL_KEYUP: {
                if (!event.key.repeat) {
                    am_key key = convert_key(event.key.keysym.sym);
                    am_window *win = am_find_window((am_native_window*)sdl_window);
                    if (win) {
                        win->push(eng->L);
                        lua_pushstring(eng->L, am_key_name(key));
                        was_error = !am_call_amulet(eng->L, "_key_up", 2, 0);
                    }
                }
                break;
            }
            case SDL_FINGERDOWN: {
                am_window *win = am_find_window((am_native_window*)sdl_window);
                win->touch_begin(
                    eng->L,
                    (void*)event.tfinger.fingerId,
                    event.tfinger.x * win->screen_width,
                    event.tfinger.y * win->screen_height,
                    event.tfinger.pressure
                );
                break;
            }
            case SDL_FINGERMOTION: {
                am_window *win = am_find_window((am_native_window*)sdl_window);
                win->touch_move(
                    eng->L,
                    (void*)event.tfinger.fingerId,
                    event.tfinger.x * win->screen_width,
                    event.tfinger.y * win->screen_height,
                    event.tfinger.pressure
                );
                break;
            }
            case SDL_FINGERUP: {
                am_window *win = am_find_window((am_native_window*)sdl_window);
                win->touch_end(
                    eng->L,
                    (void*)event.tfinger.fingerId,
                    event.tfinger.x * win->screen_width,
                    event.tfinger.y * win->screen_height,
                    event.tfinger.pressure
                );
                break;
            }
            case SDL_MOUSEMOTION: {
                int lock_pointer = EM_ASM_INT({ return window.amulet.have_pointer_lock() ? 1 : 0; }, 0);
                if (lock_pointer) {
                    mouse_x += event.motion.xrel;
                    mouse_y += event.motion.yrel;
                } else {
                    mouse_x = event.motion.x;
                    mouse_y = event.motion.y;
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                if (event.button.which != SDL_TOUCH_MOUSEID) {
                    am_window *win = am_find_window((am_native_window*)sdl_window);
                    if (win) {
                        // make sure we have the current mouse position
                        mouse_x = event.motion.x;
                        mouse_y = event.motion.y;
                        win->mouse_down(eng->L, convert_mouse_button(event.button.button));
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                if (event.button.which != SDL_TOUCH_MOUSEID) {
                    am_window *win = am_find_window((am_native_window*)sdl_window);
                    if (win) {
                        win->mouse_up(eng->L, convert_mouse_button(event.button.button));
                    }
                }
                break;
            }
            case SDL_MOUSEWHEEL: {
                mouse_wheel_x += event.wheel.x > 0 ? 1 : event.wheel.x < 0 ? -1 : 0;
                mouse_wheel_y += event.wheel.y > 0 ? 1 : event.wheel.y < 0 ? -1 : 0;
                break;
            }
            case SDL_VIDEORESIZE: {
                //am_debug("resize w = %d, h = %d", event.resize.w, event.resize.h);
                //am_handle_window_resize((am_native_window*)sdl_window, event.resize.w, event.resize.h);
                break;
            }
        }
    }
    update_mouse_state(eng->L);
    return true;
}

static void audio_callback(void *ud, Uint8 *stream, int len) {
    if (len != (int)sizeof(float) * am_conf_audio_buffer_size * am_conf_audio_channels) {
        am_log0("audio buffer size mismatch! (%d vs %d)",
            len, 
            (int)sizeof(float) * am_conf_audio_buffer_size * am_conf_audio_channels);
        memset(stream, 0, len);
        return;
    }
    int num_channels = am_conf_audio_channels;
    int num_samples = am_conf_audio_buffer_size;
    float *buffer = (float*)stream;
    memset(buffer, 0, sizeof(float) * am_conf_audio_buffer_size * am_conf_audio_channels);
    am_audio_bus bus(num_channels, num_samples, buffer);
    am_fill_audio_bus(&bus);
}

static void init_audio() {
    SDL_AudioSpec desired;
    desired.freq = am_conf_audio_sample_rate;
    desired.format = AUDIO_S16LSB;
    desired.channels = am_conf_audio_channels;
    desired.samples = am_conf_audio_buffer_size;
    desired.callback = audio_callback;
    desired.userdata = NULL;
    SDL_AudioSpec obtained;
    SDL_OpenAudio(&desired, &obtained);
    SDL_PauseAudio(0);
}

void am_set_native_window_lock_pointer(am_native_window *window, bool lock) {
    if (lock) {
        EM_ASM(
            window.amulet.pointer_lock_requested = true;
        );
    } else {
        EM_ASM(
            window.amulet.pointer_lock_requested = false;
            if (document.exitPointerLock) document.exitPointerLock();
        );
    }
}

bool am_get_native_window_lock_pointer(am_native_window *window) {
    return EM_ASM_INT({ return window.amulet.have_pointer_lock() ? 1 : 0; }, 0) == 1;
}

void am_set_native_window_show_cursor(am_native_window *window, bool show) {
    if (show) {
        EM_ASM(
            window.amulet.show_cursor = true;
            document.getElementById("canvas").style.cursor = "pointer";
        );
    } else {
        EM_ASM(
            window.amulet.show_cursor = false;
            document.getElementById("canvas").style.cursor = "none";
        );
    }
}

bool am_get_native_window_show_cursor(am_native_window *window) {
    return EM_ASM_INT({ return window.amulet.show_cursor ? 1 : 0; }, 0) == 1;
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
        case SDLK_LGUI: return AM_KEY_LGUI;
        case SDLK_RGUI: return AM_KEY_RGUI;
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

static void on_load_package_complete(unsigned int x, void *arg, const char *filename) {
    open_package();
    start_main_loop(true, false);
    EM_ASM(
        window.amulet.load_progress = 100;
        window.amulet.ready();
    );
}

static void on_load_package_error(unsigned int x, void *arg, int http_status) {
    am_log0("error loading data (HTTP status: %d)", http_status);
    EM_ASM(
        window.amulet.error("Error loading data file");
    );
}

static void on_load_package_progress(unsigned int x, void *arg, int perc) {
    EM_ASM_INT({
        window.amulet.load_progress = $0;
    }, perc);
}

static void load_package(const char *url) {
    emscripten_async_wget2(url, "data.pak", "GET", "", NULL,
        &on_load_package_complete, &on_load_package_error, &on_load_package_progress); 
}

static void open_package() {
    char *errmsg;
    package = am_open_package("data.pak", &errmsg);
    if (package == NULL) {
        am_log0("%s", errmsg);
        free(errmsg);
    }
}

char *am_get_base_path() {
    return am_format("%s", "./");
}

char *am_get_data_path() {
    return am_format("%s", "./");
}

void *am_read_resource(const char *filename, int *len, char **errmsg) {
    if (package) {
        return am_read_package_resource(package, filename, len, errmsg);
    } else {
        const char *msg = "no package available";
        *errmsg = (char*)malloc(strlen(msg) + 1);
        strcpy(*errmsg, msg);
        return NULL;
    }
}

static int video_frame = 0;
int am_next_video_capture_frame() {
    return video_frame++;
}

void am_copy_video_frame_to_texture() {
    EM_ASM(
        (function() {
            if (!window.amulet.video_capture_initialized) {
                if (!navigator.getUserMedia) {
                    if (!window.amulet.displayed_no_video_capture_alert) {
                        alert("Sorry, your browser does not appear to support video catpure.");
                        window.amulet.displayed_no_video_capture_alert = true;
                    }
                    return;
                }
                var errorCallback = function(e) {
                    window.amulet.video_capture_disallowed = true;
                };
                navigator.getUserMedia({video: true}, function(stream) {
                  var video = document.getElementById("video");
                  video.src = window.URL.createObjectURL(stream);
                  //video.onloadedmetadata = function(e) {
                  //};
                }, errorCallback);
                window.amulet.video_capture_initialized = true;
                window.amulet.video_element = document.getElementById("video");
            } else if (!window.amulet.video_capture_disallowed) {
                GLctx.texImage2D(GLctx.TEXTURE_2D, 0, GLctx.RGBA, GLctx.RGBA, GLctx.UNSIGNED_BYTE,
                    window.amulet.video_element);
            }
        })();
    );
}

void am_capture_audio(am_audio_bus *bus) {
    // NYI
}

const char *am_preferred_language() {
    return "en";
}

char *am_open_file_dialog() {
    return NULL;
}

extern "C" {

void am_emscripten_run(const char *script) {
    if (eng == NULL) {
        am_log0("%s", "INTERNAL ERROR: eng == NULL");
        return;
    }
    if (script_loaded) {
        // restart engine
        am_destroy_engine(eng);
        eng = am_init_engine(false, 0, NULL);
    }
    script_loaded = 1;

    printed_error = false;
    if (am_run_script(eng->L, script, strlen(script), "main.lua")) {
        run_loop = true;
        was_error = false;
    } else {
        was_error = true;
    }
}

void am_emscripten_run_waiting() {
    am_emscripten_run((char*)am_get_embedded_file("lua/waiting.lua")->data);
}

void am_emscripten_pause() {
    run_loop = false;
}

void am_emscripten_resume() {
    run_loop = true;
}

void am_emscripten_resize(int w, int h) {
}

}

lua_State *am_get_global_lua_state() {
    if (eng != NULL) {
        return eng->L;
    }
    return NULL;
}

#endif // AM_BACKEND_EMSCRIPTEN
