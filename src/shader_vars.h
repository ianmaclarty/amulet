struct am_shader_var {
    virtual bool translate(glm::vec3 &v) { return false; }
    virtual bool rotate(float angle, glm::vec3 &v) { return false; }
    virtual bool scale(glm::vec3 &v) { return false; }
    virtual bool vec4mul(glm::vec4 &v) { return false; }
    virtual bool fmul(float f) { return false; }
};

struct am_mat4_var : am_shader_var {
    glm::mat4 m;

    am_mat4_var();
    virtual bool translate(glm::vec3 &v);
    virtual bool rotate(float angle, glm::vec3 &v);
    virtual bool scale(glm::vec3 &v);
    virtual bool fmul(float f);
};

struct am_vec4_var : am_shader_var {
    glm::vec4 v;

    am_vec4_var();
    virtual bool vec4mul(glm::vec4 &v);
    virtual bool fmul(float f);
};

struct am_float_var : am_shader_var {
    float f;

    am_float_var();
    virtual bool fmul(float f);
};
