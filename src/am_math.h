struct am_vec2 : am_userdata {
    glm::vec2 v;
    am_vec2() {}
    am_vec2(float *val) {
        v = glm::make_vec2(val);
    }
};

struct am_vec3 : am_userdata {
    glm::vec3 v;
    am_vec3() {}
    am_vec3(float *val) {
        v = glm::make_vec3(val);
    }
};

struct am_vec4 : am_userdata {
    glm::vec4 v;
    am_vec4() {}
    am_vec4(float *val) {
        v = glm::make_vec4(val);
    }
};

struct am_mat2 : am_userdata {
    glm::mat2 m;
    am_mat2() {}
    am_mat2(float *val) {
        m = glm::make_mat2(val);
    }
};

struct am_mat3 : am_userdata {
    glm::mat3 m;
    am_mat3() {}
    am_mat3(float *val) {
        m = glm::make_mat3(val);
    }
};

struct am_mat4 : am_userdata {
    glm::mat4 m;
    am_mat4() {}
    am_mat4(float *val) {
        m = glm::make_mat4(val);
    }
};

int am_vec2_index(lua_State *L, glm::vec2 *v);
int am_vec3_index(lua_State *L, glm::vec3 *v);
int am_vec4_index(lua_State *L, glm::vec4 *v);
int am_vec2_newindex(lua_State *L, glm::vec2 *v);
int am_vec3_newindex(lua_State *L, glm::vec3 *v);
int am_vec4_newindex(lua_State *L, glm::vec4 *v);

void am_read_vec2(lua_State *L, glm::vec2 *v, int start, int end);
void am_read_vec3(lua_State *L, glm::vec3 *v, int start, int end);
void am_read_vec4(lua_State *L, glm::vec4 *v, int start, int end);

void am_open_math_module(lua_State *L);
