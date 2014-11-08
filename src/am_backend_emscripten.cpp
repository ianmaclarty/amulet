#include "amulet.h"

#ifdef AM_BACKEND_EMSCRIPTEN

#include "emscripten.h"
#include "SDL/SDL.h"

static SDL_Surface* sdl_window;

static const char *filename = "main.lua";

static void init_sdl();
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
    int msaa_samples)
{
    if (sdl_window != NULL) return NULL;
    /*
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
    */
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
    sdl_window = SDL_SetVideoMode(width, height, 16, flags);
    if (sdl_window == NULL) {
        return NULL;
    }
    am_gl_initialized = true;
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
        am_debug("fps = %0.2f", fps);
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

/*
quit:
    if (L != NULL) am_destroy_engine(L);
    SDL_Quit();
    return 0;
*/
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
    SDL_Init(SDL_INIT_VIDEO);
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
        }
    }
    return true;
}

#endif // AM_BACKEND_EMSCRIPTEN
