#include "amulet.h"

#define DEFAULT_WIN_WIDTH 640
#define DEFAULT_WIN_HEIGHT 480

static std::vector<am_window*> windows;

static void compute_viewport(am_window *win);

static bool update_size(am_window *win);

static int create_window(lua_State *L) {
    am_check_nargs(L, 1);
    if (!lua_istable(L, 1)) return luaL_error(L, "expecting a table in position 1");
    am_window_mode mode = AM_WINDOW_MODE_WINDOWED;
    am_display_orientation orientation = AM_DISPLAY_ORIENTATION_ANY;
    int requested_width = -1;
    int requested_height = -1;
    int user_width = DEFAULT_WIN_WIDTH;
    int user_height = DEFAULT_WIN_HEIGHT;
    const char *title = "Untitled";
    bool resizable = true;
    bool highdpi = false;
    bool borderless = false;
    bool depth_buffer = false;
    bool stencil_buffer = false;
    int stencil_clear_value = 0;
    bool letterbox = true;
    glm::dvec4 clear_color(0.0, 0.0, 0.0, 1.0);
    glm::dmat4 projection;
    bool user_projection = false;
    bool lock_pointer = false;
    bool show_cursor = true;
    int msaa_samples = 0;

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        const char *key = luaL_checkstring(L, -2);
        if (strcmp(key, "mode") == 0) {
            mode = am_get_enum(L, am_window_mode, -1);
        } else if (strcmp(key, "orientation") == 0) {
            orientation = am_get_enum(L, am_display_orientation, -1);
        } else if (strcmp(key, "width") == 0) {
            user_width = luaL_checknumber(L, -1);
            if (user_width <= 0) {
                return luaL_error(L, "width must be positive");
            }
        } else if (strcmp(key, "height") == 0) {
            user_height = luaL_checknumber(L, -1);
            if (user_height <= 0) {
                return luaL_error(L, "height must be positive");
            }
        } else if (strcmp(key, "physical_size") == 0) {
            am_vec2 *size = am_get_userdata(L, am_vec2, -1);
            requested_width = (int)size->v.x;
            requested_height = (int)size->v.y;
        } else if (strcmp(key, "title") == 0) {
            title = luaL_checkstring(L, -1);
        } else if (strcmp(key, "highdpi") == 0) {
            highdpi = lua_toboolean(L, -1);
        } else if (strcmp(key, "resizable") == 0) {
            resizable = lua_toboolean(L, -1);
        } else if (strcmp(key, "borderless") == 0) {
            borderless = lua_toboolean(L, -1);
        } else if (strcmp(key, "depth_buffer") == 0) {
            depth_buffer = lua_toboolean(L, -1);
        } else if (strcmp(key, "stencil_buffer") == 0) {
            stencil_buffer = lua_toboolean(L, -1);
        } else if (strcmp(key, "stencil_clear_value") == 0) {
            stencil_clear_value = luaL_checkinteger(L, -1);
        } else if (strcmp(key, "letterbox") == 0) {
            letterbox = lua_toboolean(L, -1);
        } else if (strcmp(key, "lock_pointer") == 0) {
            lock_pointer = lua_toboolean(L, -1);
        } else if (strcmp(key, "show_cursor") == 0) {
            show_cursor = lua_toboolean(L, -1);
        } else if (strcmp(key, "msaa_samples") == 0) {
            msaa_samples = luaL_checkinteger(L, -1);
            if (msaa_samples < 0) {
                return luaL_error(L, "msaa_samples can't be negative");
            }
#ifdef AM_WINDOWS
            if (am_conf_d3dangle) {
                // msaa not supported by angle driver
                msaa_samples = 0;
            }
#endif
        } else if (strcmp(key, "clear_color") == 0) {
            clear_color = am_get_userdata(L, am_vec4, -1)->v;
        } else if (strcmp(key, "projection") == 0) {
            projection = am_get_userdata(L, am_mat4, -1)->m;
            user_projection = true;
        } else {
            return luaL_error(L, "unrecognised window setting: '%s'", key);
        }
        lua_pop(L, 1); // pop value
    }
    if (requested_width == -1) {
        requested_width = (int)user_width;
        requested_height = (int)user_height;
    }
    am_window *win = am_new_userdata(L, am_window);
    win->needs_closing = false;
    win->requested_width = requested_width;
    win->requested_height = requested_height;
    win->user_width = user_width;
    win->user_height = user_height;
    win->clear_color = clear_color;
    win->stencil_clear_value = stencil_clear_value;
    win->letterbox = letterbox;
    win->native_win = am_create_native_window(
        mode,
        orientation,
        -1, -1,
        requested_width, requested_height,
        title,
        highdpi,
        resizable,
        borderless,
        depth_buffer,
        stencil_buffer,
        msaa_samples);
    if (win->native_win == NULL) {
        return luaL_error(L, "unable to create native window");
    }
    win->pixel_width = -1;
    win->pixel_height = -1;
    win->user_projection = user_projection;
    win->projection = projection;
    update_size(win);

    win->scene = NULL;
    win->scene_ref = LUA_NOREF;
    win->overlay = NULL;
    win->overlay_ref = LUA_NOREF;

    win->has_depth_buffer = depth_buffer;
    win->has_stencil_buffer = stencil_buffer;

    am_set_native_window_lock_pointer(win->native_win, lock_pointer);
    am_set_native_window_show_cursor(win->native_win, show_cursor);
    win->mode = mode;
    win->dirty = false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);
    lua_pushvalue(L, -2);
    win->window_ref = luaL_ref(L, -2);
    lua_pop(L, 1); // window table

    windows.push_back(win);

    win->pushuservalue(L);
    lua_newtable(L);
    lua_setfield(L, -2, "_event_data");
    lua_pop(L, 1); // uservalue

    lua_pushvalue(L, -1);
    am_call_amulet(L, "_init_window_event_data", 1, 0);

    return 1;
}

