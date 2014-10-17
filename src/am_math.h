struct am_vec2 : am_userdata {
    glm::vec2 v;
};

struct am_vec3 : am_userdata {
    glm::vec3 v;
};

struct am_vec4 : am_userdata {
    glm::vec4 v;
};

struct am_mat2 : am_userdata {
    glm::mat2 m;
};

struct am_mat3 : am_userdata {
    glm::mat3 m;
};

struct am_mat4 : am_userdata {
    glm::mat4 m;
};

void am_open_math_module(lua_State *L);
