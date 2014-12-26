#include "amulet.h"

#ifdef AM_BACKEND_EMSCRIPTEN

#include "emscripten.h"
#include "SDL/SDL.h"

static SDL_Surface* sdl_window;

static const char *filename = "main.lua";

static void init_sdl();
static void init_audio();
static bool handle_events();
static int report_status(lua_State *L, int status);

am_native_window *am_create_native_window(
    am_window_mode mode,
    int top, int left,
    int width, int height,
    const char *title,
    bool resizable,
    bool borderless,
    bool depth_buffer,
    bool stencil_buffer,
    int msaa_samples,
    int *drawable_width, int *drawable_height)
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
    *drawable_width = sdl_window->w;
    *drawable_height = sdl_window->h;
    return (am_native_window*)sdl_window;
}

void am_destroy_native_window(am_native_window* win) {
    sdl_window = NULL;
}

void am_native_window_pre_render(am_native_window* win) {
}

void am_native_window_post_render(am_native_window* win) {
    SDL_GL_SwapBuffers();
}

double am_get_time() {
    return ((double)SDL_GetTicks())/1000.0;
}

static double fps = 1.0;

static lua_State *L = NULL;
static double t0;
static double t;
static double t_debt;
static double fps_t0;
static double fps_max;
static double dt;
static long long frame_count = 0;
static bool run_loop = true;

static void main_loop() {
    if (!run_loop || sdl_window == NULL) return;

    if (!am_update_windows(L)) {
        run_loop = false;
        return;
    }

    am_sync_audio_graph(L);

    if (!handle_events()) {
        run_loop = false;
        return;
    }

    t = am_get_time();
    dt = fmin(1.0/15.0, t - t0); // fmin in case process was suspended, or last frame took very long
    t_debt += dt;

    if (am_conf_fixed_update_time > 0.0) {
        while (t_debt > 0.0) {
            if (!am_execute_actions(L, am_conf_fixed_update_time)) {
                run_loop = false;
                return;
            }
            t_debt -= am_conf_fixed_update_time;
        }
    } else {
        if (!am_execute_actions(L, t_debt)) {
            run_loop = false;
            return;
        }
        t_debt = 0.0;
    }

    fps_max = fmax(fps_max, dt);
    t0 = t;

    frame_count++;

    if (t - fps_t0 >= 2.0) {
        fps = (double)frame_count / (t - fps_t0);
        //am_debug("fps = %0.2f", fps);
        fps_t0 = t0;
        fps_max = 0.0;
        frame_count = 0;
    }
    SDL_GL_SwapBuffers();
}

int main( int argc, char *argv[] )
{
    int status;

    init_sdl();

    L = am_init_engine(false);
    if (L == NULL) return 1;

    status = luaL_dofile(L, filename);
    if (report_status(L, status)) return 1;
    if (sdl_window == NULL) return 1;

    t0 = am_get_time();
    t = t0;
    t_debt = 0.0;
    fps_t0 = 0.0;
    fps_max = 0.0;
    frame_count = 0;

    emscripten_set_main_loop(main_loop, 0, 0);
    return 0;
}

static int report_status(lua_State *L, int status) {
    if (status && !lua_isnil(L, -1)) {
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL) msg = "(error object is not a string)";
        am_report_error("%s\n", msg);
        lua_pop(L, 1);
    }
    return status;
}

static void init_sdl() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    init_audio();
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
                /*
                if (!event.key.repeat) {
                    const char *key = SDL_GetKeyName(event.key.keysym.sym);
                    lua_pushstring(L, key);
                    am_call_amulet(L, "_key_down", 1, 0);
                }
                */
                break;
            }
            case SDL_KEYUP: {
                /*
                if (!event.key.repeat) {
                    const char *key = SDL_GetKeyName(event.key.keysym.sym);
                    lua_pushstring(L, key);
                    am_call_amulet(L, "_key_up", 1, 0);
                }
                */
                break;
            }
            case SDL_VIDEORESIZE: {
                am_debug("resize w = %d", event.resize.w);
                break;
            }
        }
    }
    return true;
}

static float *audio_buffer = NULL;

static void audio_callback(void *ud, Uint8 *stream, int len) {
    static int count = 0;
    /*
    if (count > 0) {
        memset(stream, 0, len);
        return;
    }
    */
    count++;
    assert(audio_buffer != NULL);
    if (len != (int)sizeof(int16_t) * am_conf_audio_buffer_size * am_conf_audio_channels) {
        am_debug("audio buffer size mismatch! (%d vs %d)",
            len, 
            (int)sizeof(int16_t) * am_conf_audio_buffer_size * am_conf_audio_channels);
        memset(stream, 0, len);
        return;
    }
    int num_channels = am_conf_audio_channels;
    int num_samples = am_conf_audio_buffer_size;
    float *buffer = audio_buffer;
    memset(buffer, 0, sizeof(float) * am_conf_audio_buffer_size * am_conf_audio_channels);
    am_audio_bus bus;
    bus.num_channels = num_channels;
    bus.num_samples = num_samples;
    bus.buffer = buffer;
    for (int i = 0; i < num_channels; i++) {
        bus.channel_data[i] = bus.buffer + i * num_samples;
    }
    am_fill_audio_bus(&bus);
    // SDL expects channels to be interleaved
    //am_debug("--------------------------------------------");
    //am_debug("num_samples = %d", num_samples);
    for (int c = 0; c < num_channels; c++) {
        for (int i = 0; i < num_samples; i++) {
            ((int16_t*)stream)[i * num_channels + c] = (int16_t)(bus.channel_data[c][i] * 32767.0f);
            //if (c == 0) am_debug("* %d", ((int16_t*)stream)[i * num_channels + c]);
        }
    }
}

static void init_audio() {
    audio_buffer = (float*)malloc(sizeof(float) * am_conf_audio_channels * am_conf_audio_buffer_size);
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

extern "C" {

void am_emscripten_run(const char *script) {
    am_destroy_engine(L);
    L = am_init_engine(false);
    if (am_run_script(L, script, "main.lua")) {
        run_loop = true;
    } else {
        run_loop = false;
    }
}

void am_emscripten_resize(int w, int h) {
    if (sdl_window != NULL) {
        am_handle_window_resize((am_native_window*)sdl_window, w, h);
    }
}

}

#endif // AM_BACKEND_EMSCRIPTEN