void am_window::compute_position(double x, double y, double *usr_x, double *usr_y, double *norm_x, double *norm_y, double *px_x, double *px_y) {
    // invert y
    y = (double)screen_height - y;

    // ----- compute pixel x y -----
    double px = x * ((double)pixel_width / (double)screen_width) - (double)viewport_x;
    double py = y * ((double)pixel_height / (double)screen_height) - (double)viewport_y;

    // ----- compute user x y -----
    double ux = px;
    double uy = py;
    // convert to user window space
    if (user_projection) {
        glm::dvec4 p(ux * 2.0 / (double)viewport_width - 1.0,
            uy * 2.0 / (double)viewport_height - 1.0, 0.0, 1.0);
        p = glm::inverse(projection) * p;
        ux = p.x;
        uy = p.y;
    } else {
        // can avoid a matrix inverse and mult in this case
        ux = ux / (double)viewport_width * (user_right - user_left) + user_left;
        uy = uy / (double)viewport_height * (user_top - user_bottom) + user_bottom;
    }

    // ----- compute norm x y -----
    double nx = px / (double)viewport_width * 2.0 - 1.0;
    double ny = py / (double)viewport_height * 2.0 - 1.0;

    *usr_x = ux;
    *usr_y = uy;
    *norm_x = nx;
    *norm_y = ny;
    *px_x = px;
    *px_y = py;
}

void am_window::mouse_move(lua_State *L, double x, double y) {
    double ux, uy, nx, ny, px, py;
    compute_position(x, y, &ux, &uy, &nx, &ny, &px, &py);
    push(L);
    lua_pushnumber(L, ux);
    lua_pushnumber(L, uy);
    lua_pushnumber(L, nx);
    lua_pushnumber(L, ny);
    lua_pushnumber(L, px);
    lua_pushnumber(L, py);
    am_call_amulet(L, "_mouse_move", 7, 0);
}

void am_window::mouse_wheel(lua_State *L, double x, double y) {
    push(L);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    am_call_amulet(L, "_mouse_wheel", 3, 0);
}

void am_window::mouse_down(lua_State *L, am_mouse_button button) {
    push(L);
    lua_pushstring(L, am_mouse_button_name(button));
    am_call_amulet(L, "_mouse_down", 2, 0);
}

void am_window::mouse_up(lua_State *L, am_mouse_button button) {
    push(L);
    lua_pushstring(L, am_mouse_button_name(button));
    am_call_amulet(L, "_mouse_up", 2, 0);
}

void am_window::touch_begin(lua_State *L, void* touchid, double x, double y, double force) {
    double ux, uy, nx, ny, px, py;
    compute_position(x, y, &ux, &uy, &nx, &ny, &px, &py);
    push(L);
    lua_pushlightuserdata(L, touchid);
    lua_pushnumber(L, ux);
    lua_pushnumber(L, uy);
    lua_pushnumber(L, nx);
    lua_pushnumber(L, ny);
    lua_pushnumber(L, px);
    lua_pushnumber(L, py);
    lua_pushnumber(L, force);
    am_call_amulet(L, "_touch_begin", 9, 0);
}

