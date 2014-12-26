#include "amulet.h"

struct am_window : am_nonatomic_userdata {
    bool                needs_closing;
    am_native_window   *native_win;
    am_render_state    *rstate;
    am_scene_node      *root;
    int                 root_ref;
    int                 window_ref;
    int                 drawable_width;
    int                 drawable_height;
};

static std::vector<am_window*> windows;

static int create_window(lua_State *L) {
    am_check_nargs(L, 1);
    if (!lua_istable(L, 1)) return luaL_error(L, "expecting a table in position 1");
    am_window_mode mode = AM_WINDOW_MODE_WINDOWED;
    int top = -1;
    int left = -1;
    int width = 640;
    int height = 480;
    const char *title = "Untitled";
    bool resizable = false;
    bool borderless = false;
    bool depth_buffer = true;
    bool stencil_buffer = true;
    int msaa_samples = 0;

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        const char *key = luaL_checkstring(L, -2);
        if (strcmp(key, "mode") == 0) {
            const char *value = luaL_checkstring(L, -1);
            if (strcmp(value, "windowed") == 0) {
                mode = AM_WINDOW_MODE_WINDOWED;
            } else if (strcmp(value, "fullscreen") == 0) {
                mode = AM_WINDOW_MODE_FULLSCREEN;
            } else if (strcmp(value, "desktop") == 0) {
                mode = AM_WINDOW_MODE_FULLSCREEN_DESKTOP;
            } else {
                return luaL_error(L, "unknown window mode: '%s'", value);
            }
        } else if (strcmp(key, "top") == 0) {
            top = luaL_checkinteger(L, -1);
        } else if (strcmp(key, "left") == 0) {
            left = luaL_checkinteger(L, -1);
        } else if (strcmp(key, "width") == 0) {
            width = luaL_checkinteger(L, -1);
            if (width <= 0) {
                return luaL_error(L, "width must be positive");
            }
        } else if (strcmp(key, "height") == 0) {
            height = luaL_checkinteger(L, -1);
            if (height <= 0) {
                return luaL_error(L, "height must be positive");
            }
        } else if (strcmp(key, "title") == 0) {
            title = luaL_checkstring(L, -1);
        } else if (strcmp(key, "resizable") == 0) {
            resizable = lua_toboolean(L, -1);
        } else if (strcmp(key, "borderless") == 0) {
            borderless = lua_toboolean(L, -1);
        } else if (strcmp(key, "depth_buffer") == 0) {
            depth_buffer = lua_toboolean(L, -1);
        } else if (strcmp(key, "stencil_buffer") == 0) {
            stencil_buffer = lua_toboolean(L, -1);
        } else if (strcmp(key, "msaa_samples") == 0) {
            msaa_samples = luaL_checkinteger(L, -1);
            if (msaa_samples < 0) {
                return luaL_error(L, "msaa_samples can't be negative");
            }
        } else {
            return luaL_error(L, "unrecognised window setting: '%s'");
        }
        lua_pop(L, 1); // pop value
    }
    am_window *win = am_new_userdata(L, am_window);
    win->needs_closing = false;
    win->native_win = am_create_native_window(
        mode,
        top, left,
        width, height,
        title,
        resizable,
        borderless,
        depth_buffer,
        stencil_buffer,
        msaa_samples,
        &win->drawable_width,
        &win->drawable_height);
    if (win->native_win == NULL) {
        return luaL_error(L, "unable to create native window");
    }
    win->rstate = new am_render_state();
    win->rstate->viewport_state.set(0, 0, win->drawable_width, win->drawable_height);

    win->root = NULL;
    win->root_ref = LUA_NOREF;

    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);
    lua_pushvalue(L, -2);
    win->window_ref = luaL_ref(L, -2);
    lua_pop(L, 1); // window table

    windows.push_back(win);

    return 1;
}

static am_window* find_window(am_native_window *nwin) {
    for (unsigned int i = 0; i < windows.size(); i++) {
        am_window *win = windows[i];
        if (win->native_win == nwin) {
            return win;
        }
    }
    return NULL;
}

void am_handle_window_close(am_native_window *nwin) {
    am_window *win = find_window(nwin);
    if (win != NULL) {
        win->needs_closing = true;
    }
}

void am_handle_window_resize(am_native_window *nwin, int w, int h) {
    am_window *win = find_window(nwin);
    if (win != NULL) {
        win->drawable_width = w;
        win->drawable_height = h;
        win->rstate->viewport_state.set(0, 0, w, h);
    }
}

static int close_window(lua_State *L) {
    am_check_nargs(L, 1);
    am_window *win = am_get_userdata(L, am_window, 1);
    if (win->needs_closing) {
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
            am_destroy_native_window(win->native_win);
            win->native_win = NULL;
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

void am_destroy_windows(lua_State *L) {
    for (unsigned int i = 0; i < windows.size(); i++) {
        am_window *win = windows[i];
        win->needs_closing = true;
    }
    close_windows(L);
}

static void draw_windows() {
    for (unsigned int i = 0; i < windows.size(); i++) {
        am_window *win = windows[i];
        if (!win->needs_closing && win->root != NULL) {
            am_native_window_pre_render(win->native_win);
            win->root->render(win->rstate);
            am_native_window_post_render(win->native_win);
        }
    }
}

bool am_update_windows(lua_State *L) {
    close_windows(L);
    draw_windows();
    return windows.size() > 0;
}

static void get_root_node(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    window->pushref(L, window->root_ref);
}

static void set_root_node(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    if (lua_isnil(L, 3)) {
        if (window->root == NULL) return;
        window->unref(L, window->root_ref);
        window->root_ref = LUA_NOREF;
        window->root->deactivate_root();
        window->root = NULL;
        return;
    }
    if (window->root != NULL) window->root->deactivate_root();
    window->root = am_get_userdata(L, am_scene_node, 3);
    window->root->activate_root(L);
    if (window->root_ref == LUA_NOREF) {
        window->root_ref = window->ref(L, 3);
    } else {
        window->reref(L, window->root_ref, 3);
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

    am_register_metatable(L, MT_am_window, 0);
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
