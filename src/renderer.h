struct am_program_param;

struct am_blend_config {
    bool                blend_enabled;
    am_blend_equation   blend_equation_rgb;
    am_blend_equation   blend_equation_alpha;
    am_blend_sfactor    blend_sfactor_rgb;
    am_blend_dfactor    blend_dfactor_rgb;
    am_blend_sfactor    blend_sfactor_alpha;
    am_blend_dfactor    blend_dfactor_alpha;
    float               blend_color_r;
    float               blend_color_g;
    float               blend_color_b;
    float               blend_color_a;
};

struct am_depth_test_config {
    bool                depth_test_enabled;
    am_depth_func       depth_func;
    float               depth_range_near;
    float               depth_range_far;
};

struct am_stencil_test_config {
    bool                stencil_test_enabled;
    am_stencil_func     stencil_func_back;
    am_glint            stencil_ref_back;
    am_gluint           stencil_mask_back;
    am_stencil_func     stencil_func_front;
    am_glint            stencil_ref_front;
    am_gluint           stencil_mask_front;
    am_stencil_op       stencil_op_fail_front;
    am_stencil_op       stencil_op_zfail_front;
    am_stencil_op       stencil_op_zpass_front;
    am_stencil_op       stencil_op_fail_back;
    am_stencil_op       stencil_op_zfail_back;
    am_stencil_op       stencil_op_zpass_back;
};

struct am_scissor_test_config {
    bool                scissor_test_enabled;
    int                 scissor_x;
    int                 scissor_y;
    int                 scissor_w;
    int                 scissor_h;
};

struct am_sample_coverage_config {
    bool                sample_alpha_to_coverage_enabled;
    bool                sample_coverage_enabled;
    float               sample_coverage_value;
    bool                sample_coverage_invert;
};

struct am_viewport_config {
    int                 viewport_x;
    int                 viewport_y;
    int                 viewport_w;
    int                 viewport_h;
};

struct am_facecull_config {
    am_face_winding     front_face_winding;
    bool                cull_face_enabled;
    am_cull_face_side   cull_face_side;
};

struct am_polygon_offset_config {
    bool                polygon_offset_fill_enabled;
    float               polygon_offset_factor;
    float               polygon_offset_units;
};

struct am_framebuffer_config {
    float               clear_color_r;
    float               clear_color_g;
    float               clear_color_b;
    float               clear_color_a;
    float               clear_depth;
    am_glint            clear_stencil_val;
    bool                color_mask_r;
    bool                color_mask_g;
    bool                color_mask_b;
    bool                color_mask_a;
    bool                depth_mask;
    am_gluint           stencil_mask_front;
    am_gluint           stencil_mask_back;
};

struct am_line_config {
    float               line_width;
};

struct am_dither_config {
    bool                dither_enabled;
};

struct am_render_state {
    bool            framebuffer_id_dirty;
    bool            blend_config_dirty;
    bool            depth_test_config_dirty;
    bool            stencil_test_config_dirty;
    bool            scissor_test_config_dirty;
    bool            sample_coverage_config_dirty;
    bool            viewport_config_dirty;
    bool            facecull_config_dirty;
    bool            polygon_offset_config_dirty;
    bool            framebuffer_config_dirty;
    bool            line_config_dirty;
    bool            dither_config_dirty;

    am_buffer_id    active_indices_id;
    int             active_indices_max_value;
    int             active_indices_max_size;
    int             active_indices_offset;
    int             active_indices_count;
    am_element_index_type active_indices_type;

    int             active_attribute_array_max_size;
    int             active_attribute_array_first;
    int             active_attribute_array_count;

    am_draw_mode    active_draw_mode;

    am_program_id   active_program_id;
    am_program_id   bound_program_id;
    am_program_param **active_program_params;

    void draw();
    bool validate_active_program();
    void bind_active_program();
    void bind_active_program_params();
    void bind_active_indices();
};

struct am_program_param {
    virtual void bind(am_render_state *rstate) = 0;
};
