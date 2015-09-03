struct am_translate_node : am_scene_node {
    am_param_name_id name;
    am_vec3 *position;
    int position_ref;
    virtual void render(am_render_state *rstate);
};

struct am_scale_node : am_scene_node {
    am_param_name_id name;
    am_vec3 *scaling;
    int scaling_ref;
    virtual void render(am_render_state *rstate);
};

struct am_rotate_node : am_scene_node {
    am_param_name_id name;
    am_quat *rotation;
    int rotation_ref;
    virtual void render(am_render_state *rstate);
};

struct am_lookat_node : am_scene_node {
    am_param_name_id name;
    am_vec3 *eye;
    int eye_ref;
    am_vec3 *center;
    int center_ref;
    am_vec3 *up;
    int up_ref;
    virtual void render(am_render_state *rstate);
};

struct am_billboard_node : am_scene_node {
    am_param_name_id name;
    bool preserve_uniform_scaling;
    virtual void render(am_render_state *rstate);
};

int am_create_translate_node(lua_State *L);
int am_create_scale_node(lua_State *L);
int am_create_rotate_node(lua_State *L);
int am_create_lookat_node(lua_State *L);
int am_create_billboard_node(lua_State *L);

void am_open_transforms_module(lua_State *L);
