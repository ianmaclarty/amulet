#include "amulet.h"

am_render_state am_global_render_state;

am_viewport_state::am_viewport_state() {
    x = 0;
    y = 0;
    w = 0;
    h = 0;
}

void am_viewport_state::set(int x, int y, int w, int h) {
    am_viewport_state::x = x;
    am_viewport_state::y = y;
    am_viewport_state::w = w;
    am_viewport_state::h = h;
}

void am_viewport_state::bind(am_render_state *rstate) {
    if (x != rstate->bound_viewport_state.x ||
        y != rstate->bound_viewport_state.y ||
        w != rstate->bound_viewport_state.w ||
        h != rstate->bound_viewport_state.h)
    {
        am_set_viewport(x, y, w, h);
        rstate->bound_viewport_state = *this;
    }
}

am_depth_test_state::am_depth_test_state() {
    test_enabled = false;
    mask_enabled = false;
    func = AM_DEPTH_FUNC_ALWAYS;
}

void am_depth_test_state::set(bool test_enabled, bool mask_enabled, am_depth_func func) {
    am_depth_test_state::test_enabled = test_enabled;
    am_depth_test_state::mask_enabled = mask_enabled;
    am_depth_test_state::func = func;
}

void am_depth_test_state::restore(am_depth_test_state *old) {
    set(old->test_enabled, old->mask_enabled, old->func);
}

void am_depth_test_state::bind(am_render_state *rstate) {
    if (test_enabled != rstate->bound_depth_test_state.test_enabled) {
        am_set_depth_test_enabled(test_enabled);
        rstate->bound_depth_test_state.test_enabled = test_enabled;
    }
    if (func != rstate->bound_depth_test_state.func) {
        am_set_depth_func(func);
        rstate->bound_depth_test_state.func = func;
    }
}

am_cull_face_state::am_cull_face_state() {
    enabled = false;
    winding = AM_FACE_WIND_CCW;
    side = AM_CULL_FACE_BACK;
}

void am_cull_face_state::set(bool enabled, am_face_winding winding, am_cull_face_side side) {
    am_cull_face_state::enabled = enabled;
    am_cull_face_state::winding = winding;
    am_cull_face_state::side = side;
}

void am_cull_face_state::restore(am_cull_face_state *old) {
    set(old->enabled, old->winding, old->side);
}

void am_cull_face_state::bind(am_render_state *rstate) {
    if (enabled != rstate->bound_cull_face_state.enabled) {
        am_set_cull_face_enabled(enabled);
        rstate->bound_cull_face_state.enabled = enabled;
    }
    if (side != rstate->bound_cull_face_state.side) {
        am_set_cull_face_side(side);
        rstate->bound_cull_face_state.side = side;
    }
}

am_blend_state::am_blend_state() {
    enabled         = false; 
    equation_rgb    = AM_BLEND_EQUATION_ADD;
    equation_alpha  = AM_BLEND_EQUATION_ADD;
    sfactor_rgb     = AM_BLEND_SFACTOR_SRC_ALPHA;
    dfactor_rgb     = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
    sfactor_alpha   = AM_BLEND_SFACTOR_SRC_ALPHA;
    dfactor_alpha   = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
    color_r         = 1.0f;
    color_g         = 1.0f;
    color_b         = 1.0f;
    color_a         = 1.0f;
}

void am_blend_state::set_mode(am_blend_mode mode) {
    switch (mode) {
        case AM_BLEND_MODE_OFF:
            enabled         = false; 
            equation_rgb    = AM_BLEND_EQUATION_ADD;
            equation_alpha  = AM_BLEND_EQUATION_ADD;
            sfactor_rgb     = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            sfactor_alpha   = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            color_r         = 1.0f;
            color_g         = 1.0f;
            color_b         = 1.0f;
            color_a         = 1.0f;
        case AM_BLEND_MODE_NORMAL:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_ADD;
            equation_alpha  = AM_BLEND_EQUATION_ADD;
            sfactor_rgb     = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            sfactor_alpha   = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            color_r         = 1.0f;
            color_g         = 1.0f;
            color_b         = 1.0f;
            color_a         = 1.0f;
        case AM_BLEND_MODE_ADD:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_ADD;
            equation_alpha  = AM_BLEND_EQUATION_ADD;
            sfactor_rgb     = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE;
            sfactor_alpha   = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE;
            color_r         = 1.0f;
            color_g         = 1.0f;
            color_b         = 1.0f;
            color_a         = 1.0f;
    }
}