void am_window::touch_end(lua_State *L, void* touchid, double x, double y, double force) {
    double ux, uy, nx, ny, px, py;
    compute_position(x, y, &ux, &uy, &nx, &ny, &px, &py);
    push(L);
    lua_pushlightuserdata(L, touchid);
    lua_pushnumber(L, ux);
    lua_pushnumber(L, uy);
    lua_pushnumber(L, nx);
    lua_pushnumber(L, ny);
    lua_pushnumber(L, px);
    lua_pushnumber(L, py);
    lua_pushnumber(L, force);
    am_call_amulet(L, "_touch_end", 9, 0);
}

void am_window::touch_move(lua_State *L, void* touchid, double x, double y, double force) {
    double ux, uy, nx, ny, px, py;
    compute_position(x, y, &ux, &uy, &nx, &ny, &px, &py);
    push(L);
    lua_pushlightuserdata(L, touchid);
    lua_pushnumber(L, ux);
    lua_pushnumber(L, uy);
    lua_pushnumber(L, nx);
    lua_pushnumber(L, ny);
    lua_pushnumber(L, px);
    lua_pushnumber(L, py);
    lua_pushnumber(L, force);
    am_call_amulet(L, "_touch_move", 9, 0);
}

am_window* am_find_window(am_native_window *nwin) {
    for (unsigned int i = 0; i < windows.size(); i++) {
        am_window *win = windows[i];
        if (win->native_win == nwin) {
            return win;
        }
    }
    return NULL;
}

void am_handle_window_close(am_native_window *nwin) {
    am_window *win = am_find_window(nwin);
    if (win != NULL) {
        win->needs_closing = true;
    }
}

