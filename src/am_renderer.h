struct am_program;
struct am_program_param;
struct am_program_param_name_slot;
struct am_program_param_value;

struct am_viewport_state {
    int                     x;
    int                     y;
    int                     w;
    int                     h;

    am_viewport_state();
    void set(int x, int y, int w, int h);
    void restore(am_viewport_state *old);
    void bind(am_render_state *rstate, bool force);
};

struct am_color_mask_state {
    bool                    r;
    bool                    g;
    bool                    b;
    bool                    a;

    am_color_mask_state();
    void set(bool r, bool g, bool b, bool a);
    void restore(am_color_mask_state *old);
    void bind(am_render_state *rstate, bool force);
};

struct am_depth_test_state {
    bool                    test_enabled;
    bool                    mask_enabled;
    am_depth_func           func;

    am_depth_test_state();
    void set(bool test_enabled, bool mask_enabled, am_depth_func func);
    void restore(am_depth_test_state *old);
    void bind(am_render_state *rstate, bool force);
};

struct am_stencil_test_state {
    bool                    enabled;
    am_glint                ref;
    am_gluint               read_mask;
    am_gluint               write_mask;
    am_stencil_func         func_front;
    am_stencil_op           op_fail_front;
    am_stencil_op           op_zfail_front;
    am_stencil_op           op_zpass_front;
    am_stencil_func         func_back;
    am_stencil_op           op_fail_back;
    am_stencil_op           op_zfail_back;
    am_stencil_op           op_zpass_back;

    am_stencil_test_state();
    void set(
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
        am_stencil_op           op_zpass_back
    );
    void restore(am_stencil_test_state *old);
    void bind(am_render_state *rstate, bool force);
};

struct am_cull_face_state {
    bool                    enabled;
    am_cull_face_side       side;

    am_cull_face_state();
    void set(bool enabled, am_cull_face_side side);
    void restore(am_cull_face_state *old);
    void bind(am_render_state *rstate, bool force);
};

enum am_blend_mode {
    AM_BLEND_MODE_OFF,
    AM_BLEND_MODE_ALPHA,
    AM_BLEND_MODE_PREMULT,
    AM_BLEND_MODE_ADD,
    AM_BLEND_MODE_SUBTRACT,
    AM_BLEND_MODE_ADD_ALPHA,
    AM_BLEND_MODE_SUBTRACT_ALPHA,
    AM_BLEND_MODE_MULTIPLY,
    AM_BLEND_MODE_INVERT,
};

struct am_blend_state {
    bool                    enabled;
    am_blend_equation       equation_rgb;
    am_blend_equation       equation_alpha;
    am_blend_sfactor        sfactor_rgb;
    am_blend_dfactor        dfactor_rgb;
    am_blend_sfactor        sfactor_alpha;
    am_blend_dfactor        dfactor_alpha;
    float                   constant_r;
    float                   constant_g;
    float                   constant_b;
    float                   constant_a;

    am_blend_state();
    void set_mode(am_blend_mode mode);
    void set(bool enabled,
        am_blend_equation equation_rgb,
        am_blend_equation equation_alpha,
        am_blend_sfactor  sfactor_rgb,
        am_blend_dfactor  dfactor_rgb,
        am_blend_sfactor  sfactor_alpha,
        am_blend_dfactor  dfactor_alpha,
        float             constant_r,
        float             constant_g,
        float             constant_b,
        float             constant_a);
    void restore(am_blend_state *old);
    void bind(am_render_state *rstate, bool force);
};

struct am_scissor_test_state {
    bool                    enabled;
    int                     x;
    int                     y;
    int                     w;
    int                     h;
    am_scissor_test_state();
    void set(bool enabled, int x, int y, int w, int h);
    void restore(am_scissor_test_state *old);
    void bind(am_render_state *rstate, bool force);
};

struct am_sample_coverage_state {
    bool                    sample_alpha_to_coverage_enabled;
    bool                    sample_coverage_enabled;
    float                   sample_coverage_value;
    bool                    sample_coverage_invert;
};

struct am_polygon_offset_state {
    bool                    fill_enabled;
    float                   factor;
    float                   units;
};

struct am_line_state {
    float                   line_width;
};

struct am_dither_state {
    bool                    enabled;
};

struct am_render_state {
    uint32_t                pass;
    uint32_t                next_pass;
    uint32_t                pass_mask;

    am_viewport_state       bound_viewport_state;
    am_viewport_state       active_viewport_state;
    am_scissor_test_state   bound_scissor_test_state;
    am_scissor_test_state   active_scissor_test_state;
    am_color_mask_state     bound_color_mask_state;
    am_color_mask_state     active_color_mask_state;
    am_depth_test_state     bound_depth_test_state;
    am_depth_test_state     active_depth_test_state;
    am_stencil_test_state   bound_stencil_test_state;
    am_stencil_test_state   active_stencil_test_state;
    am_cull_face_state      bound_cull_face_state;
    am_cull_face_state      active_cull_face_state;
    am_blend_state          bound_blend_state;
    am_blend_state          active_blend_state;

    int                     max_draw_array_size;

    int                     num_enabled_vaas; // num enabled vertex attribute arrays
    am_program_id           bound_program_id;
    am_program              *active_program;
    int                     param_name_map_capacity;
    am_program_param_name_slot *param_name_map;

    int                     next_free_texture_unit;

    int                     modelview_param_index;
    int                     projection_param_index;

    uint32_t                render_count;

    am_render_state();

    void draw_arrays(am_draw_mode mode, int first, int count);
    void draw_elements(am_draw_mode mode, int first, int count,
        am_buffer_view *indices_view, am_element_index_type type);
    bool validate_active_program(am_draw_mode mode);
    void bind_active_program();
    bool bind_active_program_params();
    bool update_state();
    void enable_vaas(int n);
    void do_render(am_scene_node **roots, int num_roots, am_framebuffer_id fb, 
        bool clear, glm::dvec4 clear_color, int stencil_clear_val,
        int x, int y, int w, int h, int fbw, int fbh, glm::dmat4 proj, bool has_depthbuffer);
};

struct am_draw_node : am_scene_node {
    int first;
    int count;
    am_draw_mode mode;
    am_element_index_type type;
    am_buffer_view *indices_view;
    int view_ref;

    am_draw_node();
    virtual void render(am_render_state *rstate);
};

struct am_pass_filter_node : am_scene_node {
    uint32_t mask;
    virtual void render(am_render_state *rstate);
};

extern am_render_state *am_global_render_state;

void am_open_renderer_module(lua_State *L);
