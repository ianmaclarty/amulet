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
    AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D,
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
    AM_PROGRAM_PARAM_UNIFORM_SAMPLER2D,
    AM_PROGRAM_PARAM_ATTRIBUTE_1F,
    AM_PROGRAM_PARAM_ATTRIBUTE_2F,
    AM_PROGRAM_PARAM_ATTRIBUTE_3F,
    AM_PROGRAM_PARAM_ATTRIBUTE_4F,
};

struct am_sampler2d_param_value {
    am_glint texture_unit;
    am_texture2d *texture;
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
        am_sampler2d_param_value sampler2d;
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
    am_gluint location;
    am_param_name_id name;

    void bind(am_render_state *rstate);
};

struct am_program : am_nonatomic_userdata {
    am_program_id program_id;
    int num_params;
    bool sets_point_size;
    am_program_param *params;
};

struct am_program_node : am_scene_node {
    am_program *program;
    int program_ref;

    virtual void render(am_render_state *rstate);
};

struct am_program_param_bind_value {
    am_program_param_client_type type;
    union {
        float f;
        am_vec2 *v2;
        am_vec3 *v3;
        am_vec4 *v4;
        am_mat2 *m2;
        am_mat3 *m3;
        am_mat4 *m4;
        am_buffer_view *arr;
        am_texture2d *texture;
    } value;
    int ref;
    am_param_name_id name;
};

struct am_bind_node : am_scene_node {
    int num_params;
    am_program_param_bind_value *values;

    virtual void render(am_render_state *rstate);
};

#define AM_READ_MAT_NODE_DECL(D)                                        \
struct am_read_mat##D##_node : am_scene_node {                          \
    am_param_name_id name;                                              \
    glm::mat##D m;                                                      \
    virtual void render(am_render_state *rstate);                       \
};

AM_READ_MAT_NODE_DECL(2)
AM_READ_MAT_NODE_DECL(3)
AM_READ_MAT_NODE_DECL(4)

int am_create_program_node(lua_State *L);
int am_create_bind_node(lua_State *L);
int am_create_read_mat2_node(lua_State *L);
int am_create_read_mat3_node(lua_State *L);
int am_create_read_mat4_node(lua_State *L);

void am_open_program_module(lua_State *L);

const char *am_program_param_type_name(am_program_param_type t);
const char *am_program_param_client_type_name(am_program_param_name_slot *slot);
