#include "amulet.h"

struct am_window {
    bool                open;
    bool                needs_closing;
    SDL_Window         *sdl_win;
    SDL_GLContext       gl_context;
    am_render_state    *rstate;
    am_node            *root;
    int                 root_ref;
    int                 window_ref;
};

static std::vector<am_window*> windows;

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
    if (windows.size() > 0) {
        SDL_GL_MakeCurrent(windows[0]->sdl_win, windows[0]->gl_context);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    }
    win->sdl_win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        w, h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
    );
    win->gl_context = SDL_GL_CreateContext(win->sdl_win);
    SDL_GL_MakeCurrent(win->sdl_win, win->gl_context);
    win->rstate = new am_render_state();

    win->root = NULL;
    win->root_ref = LUA_NOREF;

    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);
    lua_pushvalue(L, -1);
    win->window_ref = luaL_ref(L, -2);
    lua_pop(L, 1); // window table

    windows.push_back(win);

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
    if (!win->open || win->needs_closing) {
        return 0; // window already closed
    }
    win->needs_closing = true;

    return 0;
}

static void close_windows(lua_State *L) {
    unsigned int i = 0;
    while (i < windows.size()) {
        am_window *win = windows[i];
        if (win->needs_closing) {
            assert(win->open);
            if (windows.size() == 1) {
                // last window, destroy context
                SDL_GL_DeleteContext(win->gl_context);
            }
            SDL_DestroyWindow(win->sdl_win);
            win->sdl_win = NULL;
            delete win->rstate;
            win->rstate = NULL;
            win->needs_closing = false;
            lua_rawgeti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);
            luaL_unref(L, -1, win->window_ref);
            lua_pop(L, 1); // window table
            win->window_ref = LUA_NOREF;
            windows.erase(windows.begin() + i);
            continue;
        }
        i++;
    }
}

static void draw_windows() {
    for (unsigned int i = 0; i < windows.size(); i++) {
        am_window *win = windows[i];
        if (win->open && win->root != NULL) {
            SDL_GL_MakeCurrent(win->sdl_win, win->gl_context);
            win->root->render(win->rstate);
            SDL_GL_SwapWindow(win->sdl_win);
        }
    }
}

static void handle_events(lua_State *L) {
    am_call_amulet(L, "_clear_keys", 0, 0);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                // close all windows and stop processing any more events
                for (unsigned int i = 0; i < windows.size(); i++) {
                    windows[i]->needs_closing = true;
                }
                close_windows(L);
                break;
            }
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE: {
                        for (unsigned int i = 0; i < windows.size(); i++) {
                            if (SDL_GetWindowID(windows[i]->sdl_win) == event.window.windowID) {
                                if (windows[i]->open) {
                                    windows[i]->needs_closing = true;
                                    close_windows(L);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
                break;
            }
            case SDL_KEYDOWN: {
                if (!event.key.repeat) {
                    const char *key = SDL_GetKeyName(event.key.keysym.sym);
                    lua_pushstring(L, key);
                    am_call_amulet(L, "_key_down", 1, 0);
                }
                break;
            }
            case SDL_KEYUP: {
                if (!event.key.repeat) {
                    const char *key = SDL_GetKeyName(event.key.keysym.sym);
                    lua_pushstring(L, key);
                    am_call_amulet(L, "_key_up", 1, 0);
                }
                break;
            }
        }
    }
}

bool am_update_windows(lua_State *L) {
    close_windows(L);
    draw_windows();
    handle_events(L);
    return windows.size() > 0;
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
        window->root->deactivate_root();
        window->root = NULL;
        return;
    }
    if (window->root != NULL) window->root->deactivate_root();
    window->root = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 3);
    window->root->activate_root();
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
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_window_mt(L);

    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);

    windows.clear();
}

bool am_gl_context_exists() {
    return windows.size() > 0;
}
