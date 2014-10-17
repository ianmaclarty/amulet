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
        glm::vec2 v2;
        glm::vec3 v3;
        glm::vec4 v4;
        glm::mat2 m2;
        glm::mat3 m3;
        glm::mat4 m4;
        am_buffer_view *arr;
    } value;
};

struct am_program_param_name_slot {
    am_program_param_value value;
    const char *name;
};

extern am_program_param_name_info *am_param_name_map;

void am_init_param_name_map(lua_State *L);
am_param_name_id am_lookup_param_name(lua_State *L, int name_idx);

struct am_program_param {
    am_program_param_type type;
    am_gluint index;
    am_program_param_name_slot *slot;

    void bind(am_render_state *rstate);
};

struct am_program : am_userdata {
    am_program_id program_id;
    int num_params;
    am_program_param *params;
};

void am_open_program_module(lua_State *L);
