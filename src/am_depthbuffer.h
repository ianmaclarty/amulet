struct am_depth_test_node : am_scene_node {
    am_depth_func func;
    bool mask_enabled;
    virtual void render(am_render_state *rstate);
};

void am_open_depthbuffer_module(lua_State *L);