void am_blend_state::set(
    bool enabled,
    am_blend_equation equation_rgb,
    am_blend_equation equation_alpha,
    am_blend_sfactor  sfactor_rgb,
    am_blend_dfactor  dfactor_rgb,
    am_blend_sfactor  sfactor_alpha,
    am_blend_dfactor  dfactor_alpha,
    float             color_r,
    float             color_g,
    float             color_b,
    float             color_a)
{
    am_blend_state::enabled         = enabled       ;
    am_blend_state::equation_rgb    = equation_rgb  ;
    am_blend_state::equation_alpha  = equation_alpha;
    am_blend_state::sfactor_rgb     = sfactor_rgb   ;
    am_blend_state::dfactor_rgb     = dfactor_rgb   ;
    am_blend_state::sfactor_alpha   = sfactor_alpha ;
    am_blend_state::dfactor_alpha   = dfactor_alpha ;
    am_blend_state::color_r         = color_r       ;
    am_blend_state::color_g         = color_g       ;
    am_blend_state::color_b         = color_b       ;
    am_blend_state::color_a         = color_a       ;
}

void am_blend_state::restore(am_blend_state *old) {
    set(
        old->enabled       ,
        old->equation_rgb  ,
        old->equation_alpha,
        old->sfactor_rgb   ,
        old->dfactor_rgb   ,
        old->sfactor_alpha ,
        old->dfactor_alpha ,
        old->color_r       ,
        old->color_g       ,
        old->color_b       ,
        old->color_a
    );
}

void am_blend_state::bind(am_render_state *rstate) {
    am_blend_state *bound = &rstate->bound_blend_state;
    if (enabled != bound->enabled) {
        am_set_blend_enabled(enabled);
        bound->enabled = enabled;
    }
    if (!enabled) return;
    if (equation_rgb != bound->equation_rgb ||
        equation_alpha != bound->equation_alpha)
    {
        am_set_blend_equation(equation_rgb, equation_alpha);
        bound->equation_rgb = equation_rgb;
        bound->equation_alpha = equation_alpha;
    }
    if (sfactor_rgb != bound->sfactor_rgb ||
        dfactor_rgb != bound->dfactor_rgb ||
        sfactor_alpha != bound->sfactor_alpha ||
        dfactor_alpha != bound->dfactor_alpha)
    {
        am_set_blend_func(sfactor_rgb, dfactor_rgb, sfactor_alpha, dfactor_alpha);
        bound->sfactor_rgb = sfactor_rgb;
        bound->dfactor_rgb = dfactor_rgb;
        bound->sfactor_alpha = sfactor_alpha;
        bound->dfactor_alpha = dfactor_alpha;
    }
    if (color_r != bound->color_r ||
        color_g != bound->color_g ||
        color_b != bound->color_b ||
        color_a != bound->color_a)
    {
        am_set_blend_color(color_r, color_g, color_b, color_a);
        bound->color_r = color_r;
        bound->color_g = color_g;
        bound->color_b = color_b;
        bound->color_a = color_a;
    }
}

void am_render_state::update_state() {
    active_viewport_state.bind(this);
    active_depth_test_state.bind(this);
    active_cull_face_state.bind(this);
    active_blend_state.bind(this);
    bind_active_program();
    bind_active_program_params();
}

void am_render_state::setup(am_framebuffer_id fb, bool clear, int w, int h, bool has_depthbuffer) {
    if (fb != 0) {
        // default framebuffer should be bound by backend-specific code
        // in am_native_window_pre_render
        am_bind_framebuffer(fb);
    }
    active_viewport_state.set(0, 0, w, h);
    active_depth_test_state.set(has_depthbuffer, has_depthbuffer,
        has_depthbuffer ? AM_DEPTH_FUNC_LESS : AM_DEPTH_FUNC_ALWAYS);
    if (clear) am_clear_framebuffer(true, true, true);
}

