struct am_translate_node : am_scene_node {
    am_param_name_id name;
    glm::vec3 v;
    virtual void render(am_render_state *rstate);
    virtual int specialized_index(lua_State *l);
};

int am_create_translate_node(lua_State *L);
void am_open_transforms_module(lua_State *L);