static int close_window(lua_State *L) {
#if defined(AM_BACKEND_EMSCRIPTEN)
    // ignore window close in HTML5 apps
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

static bool close_windows(lua_State *L) {
    int i = (int)windows.size() - 1;
    while (i >= 0) {
        am_window *win = windows[i];
        if (win->needs_closing) {
            if (i == 0) {
                // don't close main window here, because we need to destroy the
                // gl objects first.
                return false;
            } else {
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
        }
        i--;
    }
    return true;
}

static void resize_windows() {
    for (unsigned int i = 0; i < windows.size(); i++) {
        am_window *win = windows[i];
        if (win->dirty) {
            am_set_native_window_size_and_mode(win->native_win, win->requested_width, win->requested_height, win->mode);
            win->dirty = false;
        }
    }
}

static bool update_size(am_window *win) {
    int old_width = win->pixel_width;
    int old_height = win->pixel_height;
    am_get_native_window_size(win->native_win,
        &win->pixel_width, &win->pixel_height,
        &win->screen_width, &win->screen_height);
    if (old_width != win->pixel_width || old_height != win->pixel_height) {
        compute_viewport(win);
        return true;
    }
    return false;
}

static void compute_viewport(am_window *win) {
    double w0 = win->user_width;
    double h0 = win->user_height;
    double w1 = (double)win->pixel_width;
    double h1 = (double)win->pixel_height;
    if (win->letterbox) {
        double sy = h1 / h0;
        double dx = (w1 - w0 * sy) / (2.0 * w1);
        double sx = w1 / w0;
        double dy = (h1 - h0 * sx) / (2.0 * h1);
        if (dx > 0.01) {
            win->viewport_x = (int)(dx * w1);
            win->viewport_y = 0;
            win->viewport_width = win->pixel_width - (int)(dx * w1 * 2.0);
            win->viewport_height = win->pixel_height;
        } else {
            win->viewport_x = 0;
            win->viewport_width = win->pixel_width;
            if (dy > 0.01) {
                win->viewport_y = (int)(dy * h1);
                win->viewport_height = win->pixel_height - (int)(dy * h1) * 2;
            } else {
                win->viewport_y = 0;
                win->viewport_height = win->pixel_height;
            }
        }
        win->user_left = floorf(-w0 / 2.0);
        win->user_right = ceilf(w0 / 2.0);
        win->user_bottom = floorf(-h0 / 2.0);
        win->user_top = ceilf(h0 / 2.0);

        win->user_safe_left = win->user_left;
        win->user_safe_right = win->user_right;
        win->user_safe_bottom = win->user_bottom;
        win->user_safe_top = win->user_top;
    } else {
        double sy = h1 / h0;
        double dx = (w1 - w0 * sy) / (2.0 * w0 * sy);
        double sx = w1 / w0;
        double dy = (h1 - h0 * sx) / (2.0 * h0 * sx);
        if (dx > 0.01) {
            win->user_left = floorf(-w0 / 2.0) - w0 * dx;
            win->user_right = ceilf(w0 / 2.0) + w0 * dx;
            win->user_bottom = floorf(-h0 / 2.0);
            win->user_top = ceilf(h0 / 2.0);
        } else {
            win->user_left = floorf(-w0 / 2.0);
            win->user_right = ceilf(w0 / 2.0);
            if (dy > 0.01) {
                win->user_bottom = floorf(-h0 / 2.0) - h0 * dy;
                win->user_top = ceilf(h0 / 2.0) + h0 * dy;
            } else {
                win->user_bottom = floorf(-h0 / 2.0);
                win->user_top = ceilf(h0 / 2.0);
            }
        }
        win->viewport_x = 0;
        win->viewport_y = 0;
        win->viewport_width = win->pixel_width;
        win->viewport_height = win->pixel_height;

        {
            // compute safe area
            int l, r, b, t;
            am_get_native_window_safe_area_margin(win->native_win, &l, &r, &b, &t);
            double w = win->user_right - win->user_left;
            double h = win->user_top - win->user_bottom;
            double margin_l = ((double)l / (double)win->screen_width) * w;
            double margin_r = ((double)r / (double)win->screen_width) * w;
            double margin_b = ((double)b / (double)win->screen_height) * h;
            double margin_t = ((double)t / (double)win->screen_height) * h;
            win->user_safe_left = win->user_left + margin_l;
            win->user_safe_right = win->user_right - margin_r;
            win->user_safe_bottom = win->user_bottom + margin_b;
            win->user_safe_top = win->user_top - margin_t;
        }
    }

    if (!win->user_projection) { // don't change user supplied projection
        win->projection = glm::ortho(win->user_left, win->user_right,
            win->user_bottom, win->user_top, -1.0, 1.0);
    }
}

static void draw_windows() {
    for (unsigned int i = 0; i < windows.size(); i++) {
        am_window *win = windows[i];
        if (!win->needs_closing) {
            am_native_window_bind_framebuffer(win->native_win);
            am_render_state *rstate = am_global_render_state;
            am_scene_node* roots[2];
            int num_roots = 1;
            roots[0] = win->scene;
            if (win->overlay != NULL) {
                roots[1] = win->overlay;
                num_roots = 2;
            }
            double t0 = 0.0;
            if (am_record_perf_timings) {
                t0 = am_get_current_time();
            }
            rstate->do_render(&roots[0], num_roots, 0, true, win->clear_color, win->stencil_clear_value,
                win->viewport_x, win->viewport_y, win->viewport_width, win->viewport_height,
                win->pixel_width, win->pixel_height,
                win->projection, win->has_depth_buffer);
            if (am_record_perf_timings) {
                am_last_frame_draw_time = am_get_current_time() - t0;
            }
            am_native_window_swap_buffers(win->native_win);
        }
    }
}

bool am_update_windows(lua_State *L) {
    static unsigned int frame = 0;
    if (!close_windows(L)) return false;
    resize_windows();
    am_reset_gl_frame_stats();
    draw_windows();
    frame++;
    if (am_conf_log_gl_calls && am_conf_log_gl_frames > 0) {
        char *msg = am_format("SDL_GL_SwapWindow(win);\n\n // ===================== END FRAME %d ==========================\n\n", frame);
        am_log_gl(msg);
        free(msg);
    }
    if (am_conf_log_gl_frames > 0) am_conf_log_gl_frames--;
    return windows.size() > 0;
}

bool am_execute_actions(lua_State *L, double dt) {
    double t0 = 0.0;
    if (am_record_perf_timings) {
        t0 = am_get_current_time();
    }
    am_pre_frame(L, dt);
    unsigned int n = windows.size();
    bool res = true;
    for (unsigned int i = 0; i < n; i++) {
        am_window *win = windows[i];
        if (!win->needs_closing && win->scene != NULL) {
            // make sure window size properties are up-to-date before running 
            // actions.
            update_size(win);
            if (!am_execute_node_actions(L, win->scene)) {
                res = false;
                break;
            }
            if (win->overlay != NULL && !am_execute_node_actions(L, win->overlay)) {
                res = false;
                break;
            }
            win->push(L);
            am_call_amulet(L, "_clear_events", 1, 0);
        }
    }
    am_post_frame(L);
    if (am_record_perf_timings) {
        am_last_frame_lua_time = am_get_current_time() - t0;
    }
    return res;
}

static void get_scene(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    window->pushref(L, window->scene_ref);
}

static void set_scene(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    if (lua_isnil(L, 3)) {
        if (window->scene == NULL) return;
        window->unref(L, window->scene_ref);
        window->scene_ref = LUA_NOREF;
        window->scene = NULL;
        return;
    }
    window->scene = am_get_userdata(L, am_scene_node, 3);
    if (window->scene_ref == LUA_NOREF) {
        window->scene_ref = window->ref(L, 3);
    } else {
        window->reref(L, window->scene_ref, 3);
    }
}

static am_property scene_property = {get_scene, set_scene};

static void get_overlay(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    window->pushref(L, window->overlay_ref);
}

static void set_overlay(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    if (lua_isnil(L, 3)) {
        if (window->overlay == NULL) return;
        window->unref(L, window->overlay_ref);
        window->overlay_ref = LUA_NOREF;
        window->overlay = NULL;
        return;
    }
    window->overlay = am_get_userdata(L, am_scene_node, 3);
    if (window->overlay_ref == LUA_NOREF) {
        window->overlay_ref = window->ref(L, 3);
    } else {
        window->reref(L, window->overlay_ref, 3);
    }
}

static am_property overlay_property = {get_overlay, set_overlay};

static void get_window_left(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_left);
}

static void get_window_right(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_right);
}