void am_render_state::draw_arrays(am_draw_mode mode, int first, int draw_array_count) {
    if (active_program == NULL) {
        am_log1("%s", "WARNING: ignoring draw_arrays, "
            "because no shader program has been bound");
        return;
    }
    update_state();
    if (validate_active_program(mode)) {
        if (max_draw_array_size == INT_MAX && draw_array_count == INT_MAX) {
            am_log1("%s", "WARNING: ignoring draw_arrays, "
                "because no attribute arrays have been bound");
            draw_array_count = 0;
        }
        int count = max_draw_array_size - first;
        if (count > draw_array_count) count = draw_array_count;
        if (count > 0) {
            am_draw_arrays(mode, first, count);
        }
    }
}

void am_render_state::draw_elements(am_draw_mode mode, int first, int count,
    am_buffer_view *indices_view, am_element_index_type type)
{
    if (active_program == NULL) {
        am_log1("%s", "WARNING: ignoring draw_elements, "
            "because no shader program has been bound");
        return;
    }
    update_state();
    if (validate_active_program(mode)) {
        if (indices_view->buffer->elembuf_id == 0) {
            indices_view->buffer->create_elembuf();
        }
        indices_view->buffer->update_if_dirty();
        indices_view->update_max_elem_if_required();
        if (max_draw_array_size == INT_MAX) {
            am_log1("%s", "WARNING: ignoring draw_elements, "
                "because no attribute arrays have been bound");
            count = 0;
        } else if ((int)indices_view->max_elem >= max_draw_array_size) {
            am_log1("WARNING: ignoring draw_elements, "
                "because one of its indices (%d) is out of bounds "
                "(max allowed = %d)",
                ((int)indices_view->max_elem + 1), max_draw_array_size);
            count = 0;
        }
        if (count > (indices_view->size - first)) {
            count = (indices_view->size - first);
        }
        if (count > 0) {
            am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, indices_view->buffer->elembuf_id);
            am_draw_elements(mode, count, type, first * indices_view->stride);
        }
    }
}

bool am_render_state::validate_active_program(am_draw_mode mode) {
    if (mode == AM_DRAWMODE_POINTS && !active_program->sets_point_size) {
        // The value of gl_PointSize is undefined if it's not set
        // and with some drivers (notably Angle) not setting gl_PointSize
        // will cause nothing to be drawn,
        am_log1("WARNING: %s", 
            "attempt to draw points with a shader program that did not set gl_PointSize (nothing will be drawn)");
        return false;
    }
    if (am_conf_validate_shader_programs) {
        bool valid = am_validate_program(active_program->program_id);
        if (!valid) {
            char *log = am_get_program_info_log(active_program->program_id);
            am_log1("WARNING: shader program failed validation: %s", log);
            free(log);
        }
        return valid;
    } else {
        return true;
    }
}

void am_render_state::bind_active_program() {
    if (bound_program_id != active_program->program_id) {
        am_use_program(active_program->program_id);
        bound_program_id = active_program->program_id;
    }
}

void am_render_state::bind_active_program_params() {
    max_draw_array_size = INT_MAX;
    for (int i = 0; i < active_program->num_params; i++) {
        am_program_param *param = &active_program->params[i];
        param->bind(this);
    }
}

am_render_state::am_render_state() {
    max_draw_array_size = 0;

    bound_program_id = 0;
    active_program = NULL;

    next_free_texture_unit = 0;
}

am_render_state::~am_render_state() {
}

static void register_draw_arrays_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_metatable(L, "draw_arrays", MT_am_draw_arrays_node, MT_am_scene_node);
}

void am_draw_arrays_node::render(am_render_state *rstate) {
    rstate->draw_arrays(mode, first, count);
}

static int create_draw_arrays_node(lua_State *L) {
    int nargs = am_check_nargs(L, 0);
    int first = 0;
    int count = INT_MAX;
    am_draw_mode mode = AM_DRAWMODE_TRIANGLES;
    if (nargs > 0) {
        mode = am_get_enum(L, am_draw_mode, 1);
    }
    if (nargs > 1) {
        first = luaL_checkinteger(L, 2) - 1;
        if (first < 0) {
            return luaL_error(L, "argument 2 must be positive");
        }
    }
    if (nargs > 2) {
        count = luaL_checkinteger(L, 3);
        if (count < 0) {
            return luaL_error(L, "argument 3 must be non-negative");
        }
    }
    am_draw_arrays_node *node = am_new_userdata(L, am_draw_arrays_node);
    node->first = first;
    node->count = count;
    node->mode = mode;
    return 1;
}

