enum am_cull_face_mode {
    AM_CULL_FACE_MODE_FRONT,
    AM_CULL_FACE_MODE_BACK,
    AM_CULL_FACE_MODE_NONE,
};

struct am_cull_face_node : am_scene_node {
    am_cull_face_mode mode;
    virtual void render(am_render_state *rstate);
};

int am_create_cull_face_node(lua_State *L);

void am_open_culling_module(lua_State *L);
