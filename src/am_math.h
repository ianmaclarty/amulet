struct am_vec2 {
    glm::dvec2 v;
    am_vec2() {}
};

struct am_vec3 {
    glm::dvec3 v;
    am_vec3() {}
};

struct am_vec4 {
    glm::dvec4 v;
    am_vec4() {}
};

struct am_mat2 {
    glm::dmat2 m;
    am_mat2() {}
};

struct am_mat3 {
    glm::dmat3 m;
    am_mat3() {}
};

struct am_mat4 {
    glm::dmat4 m;
    am_mat4() {}
};

struct am_quat {
    glm::dquat q;
    am_quat() {}
};

bool am_sphere_visible(glm::dmat4 &matrix, glm::dvec3 &center, double radius);
bool am_box_visible(glm::dmat4 &matrix, glm::dvec3 &min, glm::dvec3 &max);

void am_open_math_module(lua_State *L);
