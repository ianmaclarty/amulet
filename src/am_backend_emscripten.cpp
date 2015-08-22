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
static void load_package();
static void on_load_package_complete(unsigned int x, void *arg, const char *filename);
static void on_load_package_error(unsigned int x, void *arg, int http_status);
static void on_load_package_progress(unsigned int x, void *arg, int perc);

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
    int msaa_samples)
{
    if (sdl_window != NULL) return NULL;
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
    if (resizable) flags |= SDL_RESIZABLE;
    if (width < 0) width = 0;
    if (height < 0) height = 0;
    // We ignore the supplied width and height and use 0,0 instead.
    // This will cause the system to use the current canvas element size.
    sdl_window = SDL_SetVideoMode(0, 0, 16, flags);
    if (sdl_window == NULL) return NULL;
    am_init_gl();
    init_mouse_state();
    return (am_native_window*)sdl_window;
}

static int oldw = 0;
static int oldh = 0;
void am_get_native_window_size(am_native_window *window, int *w, int *h) {
    SDL_GetWindowSize(NULL /* unused */, w, h);
    if (oldw != *w || oldh != *h) {
        //am_debug("native window size changed to %dx%d", *w, *h);
        oldw = *w; oldh = *h;
    }
}

void am_destroy_native_window(am_native_window* win) {
    sdl_window = NULL;
}

bool am_set_native_window_size_and_mode(am_native_window *window, int w, int h, am_window_mode mode) {
    return false;
}

void am_native_window_pre_render(am_native_window* win) {
    am_bind_framebuffer(0);
}

void am_native_window_post_render(am_native_window* win) {
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

double am_get_current_time() {
    return ((double)SDL_GetTicks())/1000.0;
}

static void main_loop() {
    if (!run_loop || sdl_window == NULL) return;

    if (!am_update_windows(eng->L)) {
        run_loop = false;
        return;
    }

    am_sync_audio_graph(eng->L);

    if (!handle_events()) {
        run_loop = false;
        return;
    }

    frame_time = am_get_current_time();
    delta_time = fmin(am_conf_min_delta_time, frame_time - t0); // fmin in case process was suspended, or last frame took very long
    t_debt += delta_time;

    if (am_conf_fixed_delta_time > 0.0) {
        while (t_debt > 0.0) {
            if (!am_execute_actions(eng->L, am_conf_fixed_delta_time)) {
                run_loop = false;
                return;
            }
            t_debt -= am_conf_fixed_delta_time;
        }
    } else {
        if (!am_execute_actions(eng->L, t_debt)) {
            run_loop = false;
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
    SDL_GL_SwapBuffers();
}

int main( int argc, char *argv[] )
{
    load_package();
}

static void start() {
    init_sdl();

    eng = am_init_engine(false, 0, NULL);
    if (eng == NULL) return;

    frame_time = am_get_current_time();

    lua_pushcclosure(eng->L, am_load_module, 0);
    lua_pushstring(eng->L, "main");
    if (!am_call(eng->L, 1, 0)) return;
    if (sdl_window == NULL) return;

    t0 = am_get_current_time();
    frame_time = t0;
    t_debt = 0.0;
    fps_t0 = 0.0;
    fps_max = 0.0;
    frame_count = 0;

    emscripten_set_main_loop(main_loop, 0, 0);
}

static int mouse_x = 0;
static int mouse_y = 0;

static void init_mouse_state() {
    SDL_GetMouseState(&mouse_x, &mouse_y);
}

static void init_sdl() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    init_audio();
}

static void update_mouse_state(lua_State *L) {
    int w, h;
    SDL_GetWindowSize(NULL, &w, &h);
    float norm_x = ((float)mouse_x / (float)w) * 2.0f - 1.0f;
    float norm_y = (1.0f - (float)mouse_y / (float)h) * 2.0f - 1.0f;
    am_find_window((am_native_window*)sdl_window)->push(L);
    lua_pushnumber(L, norm_x);
    lua_pushnumber(L, norm_y);
    run_loop = am_call_amulet(L, "_mouse_move", 3, 0);
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
                        am_find_window((am_native_window*)sdl_window)->push(eng->L);
                        lua_pushstring(eng->L, am_key_name(key));
                        run_loop = am_call_amulet(eng->L, "_key_down", 2, 0);
                    }
                }
                break;
            }
            case SDL_KEYUP: {
                if (!event.key.repeat) {
                    am_key key = convert_key(event.key.keysym.sym);
                    am_find_window((am_native_window*)sdl_window)->push(eng->L);
                    lua_pushstring(eng->L, am_key_name(key));
                    run_loop = am_call_amulet(eng->L, "_key_up", 2, 0);
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                mouse_x += event.motion.xrel;
                mouse_y += event.motion.yrel;
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                if (event.button.which != SDL_TOUCH_MOUSEID) {
                    am_find_window((am_native_window*)sdl_window)->push(eng->L);
                    lua_pushstring(eng->L, am_mouse_button_name(convert_mouse_button(event.button.button)));
                    run_loop = am_call_amulet(eng->L, "_mouse_down", 2, 0);
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                if (event.button.which != SDL_TOUCH_MOUSEID) {
                    am_find_window((am_native_window*)sdl_window)->push(eng->L);
                    lua_pushstring(eng->L, am_mouse_button_name(convert_mouse_button(event.button.button)));
                    run_loop = am_call_amulet(eng->L, "_mouse_up", 2, 0);
                }
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
        );
    }
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
    EM_ASM(
        window.amulet.load_progress = 100;
    );
    open_package();
    EM_ASM(
        window.amulet.start();
    );
    start();
}

static void on_load_package_error(unsigned int x, void *arg, int http_status) {
    am_log0("error loading data (HTTP status: %d)", http_status);
}

static void on_load_package_progress(unsigned int x, void *arg, int perc) {
    EM_ASM_INT({
        window.amulet.load_progress = $0;
    }, perc);
}

static void load_package() {
    emscripten_async_wget2("data.pak", "data.pak", "GET", "", NULL,
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

void *am_read_resource(const char *filename, int *len, char **errmsg) {
    return am_read_package_resource(package, filename, len, errmsg);
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

extern "C" {

void am_emscripten_run(const char *script) {
    am_destroy_engine(eng);
    eng = am_init_engine(false, 0, NULL);
    if (am_run_script(eng->L, script, "main.lua")) {
        run_loop = true;
    } else {
        run_loop = false;
    }
}

void am_emscripten_resize(int w, int h) {
    if (sdl_window != NULL) {
        //am_handle_window_resize((am_native_window*)sdl_window, w, h);
    }
}

}

#endif // AM_BACKEND_EMSCRIPTEN
