struct am_translate_node : am_scene_node {
    am_param_name_id name;
    glm::vec3 v;
    virtual void render(am_render_state *rstate);
    virtual int specialized_index(lua_State *l);
    virtual int specialized_newindex(lua_State *l);
};

struct am_scale_node : am_scene_node {
    am_param_name_id name;
    glm::vec3 v;
    virtual void render(am_render_state *rstate);
    virtual int specialized_index(lua_State *l);
    virtual int specialized_newindex(lua_State *l);
};

struct am_rotate_node : am_scene_node {
    am_param_name_id name;
    float angle;
    glm::vec3 about;
    virtual void render(am_render_state *rstate);
    virtual int specialized_index(lua_State *l);
    virtual int specialized_newindex(lua_State *l);
};

struct am_mult_mat4_node : am_scene_node {
    am_param_name_id name;
    glm::mat4 mat;
    virtual void render(am_render_state *rstate);
};

int am_create_translate_node(lua_State *L);
int am_create_scale_node(lua_State *L);
int am_create_rotate_node(lua_State *L);
int am_create_mult_mat4_node(lua_State *L);

void am_open_transforms_module(lua_State *L);
