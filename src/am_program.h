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

    void set_float(float f) {
        type = AM_PROGRAM_PARAM_CLIENT_TYPE_1F;
        value.f = f;
    }
    void set_vec2(glm::vec2 v2) {
        type = AM_PROGRAM_PARAM_CLIENT_TYPE_2F;
        memcpy(&value.v2[0], glm::value_ptr(v2), sizeof(glm::vec2));
    }
    void set_vec3(glm::vec3 v3) {
        type = AM_PROGRAM_PARAM_CLIENT_TYPE_3F;
        memcpy(&value.v3[0], glm::value_ptr(v3), sizeof(glm::vec3));
    }
    void set_vec4(glm::vec4 v4) {
        type = AM_PROGRAM_PARAM_CLIENT_TYPE_4F;
        memcpy(&value.v4[0], glm::value_ptr(v4), sizeof(glm::vec4));
    }
    void set_mat2(glm::mat2 m2) {
        type = AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2;
        memcpy(&value.m2[0], glm::value_ptr(m2), sizeof(glm::mat2));
    }
    void set_mat3(glm::mat3 m3) {
        type = AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3;
        memcpy(&value.m3[0], glm::value_ptr(m3), sizeof(glm::mat3));
    }
    void set_mat4(glm::mat4 m4) {
        type = AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4;
        memcpy(&value.m4[0], glm::value_ptr(m4), sizeof(glm::mat4));
    }
    void set_arr(am_buffer_view *arr) {
        type = AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY;
        value.arr = arr;
    }
    void set_samp2d(am_texture2d *tex) {
        type = AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D;
        value.sampler2d.texture = tex;
        value.sampler2d.texture_unit = -1; // assigned when rendering
    }
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

    bool bind(am_render_state *rstate);
};

struct am_program : am_nonatomic_userdata {
    am_program_id program_id;
    int num_params;
    bool sets_point_size;
    am_program_param *params;
    int num_vaas; // number of vertex attribute arrays
};

struct am_program_node : am_scene_node {
    am_program *program;
    int program_ref;

    virtual void render(am_render_state *rstate);
};

struct am_bind_node : am_scene_node {
    int num_params;
    am_param_name_id *names;
    am_program_param_value *values;
    int *refs; // refs for array or texture params

    virtual void render(am_render_state *rstate);
};

struct am_read_param_node : am_scene_node {
    am_param_name_id name;
    am_program_param_value value;
    virtual void render(am_render_state *rstate);
};

void am_open_program_module(lua_State *L);

const char *am_program_param_type_name(am_program_param_type t);
const char *am_program_param_client_type_name(am_program_param_name_slot *slot);
