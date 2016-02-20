struct am_vec2 {
    glm::dvec2 v;
    am_vec2() {}
    am_vec2(float *val) {
        v = glm::make_vec2(val);
    }
};

struct am_vec3 {
    glm::dvec3 v;
    am_vec3() {}
    am_vec3(float *val) {
        v = glm::make_vec3(val);
    }
};

struct am_vec4 {
    glm::dvec4 v;
    am_vec4() {}
    am_vec4(float *val) {
        v = glm::make_vec4(val);
    }
};

struct am_mat2 {
    glm::dmat2 m;
    am_mat2() {}
    am_mat2(float *val) {
        m = glm::make_mat2(val);
    }
};

struct am_mat3 {
    glm::dmat3 m;
    am_mat3() {}
    am_mat3(float *val) {
        m = glm::make_mat3(val);
    }
};

struct am_mat4 {
    glm::dmat4 m;
    am_mat4() {}
    am_mat4(float *val) {
        m = glm::make_mat4(val);
    }
};

struct am_quat {
    glm::dquat q;
    am_quat() {}
};

bool am_sphere_visible(glm::dmat4 &matrix, glm::dvec3 &center, double radius);

void am_open_math_module(lua_State *L);
