struct am_depth_pass_node : am_scene_node {
    am_depth_func func;
    virtual void render(am_render_state *rstate);
};

int am_create_depth_pass_node(lua_State *L);

void am_open_depthbuffer_module(lua_State *L);