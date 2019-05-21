#include "amulet.h"

am_render_state *am_global_render_state = NULL;

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

void am_viewport_state::bind(am_render_state *rstate, bool force) {
    if (force ||
        x != rstate->bound_viewport_state.x ||
        y != rstate->bound_viewport_state.y ||
        w != rstate->bound_viewport_state.w ||
        h != rstate->bound_viewport_state.h)
    {
        am_set_viewport(x, y, w, h);
        rstate->bound_viewport_state = *this;
    }
}

am_scissor_test_state::am_scissor_test_state() {
    enabled = false;
    x = 0;
    y = 0;
    w = 0;
    h = 0;
}

void am_scissor_test_state::set(bool enabled, int x, int y, int w, int h) {
    am_scissor_test_state::enabled = enabled;
    am_scissor_test_state::x = x;
    am_scissor_test_state::y = y;
    am_scissor_test_state::w = w;
    am_scissor_test_state::h = h;
}

void am_scissor_test_state::restore(am_scissor_test_state *old) {
    set(old->enabled, old->x, old->y, old->w, old->h);
}

void am_scissor_test_state::bind(am_render_state *rstate, bool force) {
    if (force ||
        enabled != rstate->bound_scissor_test_state.enabled ||
        x != rstate->bound_scissor_test_state.x ||
        y != rstate->bound_scissor_test_state.y ||
        w != rstate->bound_scissor_test_state.w ||
        h != rstate->bound_scissor_test_state.h)
    {
        am_set_scissor_test_enabled(enabled);
        if (enabled) {
            am_set_scissor(x, y, w, h);
        }
        rstate->bound_scissor_test_state = *this;
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

void am_color_mask_state::bind(am_render_state *rstate, bool force) {
    if (force ||
        r != rstate->bound_color_mask_state.r ||
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

void am_depth_test_state::bind(am_render_state *rstate, bool force) {
    if (force || test_enabled != rstate->bound_depth_test_state.test_enabled) {
        am_set_depth_test_enabled(test_enabled);
        rstate->bound_depth_test_state.test_enabled = test_enabled;
    }
    if (force || mask_enabled != rstate->bound_depth_test_state.mask_enabled) {
        am_set_framebuffer_depth_mask(mask_enabled);
        rstate->bound_depth_test_state.mask_enabled = mask_enabled;
    }
    if (force || func != rstate->bound_depth_test_state.func) {
        am_set_depth_func(func);
        rstate->bound_depth_test_state.func = func;
    }
}

am_stencil_test_state::am_stencil_test_state() {
    enabled = false;
    ref = 0;
    read_mask = 255;
    write_mask = 255;
    func_front = AM_STENCIL_FUNC_ALWAYS;
    op_fail_front = AM_STENCIL_OP_KEEP;
    op_zfail_front = AM_STENCIL_OP_KEEP;
    op_zpass_front = AM_STENCIL_OP_KEEP;
    func_back = AM_STENCIL_FUNC_ALWAYS;
    op_fail_back = AM_STENCIL_OP_KEEP;
    op_zfail_back = AM_STENCIL_OP_KEEP;
    op_zpass_back = AM_STENCIL_OP_KEEP;
}

void am_stencil_test_state::set(
    bool                    enabled,
    am_glint                ref,
    am_gluint               read_mask,
    am_gluint               write_mask,
    am_stencil_func         func_front,
    am_stencil_op           op_fail_front,
    am_stencil_op           op_zfail_front,
    am_stencil_op           op_zpass_front,
    am_stencil_func         func_back,
    am_stencil_op           op_fail_back,
    am_stencil_op           op_zfail_back,
    am_stencil_op           op_zpass_back)
{
    am_stencil_test_state::enabled = enabled;
    am_stencil_test_state::ref = ref;
    am_stencil_test_state::read_mask = read_mask;
    am_stencil_test_state::write_mask = write_mask;
    am_stencil_test_state::func_front = func_front;
    am_stencil_test_state::op_fail_front = op_fail_front;
    am_stencil_test_state::op_zfail_front = op_zfail_front;
    am_stencil_test_state::op_zpass_front = op_zpass_front;
    am_stencil_test_state::func_back = func_back;
    am_stencil_test_state::op_fail_back = op_fail_back;
    am_stencil_test_state::op_zfail_back = op_zfail_back;
    am_stencil_test_state::op_zpass_back = op_zpass_back;
}

void am_stencil_test_state::restore(am_stencil_test_state *old) {
    set(
        old->enabled,
        old->ref,
        old->read_mask,
        old->write_mask,
        old->func_front,
        old->op_fail_front,
        old->op_zfail_front,
        old->op_zpass_front,
        old->func_back,
        old->op_fail_back,
        old->op_zfail_back,
        old->op_zpass_back);
}

void am_stencil_test_state::bind(am_render_state *rstate, bool force) {
    if (force || enabled != rstate->bound_stencil_test_state.enabled) {
        am_set_stencil_test_enabled(enabled);
        rstate->bound_stencil_test_state.enabled = enabled;
    }
    if (force || 
        func_front != rstate->bound_stencil_test_state.func_front ||
        func_back != rstate->bound_stencil_test_state.func_back ||
        ref != rstate->bound_stencil_test_state.ref ||
        read_mask != rstate->bound_stencil_test_state.read_mask) 
    {
        am_set_stencil_func(ref, read_mask, func_front, func_back);
        rstate->bound_stencil_test_state.func_front = func_front;
        rstate->bound_stencil_test_state.func_back = func_back;
        rstate->bound_stencil_test_state.ref = ref;
        rstate->bound_stencil_test_state.read_mask = read_mask;
    }
    if (force || 
        op_fail_back != rstate->bound_stencil_test_state.op_fail_back ||
        op_zfail_back != rstate->bound_stencil_test_state.op_zfail_back ||
        op_zpass_back != rstate->bound_stencil_test_state.op_zpass_back) 
    {
        am_set_stencil_op(AM_STENCIL_FACE_BACK, op_fail_back, op_zfail_back, op_zpass_back);
        rstate->bound_stencil_test_state.op_fail_back = op_fail_back;
        rstate->bound_stencil_test_state.op_zfail_back = op_zfail_back;
        rstate->bound_stencil_test_state.op_zpass_back = op_zpass_back;
    }
    if (force || 
        op_fail_front != rstate->bound_stencil_test_state.op_fail_front ||
        op_zfail_front != rstate->bound_stencil_test_state.op_zfail_front ||
        op_zpass_front != rstate->bound_stencil_test_state.op_zpass_front) 
    {
        am_set_stencil_op(AM_STENCIL_FACE_FRONT, op_fail_front, op_zfail_front, op_zpass_front);
        rstate->bound_stencil_test_state.op_fail_front = op_fail_front;
        rstate->bound_stencil_test_state.op_zfail_front = op_zfail_front;
        rstate->bound_stencil_test_state.op_zpass_front = op_zpass_front;
    }
    if (force ||
        write_mask != rstate->bound_stencil_test_state.write_mask)
    {
        am_set_framebuffer_stencil_mask(write_mask);
        rstate->bound_stencil_test_state.write_mask = write_mask;
    }
}

am_cull_face_state::am_cull_face_state() {
    enabled = false;
    side = AM_CULL_FACE_BACK;
}

void am_cull_face_state::set(bool enabled, am_cull_face_side side) {
    am_cull_face_state::enabled = enabled;
    am_cull_face_state::side = side;
}

void am_cull_face_state::restore(am_cull_face_state *old) {
    set(old->enabled, old->side);
}

void am_cull_face_state::bind(am_render_state *rstate, bool force) {
    if (force || enabled != rstate->bound_cull_face_state.enabled) {
        am_set_cull_face_enabled(enabled);
        rstate->bound_cull_face_state.enabled = enabled;
    }
    if (force || side != rstate->bound_cull_face_state.side) {
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
    constant_r      = 1.0f;
    constant_g      = 1.0f;
    constant_b      = 1.0f;
    constant_a      = 1.0f;
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
            constant_r      = 1.0f;
            constant_g      = 1.0f;
            constant_b      = 1.0f;
            constant_a      = 1.0f;
            break;
        case AM_BLEND_MODE_ALPHA:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_ADD;
            equation_alpha  = AM_BLEND_EQUATION_ADD;
            sfactor_rgb     = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            sfactor_alpha   = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            constant_r      = 1.0f;
            constant_g      = 1.0f;
            constant_b      = 1.0f;
            constant_a      = 1.0f;
            break;
        case AM_BLEND_MODE_PREMULT:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_ADD;
            equation_alpha  = AM_BLEND_EQUATION_ADD;
            sfactor_rgb     = AM_BLEND_SFACTOR_ONE;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            sfactor_alpha   = AM_BLEND_SFACTOR_ONE;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
            constant_r      = 1.0f;
            constant_g      = 1.0f;
            constant_b      = 1.0f;
            constant_a      = 1.0f;
            break;
        case AM_BLEND_MODE_ADD:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_ADD;
            equation_alpha  = AM_BLEND_EQUATION_ADD;
            sfactor_rgb     = AM_BLEND_SFACTOR_ONE;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE;
            sfactor_alpha   = AM_BLEND_SFACTOR_ONE;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE;
            constant_r      = 1.0f;
            constant_g      = 1.0f;
            constant_b      = 1.0f;
            constant_a      = 1.0f;
            break;
        case AM_BLEND_MODE_SUBTRACT:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_REVERSE_SUBTRACT;
            equation_alpha  = AM_BLEND_EQUATION_REVERSE_SUBTRACT;
            sfactor_rgb     = AM_BLEND_SFACTOR_ONE;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE;
            sfactor_alpha   = AM_BLEND_SFACTOR_ONE;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE;
            constant_r      = 1.0f;
            constant_g      = 1.0f;
            constant_b      = 1.0f;
            constant_a      = 1.0f;
            break;
        case AM_BLEND_MODE_ADD_ALPHA:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_ADD;
            equation_alpha  = AM_BLEND_EQUATION_ADD;
            sfactor_rgb     = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE;
            sfactor_alpha   = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE;
            constant_r      = 1.0f;
            constant_g      = 1.0f;
            constant_b      = 1.0f;
            constant_a      = 1.0f;
            break;
        case AM_BLEND_MODE_SUBTRACT_ALPHA:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_REVERSE_SUBTRACT;
            equation_alpha  = AM_BLEND_EQUATION_REVERSE_SUBTRACT;
            sfactor_rgb     = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE;
            sfactor_alpha   = AM_BLEND_SFACTOR_SRC_ALPHA;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE;
            constant_r      = 1.0f;
            constant_g      = 1.0f;
            constant_b      = 1.0f;
            constant_a      = 1.0f;
            break;
        case AM_BLEND_MODE_MULTIPLY:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_ADD;
            equation_alpha  = AM_BLEND_EQUATION_ADD;
            sfactor_rgb     = AM_BLEND_SFACTOR_DST_COLOR;
            dfactor_rgb     = AM_BLEND_DFACTOR_ZERO;
            sfactor_alpha   = AM_BLEND_SFACTOR_DST_COLOR;
            dfactor_alpha   = AM_BLEND_DFACTOR_ZERO;
            constant_r      = 1.0f;
            constant_g      = 1.0f;
            constant_b      = 1.0f;
            constant_a      = 1.0f;
            break;
        case AM_BLEND_MODE_INVERT:
            enabled         = true; 
            equation_rgb    = AM_BLEND_EQUATION_SUBTRACT;
            equation_alpha  = AM_BLEND_EQUATION_SUBTRACT;
            sfactor_rgb     = AM_BLEND_SFACTOR_ONE;
            dfactor_rgb     = AM_BLEND_DFACTOR_ONE;
            sfactor_alpha   = AM_BLEND_SFACTOR_ONE;
            dfactor_alpha   = AM_BLEND_DFACTOR_ONE;
            constant_r      = 1.0f;
            constant_g      = 1.0f;
            constant_b      = 1.0f;
            constant_a      = 1.0f;
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
    float             constant_r,
    float             constant_g,
    float             constant_b,
    float             constant_a)
{
    am_blend_state::enabled         = enabled       ;
    am_blend_state::equation_rgb    = equation_rgb  ;
    am_blend_state::equation_alpha  = equation_alpha;
    am_blend_state::sfactor_rgb     = sfactor_rgb   ;
    am_blend_state::dfactor_rgb     = dfactor_rgb   ;
    am_blend_state::sfactor_alpha   = sfactor_alpha ;
    am_blend_state::dfactor_alpha   = dfactor_alpha ;
    am_blend_state::constant_r      = constant_r       ;
    am_blend_state::constant_g      = constant_g       ;
    am_blend_state::constant_b      = constant_b       ;
    am_blend_state::constant_a      = constant_a       ;
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
        old->constant_r    ,
        old->constant_g    ,
        old->constant_b    ,
        old->constant_a
    );
}

void am_blend_state::bind(am_render_state *rstate, bool force) {
    am_blend_state *bound = &rstate->bound_blend_state;
    if (force || enabled != bound->enabled) {
        am_set_blend_enabled(enabled);
        bound->enabled = enabled;
    }
    if (!force && !enabled) return;
    if (force ||
        equation_rgb != bound->equation_rgb ||
        equation_alpha != bound->equation_alpha)
    {
        am_set_blend_equation(equation_rgb, equation_alpha);
        bound->equation_rgb = equation_rgb;
        bound->equation_alpha = equation_alpha;
    }
    if (force ||
        sfactor_rgb != bound->sfactor_rgb ||
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
    if (force ||
        constant_r != bound->constant_r ||
        constant_g != bound->constant_g ||
        constant_b != bound->constant_b ||
        constant_a != bound->constant_a)
    {
        am_set_blend_color(constant_r, constant_g, constant_b, constant_a);
        bound->constant_r = constant_r;
        bound->constant_g = constant_g;
        bound->constant_b = constant_b;
        bound->constant_a = constant_a;
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
    active_viewport_state.bind(this, false);
    active_scissor_test_state.bind(this, false);
    active_color_mask_state.bind(this, false);
    active_depth_test_state.bind(this, false);
    active_stencil_test_state.bind(this, false);
    active_cull_face_state.bind(this, false);
    active_blend_state.bind(this, false);
    bind_active_program();
    if (!bind_active_program_params()) {
        return false;
    }
    enable_vaas(active_program->num_vaas);
    return true;
}

static void setup(am_render_state *rstate, am_framebuffer_id fb,
    bool clear, glm::dvec4 clear_color, int stencil_clear_val,
    int x, int y, int w, int h, int fbw, int fbh, glm::dmat4 proj,
    bool has_depthbuffer)
{
    if (fb != 0) {
        // default framebuffer should be bound by backend-specific code
        am_bind_framebuffer(fb);
    }

    rstate->active_viewport_state.set(x, y, w, h);
    rstate->active_viewport_state.bind(rstate, true);

    rstate->active_color_mask_state.set(true, true, true, true);
    rstate->active_color_mask_state.bind(rstate, true);

    rstate->active_depth_test_state.set(has_depthbuffer, has_depthbuffer,
        has_depthbuffer ? AM_DEPTH_FUNC_LESS : AM_DEPTH_FUNC_ALWAYS);
    rstate->active_depth_test_state.bind(rstate, true);

    rstate->active_stencil_test_state.set(
        false,
        0,
        255,
        255,
        AM_STENCIL_FUNC_ALWAYS,
        AM_STENCIL_OP_KEEP,
        AM_STENCIL_OP_KEEP,
        AM_STENCIL_OP_KEEP,
        AM_STENCIL_FUNC_ALWAYS,
        AM_STENCIL_OP_KEEP,
        AM_STENCIL_OP_KEEP,
        AM_STENCIL_OP_KEEP);
    rstate->active_stencil_test_state.bind(rstate, true);

    bool is_margin = !(x == 0 && y == 0 && w == fbw && h == fbh);

    if (clear && is_margin) {
        // do preliminary clear to black without scissor test
        // to make sure margins are black.
        am_set_scissor_test_enabled(false);
        am_set_framebuffer_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
        am_clear_framebuffer(true, true, true);
    }

    bool scissor_enabled = is_margin;
    rstate->active_scissor_test_state.set(scissor_enabled, x, y, w, h);
    rstate->active_scissor_test_state.bind(rstate, true);

    if (clear && (!is_margin ||
        clear_color.r != 0.0 || clear_color.g != 0.0 ||
        clear_color.b != 0.0 || clear_color.a != 1.0))
    {
        am_set_framebuffer_clear_color(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        am_set_framebuffer_clear_stencil_val(stencil_clear_val);
        am_clear_framebuffer(true, true, true);
    }

    rstate->param_name_map[rstate->projection_param_index].value.set_mat4(proj);
    rstate->param_name_map[rstate->modelview_param_index].value.set_mat4(glm::dmat4(1.0));
}

void am_render_state::do_render(am_scene_node **roots, int num_roots, am_framebuffer_id fb,
    bool clear, glm::dvec4 clear_color, int stencil_clear_val, int x, int y, int w, int h, int fbw, int fbh,
    glm::dmat4 proj, bool has_depthbuffer)
{
    setup(this, fb, clear, clear_color, stencil_clear_val, x, y, w, h, fbw, fbh, proj, has_depthbuffer);

    for (int i = 0; i < num_roots; i++) {
        am_scene_node *root = roots[i];
        if (root == NULL || am_node_hidden(root)) continue;
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
    am_gl_end_framebuffer_render();

    render_count++;

    assert(active_program == NULL);
    assert(next_free_texture_unit == 0);
}

static bool check_pass(am_render_state *rstate) {
    return (rstate->pass & rstate->pass_mask);
}

void am_render_state::draw_arrays(am_draw_mode mode, int first, int draw_array_count) {
    if (draw_array_count == 0) return;
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
    if (count == 0) return;
    if (!check_pass(this)) return;
    if (active_program == NULL) {
        am_log1("%s", "WARNING: ignoring draw, "
            "because no shader program has been bound");
        return;
    }
    if (!update_state()) {
        // warning would already have been emitted
        return;
    }
    if (validate_active_program(mode) && indices_view->buffer->elembuf != NULL) {
        indices_view->buffer->update_if_dirty();
        indices_view->update_max_elem_if_required();
        if (max_draw_array_size == INT_MAX) {
            am_log1("%s", "WARNING: ignoring draw, "
                "because no attribute arrays have been bound");
            count = 0;
        } else if (indices_view->size > 0 && (int)indices_view->max_elem >= max_draw_array_size) {
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
            am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, indices_view->buffer->elembuf->get_latest_id());
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

    modelview_param_index = -1;
    projection_param_index = -1;

    render_count = 0;
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

static void get_draw_node_primitive(lua_State *L, void *obj) {
    am_draw_node *node = (am_draw_node*)obj;
    am_push_enum(L, am_draw_mode, node->mode);
}

static void set_draw_node_primitive(lua_State *L, void *obj) {
    am_draw_node *node = (am_draw_node*)obj;
    node->mode = am_get_enum(L, am_draw_mode, 3);
}

static am_property draw_node_primitive_property = {get_draw_node_primitive, set_draw_node_primitive};

static void set_indices(lua_State *L, am_draw_node *node, int idx) {
    am_buffer_view *indices_view = am_get_userdata(L, am_buffer_view, idx);
    switch (indices_view->type) {
        case AM_VIEW_TYPE_U16:
            if (indices_view->stride != 2) {
                luaL_error(L, "ushort array must have stride 2 when used with draw_elements");
            }
            node->type = AM_ELEMENT_TYPE_USHORT;
            break;
        case AM_VIEW_TYPE_U16E:
            if (indices_view->stride != 2) {
                luaL_error(L, "ushort_elem array must have stride 2 when used with draw_elements");
            }
            node->type = AM_ELEMENT_TYPE_USHORT;
            break;
        case AM_VIEW_TYPE_U32:
            if (indices_view->stride != 4) {
                luaL_error(L, "uint array must have stride 4 when used with draw_elements");
            }
            node->type = AM_ELEMENT_TYPE_UINT;
            break;
        case AM_VIEW_TYPE_U32E:
            if (indices_view->stride != 4) {
                luaL_error(L, "uint_elem array must have stride 4 when used with draw_elements");
            }
            node->type = AM_ELEMENT_TYPE_UINT;
            break;
        default:
            luaL_error(L, "only u16(e) and u32(e) views can be used as element arrays");
    }
    node->indices_view = indices_view;
    if (node->view_ref == LUA_NOREF) {
        node->view_ref = node->ref(L, idx);
    } else {
        node->reref(L, node->view_ref, idx);
    }
    if (indices_view->buffer->elembuf == NULL) {
        indices_view->buffer->create_elembuf(L);
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

    am_register_property(L, "primitive", &draw_node_primitive_property);
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
    if (rstate->param_name_map != NULL) free(rstate->param_name_map);
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
    rstate->modelview_param_index = am_lookup_param_name(L, -1);
    lua_pop(L, 1); // modelview name string

    lua_pushstring(L, am_conf_default_projection_matrix_name);
    rstate->projection_param_index = am_lookup_param_name(L, -1);
    lua_pop(L, 1); // projection name string
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

#if defined(AM_ANDROID)
    // only re-create global render state on android, since the gl context 
    // would have been lost there.
    if (am_global_render_state != NULL) {
        if (am_global_render_state->param_name_map != NULL) {
            free(am_global_render_state->param_name_map);
            am_global_render_state->param_name_map = NULL;
        }
        delete am_global_render_state;
        am_global_render_state = NULL;
    }
#endif
    if (am_global_render_state == NULL) {
        am_global_render_state = new am_render_state();
    }
    init_param_name_map(am_global_render_state, L);
}
