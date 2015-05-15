struct am_program;
struct am_program_param;

struct am_viewport_state {
    bool                    dirty;
    int                     x;
    int                     y;
    int                     w;
    int                     h;

    void set(int x, int y, int w, int h);
    void update();
};

struct am_depth_test_state {
    bool                    dirty;
    bool                    test_enabled;
    bool                    mask_enabled;
    am_depth_func           func;

    void set(bool test_enabled, bool mask_enabled, am_depth_func func);
    void restore(am_depth_test_state *old);
    void update();
};

struct am_cull_face_state {
    bool                    dirty;
    am_face_winding         winding;
    bool                    enabled;
    am_cull_face_side       side;

    void set(bool enabled, am_face_winding winding, am_cull_face_side side);
    void restore(am_cull_face_state *old);
    void update();
};

struct am_blend_state {
    bool                    enabled;
    am_blend_equation       equation_rgb;
    am_blend_equation       equation_alpha;
    am_blend_sfactor        sfactor_rgb;
    am_blend_dfactor        dfactor_rgb;
    am_blend_sfactor        sfactor_alpha;
    am_blend_dfactor        dfactor_alpha;
    float                   color_r;
    float                   color_g;
    float                   color_b;
    float                   color_a;
};

struct am_stencil_test_state {
    bool                    enabled;
    am_stencil_func         func_back;
    am_glint                ref_back;
    am_gluint               mask_back;
    am_stencil_func         func_front;
    am_glint                ref_front;
    am_gluint               mask_front;
    am_stencil_op           op_fail_front;
    am_stencil_op           op_zfail_front;
    am_stencil_op           op_zpass_front;
    am_stencil_op           op_fail_back;
    am_stencil_op           op_zfail_back;
    am_stencil_op           op_zpass_back;
};

struct am_scissor_test_state {
    bool                    enabled;
    int                     x;
    int                     y;
    int                     w;
    int                     h;
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

struct am_framebuffer_state {
    float                   clear_color_r;
    float                   clear_color_g;
    float                   clear_color_b;
    float                   clear_color_a;
    float                   clear_depth;
    am_glint                clear_stencil_val;
    bool                    color_mask_r;
    bool                    color_mask_g;
    bool                    color_mask_b;
    bool                    color_mask_a;
    bool                    depth_mask;
    am_gluint               stencil_mask_front;
    am_gluint               stencil_mask_back;
};

struct am_line_state {
    float                   line_width;
};

struct am_dither_state {
    bool                    enabled;
};

struct am_render_state {
    am_viewport_state       viewport_state;
    am_depth_test_state     depth_test_state;
    am_cull_face_state      cull_face_state;

    int                     max_draw_array_size;

    am_program_id           bound_program_id;
    am_program              *active_program;

    int                     next_free_texture_unit;

    am_render_state();
    virtual ~am_render_state();

    void draw_arrays(am_draw_mode mode, int first, int count);
    void draw_elements(am_draw_mode mode, int first, int count,
        am_buffer_view *indices_view, am_element_index_type type);
    bool validate_active_program(am_draw_mode mode);
    void bind_active_program();
    void bind_active_program_params();
    void update_state();
    void setup(am_framebuffer_id fb, bool clear, int w, int h, bool has_depthbuffer);
};

struct am_draw_arrays_node : am_scene_node {
    int first;
    int count;
    am_draw_mode mode;

    virtual void render(am_render_state *rstate);
};

struct am_draw_elements_node : am_scene_node {
    int first;
    int count;
    am_draw_mode mode;
    am_element_index_type type;
    am_buffer_view *indices_view;
    int view_ref;

    am_draw_elements_node();
    virtual void render(am_render_state *rstate);
};

extern am_render_state am_global_render_state;

void am_open_renderer_module(lua_State *L);
