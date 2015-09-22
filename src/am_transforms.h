struct am_translate_node : am_scene_node {
    am_param_name_id name;
    glm::vec3 v;
    virtual void render(am_render_state *rstate);
};

struct am_scale_node : am_scene_node {
    am_param_name_id name;
    glm::vec3 v;
    virtual void render(am_render_state *rstate);
};

struct am_rotate_node : am_scene_node {
    am_param_name_id name;
    glm::quat rotation;
    float angle;
    glm::vec3 axis;
    virtual void render(am_render_state *rstate);
};

struct am_lookat_node : am_scene_node {
    am_param_name_id name;
    glm::vec3 eye;
    glm::vec3 center;
    glm::vec3 up;
    virtual void render(am_render_state *rstate);
};

struct am_billboard_node : am_scene_node {
    am_param_name_id name;
    bool preserve_uniform_scaling;
    virtual void render(am_render_state *rstate);
};

void am_open_transforms_module(lua_State *L);