static void get_window_bottom(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_bottom);
}

static void get_window_top(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_top);
}

static void get_window_width(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_right - window->user_left);
}

static void get_window_height(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_top - window->user_bottom);
}

static am_property left_property = {get_window_left, NULL};
static am_property right_property = {get_window_right, NULL};
static am_property bottom_property = {get_window_bottom, NULL};
static am_property top_property = {get_window_top, NULL};
static am_property width_property = {get_window_width, NULL};
static am_property height_property = {get_window_height, NULL};

static void get_window_pixel_width(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, am_max(1, window->viewport_width));
}

static void get_window_pixel_height(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, am_max(1, window->viewport_height));
}

static am_property pixel_width_property = {get_window_pixel_width, NULL};
static am_property pixel_height_property = {get_window_pixel_height, NULL};

static void get_window_safe_left(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_safe_left);
}

static void get_window_safe_right(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_safe_right);
}

static void get_window_safe_bottom(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_safe_bottom);
}

static void get_window_safe_top(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushinteger(L, window->user_safe_top);
}

static am_property safe_left_property = {get_window_safe_left, NULL};
static am_property safe_right_property = {get_window_safe_right, NULL};
static am_property safe_bottom_property = {get_window_safe_bottom, NULL};
static am_property safe_top_property = {get_window_safe_top, NULL};

static void get_projection(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    am_new_userdata(L, am_mat4)->m = window->projection;
}

static void set_projection(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    if (lua_isnil(L, 3)) {
        window->user_projection = false;
        compute_viewport(window);
    } else {
        window->projection = am_get_userdata(L, am_mat4, 3)->m;
        window->user_projection = true;
    }
}

static am_property projection_property = {get_projection, set_projection};

static void get_lock_pointer(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushboolean(L, am_get_native_window_lock_pointer(window->native_win));
}

static void set_lock_pointer(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    bool value = lua_toboolean(L, 3);
    am_set_native_window_lock_pointer(window->native_win, value);
}

static am_property lock_pointer_property = {get_lock_pointer, set_lock_pointer};

static void get_show_cursor(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    lua_pushboolean(L, am_get_native_window_show_cursor(window->native_win));
}

static void set_show_cursor(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    bool value = lua_toboolean(L, 3);
    am_set_native_window_show_cursor(window->native_win, value);
}

static am_property show_cursor_property = {get_show_cursor, set_show_cursor};

static void get_window_mode(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    am_push_enum(L, am_window_mode, window->mode);
}

