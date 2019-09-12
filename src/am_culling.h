#define AM_MAX_CULL_SPHERE_NAMES 8
#define AM_MAX_CULL_BOX_NAMES 8

enum am_cull_face_mode {
    AM_CULL_FACE_MODE_FRONT,
    AM_CULL_FACE_MODE_BACK,
    AM_CULL_FACE_MODE_NONE,
};

struct am_cull_face_node : am_scene_node {
    am_cull_face_mode mode;
    virtual void render(am_render_state *rstate);
};

struct am_cull_sphere_node : am_scene_node {
    am_param_name_id names[AM_MAX_CULL_SPHERE_NAMES];
    int num_names;
    glm::dvec3 center;
    float radius;
    virtual void render(am_render_state *rstate);
};

struct am_cull_box_node : am_scene_node {
    am_param_name_id names[AM_MAX_CULL_BOX_NAMES];
    int num_names;
    glm::dvec3 min;
    glm::dvec3 max;
    virtual void render(am_render_state *rstate);
};

void am_open_culling_module(lua_State *L);
