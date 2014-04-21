#include "amulet.h"

static SDL_Window *sdl_win = NULL;
SDL_GLContext gl_context;

/* Lua bindings */

static int l_create_window(lua_State *L) {
    int done = 0;
    const char *title = lua_tostring(L, 1);
    int w = luaL_checkinteger(L, 2);
    int h = luaL_checkinteger(L, 3);
    if (title == NULL) {
        return luaL_error(L, "Expecting a string in argument 1");
    }
    if (!lua_isfunction(L, 4)) {
        return luaL_error(L, "Expecting a function in argument 4");
    }
    if (sdl_win != NULL) {
        return luaL_error(L, "Window already created");
    }
    sdl_win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        w, h,
        SDL_WINDOW_OPENGL
    );

    gl_context = SDL_GL_CreateContext(sdl_win);

    /* Use late swap tearing if supported, otherwise normal vsync */
    if (SDL_GL_SetSwapInterval(-1) == -1) {
        if (SDL_GL_SetSwapInterval(1) == -1) {
            /* setting vsync not supported on this system. XXX use sleeps */
        }
    }

    /* main loop */
    while (!done) {
        lua_pushvalue(L, 4);
        lua_call(L, 0, 1);
        done = lua_toboolean(L, -1);
        lua_pop(L, 1);
        SDL_GL_SwapWindow(sdl_win);
    }

    SDL_Delay(3000);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(sdl_win);
    sdl_win = NULL;
}

void am_lua_window_module_setup(lua_State *L) {
    luaL_Reg funcs[] = {
        {"create_window",    l_create_window},
        {NULL, NULL}
    };
    am_lua_load_module(L, "am", funcs);
}
