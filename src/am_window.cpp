#include "amulet.h"

#define DEFAULT_WIN_WIDTH 640
#define DEFAULT_WIN_HEIGHT 480

struct am_window : am_nonatomic_userdata {
    bool                needs_closing;
    am_native_window   *native_win;
    am_scene_node      *root;
    int                 root_ref;
    int                 window_ref;
    int                 width;
    int                 height;
    // used when restoring windowed mode from fullscreen mode
    int                 restore_windowed_width;
    int                 restore_windowed_height;
    bool                has_depth_buffer;
    bool                has_stencil_buffer;
    bool                lock_pointer;
    am_window_mode      mode;
    bool                dirty;
    am_framebuffer_id   framebuffer;
};

static std::vector<am_window*> windows;

static int create_window(lua_State *L) {
    am_check_nargs(L, 1);
    if (!lua_istable(L, 1)) return luaL_error(L, "expecting a table in position 1");
    am_window_mode mode = AM_WINDOW_MODE_WINDOWED;
    am_display_orientation orientation = AM_DISPLAY_ORIENTATION_ANY;
    int top = -1;
    int left = -1;
    int width = DEFAULT_WIN_WIDTH;
    int height = DEFAULT_WIN_HEIGHT;
    const char *title = "Untitled";
    bool resizable = true;
    bool borderless = false;
    bool depth_buffer = false;
    bool stencil_buffer = false;
    bool lock_pointer = false;
    int msaa_samples = 0;

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        const char *key = luaL_checkstring(L, -2);
        if (strcmp(key, "mode") == 0) {
            mode = am_get_enum(L, am_window_mode, -1);
        } else if (strcmp(key, "orientation") == 0) {
            orientation = am_get_enum(L, am_display_orientation, -1);
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
        } else if (strcmp(key, "lock_pointer") == 0) {
            lock_pointer = lua_toboolean(L, -1);
        } else if (strcmp(key, "msaa_samples") == 0) {
            msaa_samples = luaL_checkinteger(L, -1);
            if (msaa_samples < 0) {
                return luaL_error(L, "msaa_samples can't be negative");
            }
        } else {
            return luaL_error(L, "unrecognised window setting: '%s'", key);
        }
        lua_pop(L, 1); // pop value
    }
    am_window *win = am_new_userdata(L, am_window);
    win->needs_closing = false;
    win->native_win = am_create_native_window(
        mode,
        orientation,
        top, left,
        width, height,
        title,
        resizable,
        borderless,
        depth_buffer,
        stencil_buffer,
        msaa_samples,
        &win->framebuffer);
    am_get_native_window_size(win->native_win, &win->width, &win->height);
    if (win->native_win == NULL) {
        return luaL_error(L, "unable to create native window");
    }

    win->root = NULL;
    win->root_ref = LUA_NOREF;

    win->has_depth_buffer = depth_buffer;
    win->has_stencil_buffer = stencil_buffer;

    win->lock_pointer = lock_pointer;
    am_set_native_window_lock_pointer(win->native_win, lock_pointer);
    win->mode = mode;
    win->dirty = false;

    win->restore_windowed_width = -1;
    win->restore_windowed_height = -1;

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

static int close_window(lua_State *L) {
#if defined(AM_BACKEND_EMSCRIPTEN)
    // closing the window only stops the program running, which
    // just looks like it's crashed to the user.
    return 0;
#else
    am_check_nargs(L, 1);
    am_window *win = am_get_userdata(L, am_window, 1);
    if (win->needs_closing) {
        return 0; // window already closed
    }
    win->needs_closing = true;
    return 0;
#endif
}