am_draw_elements_node::am_draw_elements_node() {
    first = 0;
    count = INT_MAX;
    mode = AM_DRAWMODE_TRIANGLES;
    type = AM_ELEMENT_TYPE_USHORT;
    indices_view = NULL;
    view_ref = LUA_NOREF;
}

static void get_draw_elements_first(lua_State *L, void *obj) {
    am_draw_elements_node *node = (am_draw_elements_node*)obj;
    lua_pushinteger(L, node->first + 1);
}

static void set_draw_elements_first(lua_State *L, void *obj) {
    am_draw_elements_node *node = (am_draw_elements_node*)obj;
    int first = luaL_checkinteger(L, 3) - 1;
    if (first < 0) {
        luaL_error(L, "value must be positive (in fact %d)", first + 1);
    }
    node->first = first;
}

static am_property draw_elements_first_property = {get_draw_elements_first, set_draw_elements_first};

static void get_draw_elements_count(lua_State *L, void *obj) {
    am_draw_elements_node *node = (am_draw_elements_node*)obj;
    lua_pushinteger(L, node->count);
}

static void set_draw_elements_count(lua_State *L, void *obj) {
    am_draw_elements_node *node = (am_draw_elements_node*)obj;
    int count = luaL_checkinteger(L, 3);
    if (count < 0) {
        luaL_error(L, "value must be non-negative (in fact %d)", count);
    }
    node->count = count;
}

static am_property draw_elements_count_property = {get_draw_elements_count, set_draw_elements_count};

static void register_draw_elements_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "first", &draw_elements_first_property);
    am_register_property(L, "count", &draw_elements_count_property);

    am_register_metatable(L, "draw_elements", MT_am_draw_elements_node, MT_am_scene_node);
}

void am_draw_elements_node::render(am_render_state *rstate) {
    rstate->draw_elements(mode, first, count, indices_view, type);
}

static int create_draw_elements_node(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    int first = 0;
    int count = INT_MAX;
    am_draw_mode mode = AM_DRAWMODE_TRIANGLES;
    am_buffer_view *indices_view = am_get_userdata(L, am_buffer_view, 1);
    am_draw_elements_node *node = am_new_userdata(L, am_draw_elements_node);
    switch (indices_view->type) {
        case AM_VIEW_TYPE_USHORT_ELEM:
            if (indices_view->stride != 2) {
                return luaL_error(L, "ushort_elem array must have stride 2 when used with draw_elements");
            }
            node->type = AM_ELEMENT_TYPE_USHORT;
            break;
        case AM_VIEW_TYPE_UINT_ELEM:
            if (indices_view->stride != 4) {
                return luaL_error(L, "uint_elem array must have stride 4 when used with draw_elements");
            }
            node->type = AM_ELEMENT_TYPE_UINT;
            break;
        default:
            return luaL_error(L, "only ushort_elem and uint_elem views can be used as element arrays");
    }
    node->indices_view = indices_view;
    node->view_ref = node->ref(L, 1);
    if (nargs > 1) {
        mode = am_get_enum(L, am_draw_mode, 2);
    }
    if (nargs > 2) {
        first = luaL_checkinteger(L, 3) - 1;
        if (first < 0) {
            return luaL_error(L, "argument 3 must be positive");
        }
    }
    if (nargs > 3) {
        count = luaL_checkinteger(L, 4);
        if (count < 0) {
            return luaL_error(L, "argument 4 must be non-negative");
        }
    }
    node->first = first;
    node->count = count;
    node->mode = mode;
    return 1;
}

void am_open_renderer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"draw_arrays", create_draw_arrays_node},
        {"draw_elements", create_draw_elements_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);

    am_enum_value draw_mode_enum[] = {
        {"points",          AM_DRAWMODE_POINTS},
        {"lines",           AM_DRAWMODE_LINES},
        {"line_strip",      AM_DRAWMODE_LINE_STRIP},
        {"line_loop",       AM_DRAWMODE_LINE_LOOP},
        {"triangles",       AM_DRAWMODE_TRIANGLES},
        {"triangle_strip",  AM_DRAWMODE_TRIANGLE_STRIP},
        {"triangle_fan",    AM_DRAWMODE_TRIANGLE_FAN},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_draw_mode, draw_mode_enum);

    register_draw_arrays_node_mt(L);
    register_draw_elements_node_mt(L);
}
