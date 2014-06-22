#include "amulet.h"

struct am_window {
    bool            live;
    SDL_Window     *sdl_win;
    SDL_GLContext   gl_context;
    am_render_state *rstate;
};

static int create_window(lua_State *L) {
    am_check_nargs(L, 3);
    const char *title = lua_tostring(L, 1);
    int w = luaL_checkinteger(L, 2);
    int h = luaL_checkinteger(L, 3);
    if (title == NULL) {
        return luaL_error(L, "expecting a string in argument 1");
    }
    am_window *win = new (lua_newuserdata(L, sizeof(am_window))) am_window();
    win->sdl_win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        w, h,
        SDL_WINDOW_OPENGL
    );

    win->gl_context = SDL_GL_CreateContext(win->sdl_win);
    win->rstate = new am_render_state();
    win->live = true;

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

static int destroy_window(lua_State *L) {
    am_check_nargs(L, 1);
    am_window *win = (am_window*)am_check_metatable_id(L, AM_MT_WINDOW, 1);
    if (!win->live) {
        return luaL_error(L, "attempt to destroy a window twice");
    }
    SDL_GL_DeleteContext(win->gl_context);
    SDL_DestroyWindow(win->sdl_win);
    win->sdl_win = NULL;
    delete win->rstate;
    win->rstate = NULL;
    win->live = false;
    return 0;
}

static int swap_window(lua_State *L) {
    am_check_nargs(L, 1);
    am_window *win = (am_window*)am_check_metatable_id(L, AM_MT_WINDOW, 1);
    if (!win->live) {
        return luaL_error(L, "attempt to use a destroyed window");
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
    if (win->live) {
        return destroy_window(L);
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
    lua_pushcclosure(L, destroy_window, 0);
    lua_setfield(L, -2, "destroy");
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
}
