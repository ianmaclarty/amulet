#include "amulet.h"

am_mat4_var::am_mat4_var() {
    m = glm::mat4(1.0f);
}

bool am_mat4_var::translate(glm::vec3 &v0) {
    m = glm::translate(m, v0);
    return true;
}

bool am_mat4_var::rotate(float angle0, glm::vec3 &v0) {
    m = glm::rotate(m, angle0, v0);
    return true;
}

bool am_mat4_var::scale(glm::vec3 &v0) {
    m = glm::scale(m, v0);
    return true;
}

bool am_mat4_var::fmul(float f0) {
    m = m * f0;
    return true;
}

am_vec4_var::am_vec4_var() {
    v = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
}

bool am_vec4_var::vec4mul(glm::vec4 &v0) {
    v = v * v0;
    return true;
}

bool am_vec4_var::fmul(float f0) {
    v = v * f0;
    return true;
}

am_float_var::am_float_var() {
    f = 0.0f;
}

bool am_float_var::fmul(float f0) {
    f = f * f0;
    return true;
}
