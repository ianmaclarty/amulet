typedef int am_param_name_id;

enum am_program_param_client_type {
    AM_PROGRAM_PARAM_CLIENT_TYPE_1F,
    AM_PROGRAM_PARAM_CLIENT_TYPE_2F,
    AM_PROGRAM_PARAM_CLIENT_TYPE_3F,
    AM_PROGRAM_PARAM_CLIENT_TYPE_4F,
    AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2,
    AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3,
    AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4,
    AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY,
    AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED,
};

enum am_program_param_type {
    AM_PROGRAM_PARAM_UNIFORM_1F,
    AM_PROGRAM_PARAM_UNIFORM_2F,
    AM_PROGRAM_PARAM_UNIFORM_3F,
    AM_PROGRAM_PARAM_UNIFORM_4F,
    AM_PROGRAM_PARAM_UNIFORM_MAT2,
    AM_PROGRAM_PARAM_UNIFORM_MAT3,
    AM_PROGRAM_PARAM_UNIFORM_MAT4,
    AM_PROGRAM_PARAM_ATTRIBUTE_1F,
    AM_PROGRAM_PARAM_ATTRIBUTE_2F,
    AM_PROGRAM_PARAM_ATTRIBUTE_3F,
    AM_PROGRAM_PARAM_ATTRIBUTE_4F,
};

struct am_program_param_value {
    am_program_param_client_type type;
    union {
        float f;
        float v2[2];
        float v3[3];
        float v4[4];
        float m2[4];
        float m3[9];
        float m4[16];
        am_buffer_view *arr;
    } value;
};

struct am_program_param_name_slot {
    am_program_param_value value;
    const char *name;
};

extern am_program_param_name_slot *am_param_name_map;

void am_init_param_name_map(lua_State *L);
am_param_name_id am_lookup_param_name(lua_State *L, int name_idx);

struct am_program_param {
    am_program_param_type type;
    am_gluint index;
    am_param_name_id name;

    void bind(am_render_state *rstate);
};

struct am_program : am_userdata {
    am_program_id program_id;
    int num_params;
    am_program_param *params;
};

struct am_program_node : am_scene_node {
    am_program *program;
    int program_ref;

    virtual void render(am_render_state *rstate);
};

struct am_bind_array_node : am_scene_node {
    am_param_name_id name;
    am_buffer_view *arr;
    int arr_ref;

    virtual void render(am_render_state *rstate);
};

#define AM_BIND_MAT_NODE_DECL(D)                                        \
struct am_bind_mat##D##_node : am_scene_node {                          \
    am_param_name_id name;                                              \
    glm::mat##D m;                                                      \
    virtual void render(am_render_state *rstate);                       \
};

AM_BIND_MAT_NODE_DECL(2)
AM_BIND_MAT_NODE_DECL(3)
AM_BIND_MAT_NODE_DECL(4)

#define AM_BIND_VEC_NODE_DECL(D)                                        \
struct am_bind_vec##D##_node : am_scene_node {                          \
    am_param_name_id name;                                              \
    glm::vec##D v;                                                      \
    virtual void render(am_render_state *rstate);                       \
    virtual int specialized_index(lua_State *L);                        \
    virtual int specialized_newindex(lua_State *L);                     \
};

AM_BIND_VEC_NODE_DECL(2)
AM_BIND_VEC_NODE_DECL(3)
AM_BIND_VEC_NODE_DECL(4)

int am_create_program_node(lua_State *L);
int am_create_bind_array_node(lua_State *L);
int am_create_bind_vec2_node(lua_State *L);
int am_create_bind_vec3_node(lua_State *L);
int am_create_bind_vec4_node(lua_State *L);
int am_create_bind_mat2_node(lua_State *L);
int am_create_bind_mat3_node(lua_State *L);
int am_create_bind_mat4_node(lua_State *L);

void am_open_program_module(lua_State *L);
