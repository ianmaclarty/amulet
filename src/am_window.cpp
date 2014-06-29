#include "amulet.h"

#define AM_MAX_WINDOWS 32

struct am_window {
    int                 window_id;
    bool                open;
    SDL_Window         *sdl_win;
    SDL_GLContext       gl_context;
    am_render_state    *rstate;
};

static am_window *open_windows[AM_MAX_WINDOWS];

static int create_window(lua_State *L) {
    // find free window id
    int window_id;
    for (window_id = 0; window_id < AM_MAX_WINDOWS; window_id++) {
        if (open_windows[window_id] == NULL) {
            goto found_window_id;
        }
    }
    return luaL_error(L, "sorry, too many open windows (at most %d allowed)", AM_MAX_WINDOWS);
found_window_id:
    am_check_nargs(L, 3);
    const char *title = lua_tostring(L, 1);
    int w = luaL_checkinteger(L, 2);
    int h = luaL_checkinteger(L, 3);
    if (title == NULL) {
        return luaL_error(L, "expecting a string in argument 1");
    }
    am_window *win = new (lua_newuserdata(L, sizeof(am_window))) am_window();
    open_windows[window_id] = win;
    win->window_id = window_id;
    win->open = true;
    win->sdl_win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        w, h,
        SDL_WINDOW_OPENGL
    );
    win->gl_context = SDL_GL_CreateContext(win->sdl_win);
    win->rstate = new am_render_state();

    /* Use late swap tearing if supported, otherwise normal vsync */
    if (SDL_GL_SetSwapInterval(-1) == -1) {
        if (SDL_GL_SetSwapInterval(1) == -1) {
            /* setting vsync not supported on this system. XXX use sleeps */
        }
    }

    if (!am_init_gl()) {
        lua_pop(L, 1);
        return 0;
    }

    am_set_metatable(L, AM_MT_WINDOW, -1);

    return 1;
}

static int close_window(lua_State *L) {
    am_check_nargs(L, 1);
    am_window *win = (am_window*)am_check_metatable_id(L, AM_MT_WINDOW, 1);
    if (!win->open) {
        return 0; // window already closed
    }
    SDL_GL_DeleteContext(win->gl_context);
    SDL_DestroyWindow(win->sdl_win);
    win->sdl_win = NULL;
    delete win->rstate;
    win->rstate = NULL;
    win->open = false;
    open_windows[win->window_id] = NULL;
    return 0;
}

static int swap_window(lua_State *L) {
    am_check_nargs(L, 1);
    am_window *win = (am_window*)am_check_metatable_id(L, AM_MT_WINDOW, 1);
    if (!win->open) {
        return luaL_error(L, "attempt to swap a closed window");
    }
    SDL_GL_SwapWindow(win->sdl_win);
    return 0;
}

static int draw_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_window *win = (am_window*)am_check_metatable_id(L, AM_MT_WINDOW, 1);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    node->render(win->rstate);
    return 0;
}

static int gc_window(lua_State *L) {
    am_window *win = (am_window*)am_check_metatable_id(L, AM_MT_WINDOW, 1);
    if (win->open) {
        return close_window(L);
    }
    return 0;
}

static void register_window_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, draw_node, 0);
    lua_setfield(L, -2, "draw");
    lua_pushcclosure(L, swap_window, 0);
    lua_setfield(L, -2, "swap");
    lua_pushcclosure(L, close_window, 0);
    lua_setfield(L, -2, "close");
    lua_pushcclosure(L, gc_window, 0);
    lua_setfield(L, -2, "__gc");
    lua_pushstring(L, "window");
    lua_setfield(L, -2, "tname");
    am_register_metatable(L, AM_MT_WINDOW, 0);
}

void am_open_window_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"create_window",    create_window},
        {NULL, NULL}
    };
    am_open_module(L, "amulet", funcs);
    register_window_mt(L);
    for (int i = 0; i < AM_MAX_WINDOWS; i++) {
        open_windows[i] = NULL;
    }
}
