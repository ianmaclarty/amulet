struct am_stencil_test_node : am_scene_node {
    bool                    enabled;
    am_glint                ref;
    am_gluint               read_mask;
    am_gluint               write_mask;
    am_stencil_func         func_back;
    am_stencil_func         func_front;
    am_stencil_op           op_fail_front;
    am_stencil_op           op_zfail_front;
    am_stencil_op           op_zpass_front;
    am_stencil_op           op_fail_back;
    am_stencil_op           op_zfail_back;
    am_stencil_op           op_zpass_back;

    virtual void render(am_render_state *rstate);
};

void am_open_stencilbuffer_module(lua_State *L);