static void close_windows(lua_State *L) {
    unsigned int i = 0;
    while (i < windows.size()) {
        am_window *win = windows[i];
        if (win->needs_closing) {
            am_destroy_native_window(win->native_win);
            win->native_win = NULL;
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

static void update_window_sizes() {
    for (unsigned int i = 0; i < windows.size(); i++) {
        am_window *win = windows[i];
        if (win->dirty) {
            int w, h;
            if (win->mode == AM_WINDOW_MODE_WINDOWED && win->restore_windowed_height > 0 && win->restore_windowed_width > 0) {
                w = win->restore_windowed_width;
                h = win->restore_windowed_height;
                win->restore_windowed_width = -1;
                win->restore_windowed_height = -1;
            } else {
                w = win->width;
                h = win->height;
            }
            //am_debug("resized window to %dx%d", w, h);
            am_set_native_window_size_and_mode(win->native_win, w, h, win->mode);
            win->dirty = false;
        }
    }
}

static void draw_windows() {
    for (unsigned int i = 0; i < windows.size(); i++) {
        am_window *win = windows[i];
        if (!win->needs_closing && win->root != NULL) {
            am_native_window_pre_render(win->native_win);
            am_render_state *rstate = &am_global_render_state;
            am_get_native_window_size(win->native_win, &win->width, &win->height);
            rstate->setup(win->framebuffer, true, win->width, win->height, win->has_depth_buffer);
            win->root->render(rstate);
            am_native_window_post_render(win->native_win);
        }
    }
}

bool am_update_windows(lua_State *L) {
    static unsigned int frame = 0;
    close_windows(L);
    update_window_sizes();
    draw_windows();
    frame++;
    if (am_conf_log_gl_calls) {
        am_debug("GL: ===================== END FRAME %d ==========================", frame);
    }
    return windows.size() > 0;
}

bool am_execute_actions(lua_State *L, double dt) {
    am_prepare_to_execute_actions(L, dt);
    unsigned int n = windows.size();
    for (unsigned int i = 0; i < n; i++) {
        am_window *win = windows[i];
        if (!win->needs_closing && win->root != NULL) {
            if (!am_execute_node_actions(L, win->root)) {
                return false;
            }
        }
    }
    return true;
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
        window->root = NULL;
        return;
    }
    window->root = am_get_userdata(L, am_scene_node, 3);
    if (window->root_ref == LUA_NOREF) {
        window->root_ref = window->ref(L, 3);
    } else {
        window->reref(L, window->root_ref, 3);
    }
}

static am_property root_property = {get_root_node, set_root_node};

static void get_window_width(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->width);
}

static void set_window_width(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    int w = luaL_checkinteger(L, 3);
    if (w > 0) {
        window->width = w;
        window->dirty = true;
    }
}

static void get_window_height(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->height);
}

static void set_window_height(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    int h = luaL_checkinteger(L, 3);
    if (h > 0) {
        window->height = h;
        window->dirty = true;
    }
}

static am_property width_property = {get_window_width, set_window_width};
static am_property height_property = {get_window_height, set_window_height};

static void get_lock_pointer(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushboolean(L, window->lock_pointer);
}

static void set_lock_pointer(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    bool value = lua_toboolean(L, 3);
    am_set_native_window_lock_pointer(window->native_win, value);
    window->lock_pointer = value;
}

static am_property lock_pointer_property = {get_lock_pointer, set_lock_pointer};

static void get_window_mode(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    am_push_enum(L, am_window_mode, window->mode);
}

static void set_window_mode(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    am_window_mode old_mode = window->mode;
    window->mode = am_get_enum(L, am_window_mode, 3);
    if (old_mode == AM_WINDOW_MODE_WINDOWED && window->mode != old_mode) {
        window->restore_windowed_width = window->width;
        window->restore_windowed_height = window->height;
    }
    window->dirty = true;
}

static am_property window_mode_property = {get_window_mode, set_window_mode};

static void register_window_mt(lua_State *L) {
    lua_newtable(L);

    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    am_register_property(L, "root", &root_property);
    am_register_property(L, "lock_pointer", &lock_pointer_property);
    am_register_property(L, "width", &width_property);
    am_register_property(L, "height", &height_property);
    am_register_property(L, "mode", &window_mode_property);

    lua_pushcclosure(L, close_window, 0);
    lua_setfield(L, -2, "close");

    am_register_metatable(L, "window", MT_am_window, 0);
}

void am_open_window_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"window",    create_window},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_window_mt(L);

    am_enum_value window_mode_enum[] = {
        {"windowed",        AM_WINDOW_MODE_WINDOWED},
        {"fullscreen",      AM_WINDOW_MODE_FULLSCREEN},
        {"desktop",         AM_WINDOW_MODE_FULLSCREEN_DESKTOP},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_window_mode, window_mode_enum);

    am_enum_value orientation_enum[] = {
        {"portrait",    AM_DISPLAY_ORIENTATION_PORTRAIT},
        {"landscape",   AM_DISPLAY_ORIENTATION_LANDSCAPE},
        {"any",         AM_DISPLAY_ORIENTATION_ANY},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_display_orientation, orientation_enum);

    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);

    windows.clear();
}
