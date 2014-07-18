#include "amulet.h"

struct am_window {
    bool                is_main;
    bool                open;
    SDL_Window         *sdl_win;
    SDL_GLContext       gl_context;
    am_render_state    *rstate;
    am_node            *root;
    int                 root_ref;
    int                 window_ref;
};

static am_window *main_window = NULL;

static int create_window(lua_State *L) {
    am_check_nargs(L, 3);
    const char *title = lua_tostring(L, 1);
    int w = luaL_checkinteger(L, 2);
    int h = luaL_checkinteger(L, 3);
    if (title == NULL) {
        return luaL_error(L, "expecting a string in argument 1");
    }
    am_window *win = new (am_new_nonatomic_userdata(L, sizeof(am_window))) am_window();
    win->open = true;
    if (main_window != NULL) {
        SDL_GL_MakeCurrent(main_window->sdl_win, main_window->gl_context);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    }
    win->sdl_win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        w, h,
        SDL_WINDOW_OPENGL
    );
    win->gl_context = SDL_GL_CreateContext(win->sdl_win);
    win->rstate = new am_render_state();
    if (main_window == NULL) {
        main_window = win;
        win->is_main = true;
    } else {
        win->is_main = false;
    }

    win->root = NULL;
    win->root_ref = LUA_NOREF;

    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);
    lua_pushvalue(L, -1);
    win->window_ref = luaL_ref(L, -2);
    lua_pop(L, 1); // window table

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
    if (win->is_main) {
        SDL_GL_DeleteContext(win->gl_context);
        main_window = NULL;
    }
    SDL_DestroyWindow(win->sdl_win);
    win->sdl_win = NULL;
    delete win->rstate;
    win->rstate = NULL;
    win->open = false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);
    luaL_unref(L, -1, win->window_ref);
    lua_pop(L, 1); // window table
    win->window_ref = LUA_NOREF;

    return 0;
}

static int swap_window(lua_State *L) {
    am_check_nargs(L, 1);
    am_window *win = (am_window*)am_check_metatable_id(L, AM_MT_WINDOW, 1);
    if (!win->open) {
        return luaL_error(L, "attempt to swap a closed window");
    }
    if (main_window == NULL) {
        return luaL_error(L, "can't swap a after main window closed");
    }
    SDL_GL_SwapWindow(win->sdl_win);
    return 0;
}

static int draw_node(lua_State *L) {
    if (main_window == NULL) {
        return luaL_error(L, "can't draw after main window closed");
    }
    am_check_nargs(L, 2);
    am_window *win = (am_window*)am_check_metatable_id(L, AM_MT_WINDOW, 1);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    SDL_GL_MakeCurrent(win->sdl_win, win->gl_context);
    node->render(win->rstate);
    return 0;
}

static void get_root_node(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    am_push_ref(L, 1, window->root_ref);
}

static void set_root_node(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    if (lua_isnil(L, 3)) {
        if (window->root == NULL) return;
        am_delete_ref(L, 1, window->root_ref);
        window->root->root_count--;
        window->root = NULL;
        return;
    }
    if (window->root != NULL) window->root->root_count--;
    window->root = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 3);
    window->root->root_count++;
    if (window->root_ref == LUA_NOREF) {
        window->root_ref = am_new_ref(L, 1, 3);
    } else {
        am_replace_ref(L, 1, window->root_ref, 3);
    }
}

static am_property root_property = {get_root_node, set_root_node};

static void register_window_mt(lua_State *L) {
    lua_newtable(L);

    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    am_register_property(L, "root", &root_property);

    lua_pushcclosure(L, draw_node, 0);
    lua_setfield(L, -2, "draw");
    lua_pushcclosure(L, swap_window, 0);
    lua_setfield(L, -2, "swap");
    lua_pushcclosure(L, close_window, 0);
    lua_setfield(L, -2, "close");

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

    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);
}

bool am_gl_context_exists() {
    return main_window != NULL;
}
