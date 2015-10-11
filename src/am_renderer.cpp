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

void am_viewport_state::restore(am_viewport_state *old) {
    set(old->x, old->y, old->w, old->h);
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

am_color_mask_state::am_color_mask_state() {
    r = true;
    g = true;
    b = true;
    a = true;
}

void am_color_mask_state::set(bool r, bool g, bool b, bool a) {
    am_color_mask_state::r = r;
    am_color_mask_state::g = g;
    am_color_mask_state::b = b;
    am_color_mask_state::a = a;
}

void am_color_mask_state::restore(am_color_mask_state *old) {
    set(old->r, old->g, old->b, old->a);
}

void am_color_mask_state::bind(am_render_state *rstate) {
    if (r != rstate->bound_color_mask_state.r ||
        g != rstate->bound_color_mask_state.g ||
        b != rstate->bound_color_mask_state.b ||
        a != rstate->bound_color_mask_state.a)
    {
        am_set_framebuffer_color_mask(r, g, b, a);
        rstate->bound_color_mask_state = *this;
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
    if (mask_enabled != rstate->bound_depth_test_state.mask_enabled) {
        am_set_framebuffer_depth_mask(mask_enabled);
        rstate->bound_depth_test_state.mask_enabled = mask_enabled;
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
            break;
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
            break;
        case AM_BLEND_MODE_PREMULT:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_ADD;
            equation_alpha  = AM_BLEND_EQUATION_ADD;
            sfactor_rgb     = AM_BLEND_SFACTOR_ONE;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            sfactor_alpha   = AM_BLEND_SFACTOR_ONE;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            color_r         = 1.0f;
            color_g         = 1.0f;
            color_b         = 1.0f;
            color_a         = 1.0f;
            break;
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
            break;
        case AM_BLEND_MODE_SUBTRACT:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_REVERSE_SUBTRACT;
            equation_alpha  = AM_BLEND_EQUATION_REVERSE_SUBTRACT;
            sfactor_rgb     = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE;
            sfactor_alpha   = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE;
            color_r         = 1.0f;
            color_g         = 1.0f;
            color_b         = 1.0f;
            color_a         = 1.0f;
            break;
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

void am_render_state::enable_vaas(int n) {
    if (n > num_enabled_vaas) {
        for (int i = num_enabled_vaas; i < n; i++) {
            am_set_attribute_array_enabled(i, true);
        }
    } else {
        for (int i = n; i < num_enabled_vaas; i++) {
            am_set_attribute_array_enabled(i, false);
        }
    }
    num_enabled_vaas = n;
}

bool am_render_state::update_state() {
    assert(active_program != NULL);
    active_viewport_state.bind(this);
    active_color_mask_state.bind(this);
    active_depth_test_state.bind(this);
    active_cull_face_state.bind(this);
    active_blend_state.bind(this);
    bind_active_program();
    if (!bind_active_program_params()) {
        return false;
    }
    enable_vaas(active_program->num_vaas);
    return true;
}

static void setup(am_render_state *rstate, am_framebuffer_id fb,
    bool clear, glm::vec4 clear_color,
    int x, int y, int w, int h, glm::mat4 proj,
    bool has_depthbuffer)
{
    if (fb != 0) {
        // default framebuffer should be bound by backend-specific code
        // in am_native_window_pre_render
        am_bind_framebuffer(fb);
    }
    rstate->active_viewport_state.set(x, y, w, h);

    rstate->active_depth_test_state.set(has_depthbuffer, has_depthbuffer,
        has_depthbuffer ? AM_DEPTH_FUNC_LESS : AM_DEPTH_FUNC_ALWAYS);
    am_set_framebuffer_depth_mask(has_depthbuffer);
    rstate->bound_depth_test_state.mask_enabled = has_depthbuffer;

    rstate->bound_color_mask_state.set(true, true, true, true);
    rstate->active_color_mask_state.set(true, true, true, true);
    am_set_framebuffer_color_mask(true, true, true, true);

    rstate->projection_param->set_mat4(proj);
    if (clear) {
        am_set_framebuffer_clear_color(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        am_clear_framebuffer(true, true, true);
    }
}

void am_render_state::do_render(am_scene_node *root, am_framebuffer_id fb,
    bool clear, glm::vec4 clear_color, int x, int y, int w, int h,
    glm::mat4 proj, bool has_depthbuffer)
{
    setup(this, fb, clear, clear_color, x, y, w, h, proj, has_depthbuffer);
    if (root != NULL) {
        next_pass = 1;
        do {
            pass = next_pass;
            pass_mask = 1;
            root->render(this);
        } while (next_pass > pass);
    }

    // Unbind the current program, because it might be
    // deleted and the id reused before the next call to
    // do_render.
    bound_program_id = 0;
    am_use_program(0);

    assert(active_program == NULL);
    assert(next_free_texture_unit == 0);
}

static bool check_pass(am_render_state *rstate) {
    return (rstate->pass & rstate->pass_mask);
}

void am_render_state::draw_arrays(am_draw_mode mode, int first, int draw_array_count) {
    if (!check_pass(this)) { return; }
    if (active_program == NULL) {
        am_log1("%s", "WARNING: ignoring draw, "
            "because no shader program has been bound");
        return;
    }
    if (!update_state()) {
        // warning would already have been emitted
        return;
    }
    if (validate_active_program(mode)) {
        if (max_draw_array_size == INT_MAX && draw_array_count == INT_MAX) {
            am_log1("%s", "WARNING: ignoring draw, "
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
    if (!check_pass(this)) return;
    if (active_program == NULL) {
        am_log1("%s", "WARNING: ignoring draw, "
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
            am_log1("%s", "WARNING: ignoring draw, "
                "because no attribute arrays have been bound");
            count = 0;
        } else if ((int)indices_view->max_elem >= max_draw_array_size) {
            am_log1("WARNING: ignoring draw, "
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

bool am_render_state::bind_active_program_params() {
    max_draw_array_size = INT_MAX;
    for (int i = 0; i < active_program->num_params; i++) {
        am_program_param *param = &active_program->params[i];
        if (!param->bind(this)) return false;
    }
    return true;
}

am_render_state::am_render_state() {
    pass = 1;
    next_pass = 1;
    pass_mask = 1;

    max_draw_array_size = 0;

    num_enabled_vaas = 0;
    bound_program_id = 0;
    active_program = NULL;

    param_name_map_capacity = 0;
    param_name_map = NULL;

    next_free_texture_unit = 0;

    modelview_param = NULL;
    projection_param = NULL;
}

am_draw_node::am_draw_node() {
    first = 0;
    count = INT_MAX;
    mode = AM_DRAWMODE_TRIANGLES;
    type = AM_ELEMENT_TYPE_USHORT;
    indices_view = NULL;
    view_ref = LUA_NOREF;
}

static void get_draw_node_first(lua_State *L, void *obj) {
    am_draw_node *node = (am_draw_node*)obj;
    lua_pushinteger(L, node->first + 1);
}

static void set_draw_node_first(lua_State *L, void *obj) {
    am_draw_node *node = (am_draw_node*)obj;
    int first = luaL_checkinteger(L, 3) - 1;
    if (first < 0) {
        luaL_error(L, "value must be positive (in fact %d)", first + 1);
    }
    node->first = first;
}

static am_property draw_node_first_property = {get_draw_node_first, set_draw_node_first};

static void get_draw_node_count(lua_State *L, void *obj) {
    am_draw_node *node = (am_draw_node*)obj;
    lua_pushinteger(L, node->count);
}

static void set_draw_node_count(lua_State *L, void *obj) {
    am_draw_node *node = (am_draw_node*)obj;
    int count = luaL_checkinteger(L, 3);
    if (count < 0) {
        luaL_error(L, "value must be non-negative (in fact %d)", count);
    }
    node->count = count;
}

static am_property draw_node_count_property = {get_draw_node_count, set_draw_node_count};

static void set_indices(lua_State *L, am_draw_node *node, int idx) {
    am_buffer_view *indices_view = am_get_userdata(L, am_buffer_view, idx);
    switch (indices_view->type) {
        case AM_VIEW_TYPE_USHORT_ELEM:
            if (indices_view->stride != 2) {
                luaL_error(L, "ushort_elem array must have stride 2 when used with draw_elements");
            }
            node->type = AM_ELEMENT_TYPE_USHORT;
            break;
        case AM_VIEW_TYPE_UINT_ELEM:
            if (indices_view->stride != 4) {
                luaL_error(L, "uint_elem array must have stride 4 when used with draw_elements");
            }
            node->type = AM_ELEMENT_TYPE_UINT;
            break;
        default:
            luaL_error(L, "only ushort_elem and uint_elem views can be used as element arrays");
    }
    node->indices_view = indices_view;
    if (node->view_ref == LUA_NOREF) {
        node->view_ref = node->ref(L, idx);
    } else {
        node->reref(L, node->view_ref, idx);
    }
}

static void get_draw_node_elements(lua_State *L, void *obj) {
    am_draw_node *node = (am_draw_node*)obj;
    if (node->view_ref != LUA_NOREF) {
        node->pushref(L, node->view_ref);
    } else {
        lua_pushnil(L);
    }
}

static void set_draw_node_elements(lua_State *L, void *obj) {
    am_draw_node *node = (am_draw_node*)obj;
    set_indices(L, node, 3);
}

static am_property draw_node_elements_property = {get_draw_node_elements, set_draw_node_elements};

static void register_draw_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "first", &draw_node_first_property);
    am_register_property(L, "count", &draw_node_count_property);
    am_register_property(L, "elements", &draw_node_elements_property);

    am_register_metatable(L, "draw_node", MT_am_draw_node, MT_am_scene_node);
}

void am_draw_node::render(am_render_state *rstate) {
    if (indices_view == NULL) {
        rstate->draw_arrays(mode, first, count);
    } else {
        rstate->draw_elements(mode, first, count, indices_view, type);
    }
}

static int create_draw_node(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    int first = 0;
    int count = INT_MAX;
    am_draw_mode mode = am_get_enum(L, am_draw_mode, 1);
    am_draw_node *node = am_new_userdata(L, am_draw_node);
    node->tags.push_back(L, AM_TAG_DRAW);
    int nxt_arg = 2;
    if (nargs >= nxt_arg && !lua_isnumber(L, nxt_arg)) {
        set_indices(L, node, nxt_arg);
        nxt_arg++;
    }
    if (nargs >= nxt_arg) {
        first = luaL_checkinteger(L, nxt_arg) - 1;
        if (first < 0) {
            return luaL_error(L, "argument %d must be positive", nxt_arg);
        }
        nxt_arg++;
    }
    if (nargs >= nxt_arg) {
        count = luaL_checkinteger(L, nxt_arg);
        if (count < 0) {
            return luaL_error(L, "argument %d must be non-negative", nxt_arg);
        }
        nxt_arg++;
    }
    node->first = first;
    node->count = count;
    node->mode = mode;
    return 1;
}

void am_pass_filter_node::render(am_render_state *rstate) {
    if (rstate->pass & mask) {
        uint32_t old = rstate->pass_mask;
        rstate->pass_mask = rstate->pass;
        render_children(rstate);
        rstate->pass_mask = old;
    }
    uint32_t next = rstate->pass << 1;
    while (next && !(next & mask)) {
        next <<= 1;
    }
    if (next && (rstate->next_pass == rstate->pass || next < rstate->next_pass)) {
        rstate->next_pass = next;
    }
}

static int create_pass_filter_node(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_pass_filter_node *node = am_new_userdata(L, am_pass_filter_node);
    node->mask = 1 << (luaL_checkinteger(L, 1) - 1);
    for (int i = 2; i <= nargs; i++) {
        node->mask |= 1 << (luaL_checkinteger(L, i) - 1);
    }
    return 1;
}

static void register_pass_filter_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_metatable(L, "pass_filter", MT_am_pass_filter_node, MT_am_scene_node);
}

static void init_param_name_map(am_render_state *rstate, lua_State *L) {
    rstate->param_name_map_capacity = 32;
    rstate->param_name_map = (am_program_param_name_slot*)malloc(sizeof(am_program_param_name_slot) * rstate->param_name_map_capacity);
    memset(rstate->param_name_map, 0, sizeof(am_program_param_name_slot) * rstate->param_name_map_capacity);
    for (int i = 0; i < rstate->param_name_map_capacity; i++) {
        rstate->param_name_map[i].name = NULL;
        rstate->param_name_map[i].value.type = AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED;
    }
    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_PARAM_NAME_STRING_TABLE);

    // create default modelview and projection names
    lua_pushstring(L, am_conf_default_modelview_matrix_name);
    rstate->modelview_param = &rstate->param_name_map[am_lookup_param_name(L, -1)].value;
    lua_pop(L, 1); // modelview name string
    rstate->modelview_param->set_mat4(glm::mat4(1.0f));

    lua_pushstring(L, am_conf_default_projection_matrix_name);
    rstate->projection_param = &rstate->param_name_map[am_lookup_param_name(L, -1)].value;
    lua_pop(L, 1); // projection name string
    rstate->projection_param->set_mat4(glm::mat4(1.0f));
}

void am_open_renderer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"draw", create_draw_node},
        {"pass", create_pass_filter_node},
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

    register_draw_node_mt(L);
    register_pass_filter_node_mt(L);

    init_param_name_map(&am_global_render_state, L);
}