static void set_window_mode(lua_State *L, void *obj) {
    am_window *window = (am_window*)obj;
    am_window_mode old_mode = window->mode;
    am_window_mode new_mode = am_get_enum(L, am_window_mode, 3);
    if (old_mode != new_mode) {
        window->mode = new_mode;
        window->dirty = true;
    }
}

static am_property window_mode_property = {get_window_mode, set_window_mode};

static void get_clear_color(lua_State *L, void *obj) {
    am_window *win = (am_window*)obj;
    am_new_userdata(L, am_vec4)->v = win->clear_color;
}
static void set_clear_color(lua_State *L, void *obj) {
    am_window *win = (am_window*)obj;
    win->clear_color = am_get_userdata(L, am_vec4, 3)->v;
}

static am_property clear_color_property = {get_clear_color, set_clear_color};

static void get_stencil_clear_value(lua_State *L, void *obj) {
    am_window *win = (am_window*)obj;
    lua_pushinteger(L, win->stencil_clear_value);
}
static void set_stencil_clear_value(lua_State *L, void *obj) {
    am_window *win = (am_window*)obj;
    win->stencil_clear_value = luaL_checkinteger(L, 3);
}

static am_property stencil_clear_value_property = {get_stencil_clear_value, set_stencil_clear_value};

static void get_letterbox(lua_State *L, void *obj) {
    am_window *win = (am_window*)obj;
    lua_pushboolean(L, win->letterbox ? 1 : 0);
}
static void set_letterbox(lua_State *L, void *obj) {
    am_window *win = (am_window*)obj;
    win->letterbox = lua_toboolean(L, 3);
    compute_viewport(win);
}

static am_property letterbox_property = {get_letterbox, set_letterbox};

static void register_window_mt(lua_State *L) {
    lua_newtable(L);

    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    am_register_property(L, "scene", &scene_property);
    am_register_property(L, "_overlay", &overlay_property);
    am_register_property(L, "lock_pointer", &lock_pointer_property);
    am_register_property(L, "show_cursor", &show_cursor_property);
    am_register_property(L, "pixel_width", &pixel_width_property);
    am_register_property(L, "pixel_height", &pixel_height_property);
    am_register_property(L, "left", &left_property);
    am_register_property(L, "right", &right_property);
    am_register_property(L, "bottom", &bottom_property);
    am_register_property(L, "top", &top_property);
    am_register_property(L, "width", &width_property);
    am_register_property(L, "height", &height_property);
    am_register_property(L, "safe_left", &safe_left_property);
    am_register_property(L, "safe_right", &safe_right_property);
    am_register_property(L, "safe_bottom", &safe_bottom_property);
    am_register_property(L, "safe_top", &safe_top_property);
    am_register_property(L, "mode", &window_mode_property);
    am_register_property(L, "clear_color", &clear_color_property);
    am_register_property(L, "stencil_clear_value", &stencil_clear_value_property);
    am_register_property(L, "letterbox", &letterbox_property);
    am_register_property(L, "projection", &projection_property);

    lua_pushcclosure(L, close_window, 0);
    lua_setfield(L, -2, "close");

    am_register_metatable(L, "window", MT_am_window, 0);
}

static int get_frame_draw_calls(lua_State *L) {
    lua_pushinteger(L, am_frame_draw_calls);
    return 1;
}

static int get_frame_use_program_calls(lua_State *L) {
    lua_pushinteger(L, am_frame_use_program_calls);
    return 1;
}

void am_open_window_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"window",    create_window},
        {"_frame_draw_calls", get_frame_draw_calls},
        {"_frame_use_program_calls", get_frame_use_program_calls},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_window_mt(L);

    am_enum_value window_mode_enum[] = {
        {"windowed",        AM_WINDOW_MODE_WINDOWED},
        {"fullscreen",      AM_WINDOW_MODE_FULLSCREEN_DESKTOP},
        {"real-fullscreen", AM_WINDOW_MODE_FULLSCREEN},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_window_mode, window_mode_enum);

    am_enum_value orientation_enum[] = {
        {"portrait",    AM_DISPLAY_ORIENTATION_PORTRAIT},
        {"landscape",   AM_DISPLAY_ORIENTATION_LANDSCAPE},
        {"any",         AM_DISPLAY_ORIENTATION_ANY},
        {"hybrid",      AM_DISPLAY_ORIENTATION_HYBRID},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_display_orientation, orientation_enum);

    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_WINDOW_TABLE);

    windows.clear();
}
