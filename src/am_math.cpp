#include "amulet.h"

static int vec2_new(lua_State *L);
static int vec2_index(lua_State *L);
static int vec2_call(lua_State *L);
static int vec2_add(lua_State *L);
static int vec2_sub(lua_State *L);
static int vec2_mul(lua_State *L);
static int vec2_div(lua_State *L);
static int vec2_unm(lua_State *L);
static int vec2_len(lua_State *L);
static int vec2_eq(lua_State *L);

static int vec3_new(lua_State *L);
static int vec3_index(lua_State *L);
static int vec3_call(lua_State *L);
static int vec3_add(lua_State *L);
static int vec3_sub(lua_State *L);
static int vec3_mul(lua_State *L);
static int vec3_div(lua_State *L);
static int vec3_unm(lua_State *L);
static int vec3_len(lua_State *L);
static int vec3_eq(lua_State *L);

static int vec4_new(lua_State *L);
static int vec4_index(lua_State *L);
static int vec4_call(lua_State *L);
static int vec4_add(lua_State *L);
static int vec4_sub(lua_State *L);
static int vec4_mul(lua_State *L);
static int vec4_div(lua_State *L);
static int vec4_unm(lua_State *L);
static int vec4_len(lua_State *L);
static int vec4_eq(lua_State *L);

static int mat2_new(lua_State *L);
static int mat2_index(lua_State *L);
static int mat2_add(lua_State *L);
static int mat2_sub(lua_State *L);
static int mat2_mul(lua_State *L);
static int mat2_div(lua_State *L);
static int mat2_unm(lua_State *L);
static int mat2_set(lua_State *L);
static int mat2_len(lua_State *L);
static int mat2_eq(lua_State *L);

static int mat3_new(lua_State *L);
static int mat3_index(lua_State *L);
static int mat3_add(lua_State *L);
static int mat3_sub(lua_State *L);
static int mat3_mul(lua_State *L);
static int mat3_div(lua_State *L);
static int mat3_unm(lua_State *L);
static int mat3_set(lua_State *L);
static int mat3_len(lua_State *L);
static int mat3_eq(lua_State *L);

static int mat4_new(lua_State *L);
static int mat4_index(lua_State *L);
static int mat4_add(lua_State *L);
static int mat4_sub(lua_State *L);
static int mat4_mul(lua_State *L);
static int mat4_div(lua_State *L);
static int mat4_unm(lua_State *L);
static int mat4_set(lua_State *L);
static int mat4_len(lua_State *L);
static int mat4_eq(lua_State *L);

static int vec_length(lua_State *L);
static int vec_distance(lua_State *L);
static int vec_dot(lua_State *L);
static int vec_cross(lua_State *L);
static int vec_normalize(lua_State *L);
static int vec_faceforward(lua_State *L);
static int vec_reflect(lua_State *L);
static int vec_refract(lua_State *L);

static int quat_new(lua_State *L);
static int quat_index(lua_State *L);
static int quat_mul(lua_State *L);
static int quat_eq(lua_State *L);

static int perspective(lua_State *L);

//---------------- newindex func for immutable objects ------------------//

static int immutable_newindex(lua_State *L) {
    return luaL_error(L, "attempt to update immutable type %s",
        am_get_typename(L, am_get_type(L, 1)));
}

//-------------------------- vec* helper macros ------------------//

#define VEC_OP_FUNC(D, OPNAME, OP)                                              \
static int vec##D##_##OPNAME(lua_State *L) {                                    \
    if (lua_isnumber(L, 1)) {                                                   \
        am_vec##D *z = am_new_userdata(L, am_vec##D);                           \
        double x = lua_tonumber(L, 1);                                          \
        am_vec##D *y = (am_vec##D*)lua_touserdata(L, 2);                        \
        z->v = x OP y->v;                                                       \
    } else {                                                                    \
        switch (am_get_type(L, 2)) {                                            \
            case LUA_TNUMBER: {                                                 \
                am_vec##D *z = am_new_userdata(L, am_vec##D);                   \
                am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                \
                double y = lua_tonumber(L, 2);                                  \
                z->v = x->v OP y;                                               \
                break;                                                          \
            }                                                                   \
            case MT_am_buffer_view: {                                           \
                return am_mathv_##OPNAME(L);                                    \
                break;                                                          \
            }                                                                   \
            default: {                                                          \
                am_vec##D *z = am_new_userdata(L, am_vec##D);                   \
                am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                \
                am_vec##D *y = (am_vec##D*)lua_touserdata(L, 2);                \
                z->v = x->v OP y->v;                                            \
                break;                                                          \
            }                                                                   \
        }                                                                       \
    }                                                                           \
    return 1;                                                                   \
}

#define VEC2_MUL_FUNC                                                           \
static int vec2_mul(lua_State *L) {                                             \
    if (lua_isnumber(L, 1)) {                                                   \
        am_vec2 *z = am_new_userdata(L, am_vec2);                               \
        double x = lua_tonumber(L, 1);                                          \
        am_vec2 *y = am_get_userdata(L, am_vec2, 2);                            \
        z->v = x * y->v;                                                        \
    } else {                                                                    \
        switch (am_get_type(L, 2)) {                                            \
            case LUA_TNUMBER: {                                                 \
                am_vec2 *z = am_new_userdata(L, am_vec2);                       \
                am_vec2 *x = am_get_userdata(L, am_vec2, 1);                    \
                double y = lua_tonumber(L, 2);                                  \
                z->v = x->v * y;                                                \
                break;                                                          \
            }                                                                   \
            case MT_am_mat2: {                                                  \
                am_vec2 *z = am_new_userdata(L, am_vec2);                       \
                am_vec2 *x = am_get_userdata(L, am_vec2, 1);                    \
                am_mat2 *y = am_get_userdata(L, am_mat2, 2);                    \
                z->v = x->v * y->m;                                             \
                break;                                                          \
            }                                                                   \
            case MT_am_quat: {                                                  \
                am_vec2 *z = am_new_userdata(L, am_vec2);                       \
                am_vec2 *x = am_get_userdata(L, am_vec2, 1);                    \
                am_quat *y = am_get_userdata(L, am_quat, 2);                    \
                glm::dvec3 v3 = glm::dvec3(x->v.x, x->v.y, 0.0) * y->q;         \
                z->v = glm::dvec2(v3.x, v3.y);                                  \
                break;                                                          \
            }                                                                   \
            case MT_am_buffer_view: {                                           \
                return am_mathv_mul(L);                                         \
                break;                                                          \
            }                                                                   \
            default: {                                                          \
                am_vec2 *z = am_new_userdata(L, am_vec2);                       \
                am_vec2 *x = am_get_userdata(L, am_vec2, 1);                    \
                am_vec2 *y = am_get_userdata(L, am_vec2, 2);                    \
                z->v = x->v * y->v;                                             \
            }                                                                   \
        }                                                                       \
    }                                                                           \
    return 1;                                                                   \
}

#define VEC_MUL_FUNC_Q(D)                                                       \
static int vec##D##_mul(lua_State *L) {                                         \
    if (lua_isnumber(L, 1)) {                                                   \
        am_vec##D *z = am_new_userdata(L, am_vec##D);                           \
        double x = lua_tonumber(L, 1);                                          \
        am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                        \
        z->v = x * y->v;                                                        \
    } else {                                                                    \
        switch (am_get_type(L, 2)) {                                            \
            case LUA_TNUMBER: {                                                 \
                am_vec##D *z = am_new_userdata(L, am_vec##D);                   \
                am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                \
                double y = lua_tonumber(L, 2);                                  \
                z->v = x->v * y;                                                \
                break;                                                          \
            }                                                                   \
            case MT_am_mat##D: {                                                \
                am_vec##D *z = am_new_userdata(L, am_vec##D);                   \
                am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                \
                am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                \
                z->v = x->v * y->m;                                             \
                break;                                                          \
            }                                                                   \
            case MT_am_quat: {                                                  \
                am_vec##D *z = am_new_userdata(L, am_vec##D);                   \
                am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                \
                am_quat *y = am_get_userdata(L, am_quat, 2);                    \
                z->v = x->v * y->q;                                             \
                break;                                                          \
            }                                                                   \
            case MT_am_buffer_view: {                                           \
                return am_mathv_mul(L);                                         \
                break;                                                          \
            }                                                                   \
            default: {                                                          \
                am_vec##D *z = am_new_userdata(L, am_vec##D);                   \
                am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                \
                am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                \
                z->v = x->v * y->v;                                             \
            }                                                                   \
        }                                                                       \
    }                                                                           \
    return 1;                                                                   \
}

#define VEC_DIV_FUNC(D)                                                         \
static int vec##D##_div(lua_State *L) {                                         \
    if (lua_isnumber(L, 1)) {                                                   \
        am_vec##D *z = am_new_userdata(L, am_vec##D);                           \
        double x = lua_tonumber(L, 1);                                          \
        am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                        \
        z->v = x / y->v;                                                        \
    } else {                                                                    \
        switch (am_get_type(L, 2)) {                                            \
            case LUA_TNUMBER: {                                                 \
                am_vec##D *z = am_new_userdata(L, am_vec##D);                   \
                am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                \
                double y = lua_tonumber(L, 2);                                  \
                z->v = x->v / y;                                                \
                break;                                                          \
            }                                                                   \
            case MT_am_mat##D: {                                                \
                am_vec##D *z = am_new_userdata(L, am_vec##D);                   \
                am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                \
                am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                \
                z->v = x->v * glm::inverse(y->m);                               \
                break;                                                          \
            }                                                                   \
            case MT_am_buffer_view: {                                           \
                return am_mathv_div(L);                                         \
                break;                                                          \
            }                                                                   \
            default: {                                                          \
                am_vec##D *z = am_new_userdata(L, am_vec##D);                   \
                am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                \
                am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                \
                z->v = x->v / y->v;                                             \
            }                                                                   \
        }                                                                       \
    }                                                                           \
    return 1;                                                                   \
}

#define VEC_UNM_FUNC(D)                                                     \
static int vec##D##_unm(lua_State *L) {                                     \
    am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                        \
    am_vec##D *y = am_new_userdata(L, am_vec##D);                           \
    y->v = -x->v;                                                           \
    return 1;                                                               \
}

#define VEC_LEN_FUNC(D)                                                     \
static int vec##D##_len(lua_State *L) {                                     \
    lua_pushinteger(L, D);                                                  \
    return 1;                                                               \
}

#define VEC_NEW_FUNC(D)                                                                 \
static int vec##D##_new(lua_State *L) {                                                 \
    int n = lua_gettop(L);                                                              \
    if (n == 0) {                                                                       \
        return luaL_error(L, "vec" #D " constructor requires at least one argument");   \
    }                                                                                   \
    if (n == 1 && lua_isnumber(L, 1)) {                                                 \
        am_vec##D *v = am_new_userdata(L, am_vec##D);                                   \
        v->v = glm::dvec##D((double)lua_tonumber(L, 1));                                \
    } else {                                                                            \
        am_vec##D *vv = am_new_userdata(L, am_vec##D);                                  \
        glm::dvec##D *v = &vv->v;                                                       \
        int i = 0;                                                                      \
        int j;                                                                          \
        for (j = 1; j <= n; j++) {                                                      \
            switch (am_get_type(L, j)) {                                                \
                case LUA_TNUMBER: {                                                     \
                    (*v)[i++] = lua_tonumber(L, j);                                     \
                    if (i >= D) goto endfor;                                            \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec2: {                                                      \
                    am_vec2 *va = (am_vec2*)lua_touserdata(L, j);                       \
                    (*v)[i++] = va->v.x;                                                \
                    if (i >= D) goto endfor;                                            \
                    (*v)[i++] = va->v.y;                                                \
                    if (i >= D) goto endfor;                                            \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec3: {                                                      \
                    am_vec3 *va = (am_vec3*)lua_touserdata(L, j);                       \
                    (*v)[i++] = va->v.x;                                                \
                    if (i >= D) goto endfor;                                            \
                    (*v)[i++] = va->v.y;                                                \
                    if (i >= D) goto endfor;                                            \
                    (*v)[i++] = va->v.z;                                                \
                    if (i >= D) goto endfor;                                            \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec4: {                                                      \
                    am_vec4 *va = (am_vec4*)lua_touserdata(L, j);                       \
                    (*v)[i++] = va->v.x;                                                \
                    if (i >= D) goto endfor;                                            \
                    (*v)[i++] = va->v.y;                                                \
                    if (i >= D) goto endfor;                                            \
                    (*v)[i++] = va->v.z;                                                \
                    if (i >= D) goto endfor;                                            \
                    (*v)[i++] = va->v.w;                                                \
                    if (i >= D) goto endfor;                                            \
                    break;                                                              \
                }                                                                       \
                default: return luaL_error(L,                                           \
                    "unexpected type %s at position %d in vec" #D " argument list",     \
                    am_get_typename(L, am_get_type(L, j)), j);                          \
            }                                                                           \
        }                                                                               \
        endfor:                                                                         \
        if (j < n) {                                                                    \
            return luaL_error(L, "too many arguments to vec" #D " constructor");        \
        }                                                                               \
        if (i < D) {                                                                    \
            return luaL_error(L,                                                        \
                "vec" #D " constructor arguments have insufficient components");        \
        }                                                                               \
    }                                                                                   \
    return 1;                                                                           \
}

static const int vec_component_offset[] = {
     3, // a
     2, // b
    -1, // c
    -1, // d
    -1, // e
    -1, // f
     1, // g
    -1, // h
    -1, // i
    -1, // j
    -1, // k
    -1, // l
    -1, // m
    -1, // n
    -1, // o
     2, // p
     3, // q
     0, // r
     0, // s
     1, // t
    -1, // u
    -1, // v
     3, // w
     0, // x
     1, // y
     2, // z
};
#define VEC_COMPONENT_OFFSET(c) (((c) >= 'a' && (c) <= 'z') ? vec_component_offset[c-'a'] : -1)

#define VEC_INDEX_FUNC(D)                                                               \
int vec##D##_index(lua_State *L) {                                                      \
    glm::dvec##D v = ((am_vec##D*)lua_touserdata(L, 1))->v;                             \
    if (lua_type(L, 2) == LUA_TSTRING) {                                                \
        size_t len;                                                                     \
        const char *str = lua_tolstring(L, 2, &len);                                    \
        switch (len) {                                                                  \
            case 1:                                                                     \
                switch (str[0]) {                                                       \
                    case 'x':                                                           \
                    case 'r':                                                           \
                    case 's':                                                           \
                        lua_pushnumber(L, v[0]);                                        \
                        return 1;                                                       \
                    case 'y':                                                           \
                    case 'g':                                                           \
                    case 't':                                                           \
                        lua_pushnumber(L, v[1]);                                        \
                        return 1;                                                       \
                    case 'z':                                                           \
                    case 'b':                                                           \
                    case 'p':                                                           \
                        if (D > 2) {                                                    \
                            lua_pushnumber(L, v[2]);                                    \
                            return 1;                                                   \
                        } else {                                                        \
                            return 0;                                                   \
                        }                                                               \
                    case 'w':                                                           \
                    case 'a':                                                           \
                    case 'q':                                                           \
                        if (D > 3) {                                                    \
                            lua_pushnumber(L, v[3]);                                    \
                            return 1;                                                   \
                        } else {                                                        \
                            return 0;                                                   \
                        }                                                               \
                    default:                                                            \
                        return 0;                                                       \
                }                                                                       \
            case 2: {                                                                   \
                glm::dvec2 vv;                                                          \
                for (int i = 0; i < 2; i++) {                                           \
                    int os = VEC_COMPONENT_OFFSET(str[i]);                              \
                    if (os >= 0 && os < D) {                                            \
                        vv[i] = v[os];                                                  \
                    } else {                                                            \
                        return 0;                                                       \
                    }                                                                   \
                }                                                                       \
                am_vec2 *nv = am_new_userdata(L, am_vec2);                              \
                nv->v = vv;                                                             \
                return 1;                                                               \
            }                                                                           \
            case 3: {                                                                   \
                glm::dvec3 vv;                                                          \
                for (int i = 0; i < 3; i++) {                                           \
                    int os = VEC_COMPONENT_OFFSET(str[i]);                              \
                    if (os >= 0 && os < D) {                                            \
                        vv[i] = v[os];                                                  \
                    } else {                                                            \
                        return 0;                                                       \
                    }                                                                   \
                }                                                                       \
                am_vec3 *nv = am_new_userdata(L, am_vec3);                              \
                nv->v = vv;                                                             \
                return 1;                                                               \
            }                                                                           \
            case 4: {                                                                   \
                glm::dvec4 vv;                                                          \
                for (int i = 0; i < 4; i++) {                                           \
                    int os = VEC_COMPONENT_OFFSET(str[i]);                              \
                    if (os >= 0 && os < D) {                                            \
                        vv[i] = v[os];                                                  \
                    } else {                                                            \
                        return 0;                                                       \
                    }                                                                   \
                }                                                                       \
                am_vec4 *nv = am_new_userdata(L, am_vec4);                              \
                nv->v = vv;                                                             \
                return 1;                                                               \
            }                                                                           \
            default:                                                                    \
                return 0;                                                               \
        }                                                                               \
    } else {                                                                            \
        int i = lua_tointeger(L, 2);                                                    \
        if (i > 0 && i <= D) {                                                          \
            lua_pushnumber(L, v[i-1]);                                                  \
            return 1;                                                                   \
        } else {                                                                        \
            return 0;                                                                   \
        }                                                                               \
    }                                                                                   \
}

#define VEC_CALL_FUNC(D)                                                                \
                                                                                        \
static int vec##D##_set(lua_State *L, glm::dvec##D *v) {                                \
    if (lua_type(L, -2) == LUA_TSTRING) {                                               \
        size_t len;                                                                     \
        const char *str = lua_tolstring(L, -2, &len);                                   \
        if (len == 1) {                                                                 \
            switch (str[0]) {                                                           \
                case 'x':                                                               \
                case 'r':                                                               \
                case 's':                                                               \
                    v->x = luaL_checknumber(L, -1);                                     \
                    break;                                                              \
                case 'y':                                                               \
                case 'g':                                                               \
                case 't':                                                               \
                    v->y = luaL_checknumber(L, -1);                                     \
                    break;                                                              \
                case 'z':                                                               \
                case 'b':                                                               \
                case 'p':                                                               \
                    if (D > 2) {                                                        \
                        (*v)[2] = luaL_checknumber(L, -1);                              \
                    } else {                                                            \
                        return false;                                                   \
                    }                                                                   \
                    break;                                                              \
                case 'w':                                                               \
                case 'a':                                                               \
                case 'q':                                                               \
                    if (D > 3) {                                                        \
                        (*v)[3] = luaL_checknumber(L, -1);                              \
                    } else {                                                            \
                       return false;                                                    \
                    }                                                                   \
                    break;                                                              \
                default:                                                                \
                    return false;                                                       \
            }                                                                           \
        } else {                                                                        \
            if (len == 0 || len > D) return false;                                      \
            switch (am_get_type(L, -1)) {                                               \
                case LUA_TNUMBER: {                                                     \
                    double num = lua_tonumber(L, -1);                                   \
                    for (unsigned int i = 0; i < len; i++) {                            \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*v)[os] = num;                                             \
                        } else {                                                        \
                            return false;                                               \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec2: {                                                      \
                    if (len != 2) return false;                                         \
                    glm::dvec2 *nv = &((am_vec2*)lua_touserdata(L, -1))->v;             \
                    for (int i = 0; i < 2; i++) {                                       \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*v)[os] = (*nv)[i];                                        \
                        } else {                                                        \
                            return false;                                               \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec3: {                                                      \
                    if (len != 3) return false;                                         \
                    glm::dvec3 *nv = &((am_vec3*)lua_touserdata(L, -1))->v;             \
                    for (int i = 0; i < 3; i++) {                                       \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*v)[os] = (*nv)[i];                                        \
                        } else {                                                        \
                            return false;                                               \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec4: {                                                      \
                    if (len != 4) return false;                                         \
                    glm::dvec4 *nv = &((am_vec4*)lua_touserdata(L, -1))->v;             \
                    for (int i = 0; i < 4; i++) {                                       \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*v)[os] = (*nv)[i];                                        \
                        } else {                                                        \
                            return false;                                               \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                default:                                                                \
                    return false;                                                       \
            }                                                                           \
        }                                                                               \
    } else {                                                                            \
        int i = lua_tointeger(L, -2);                                                   \
        if (i > 0 && i <= D) {                                                          \
            (*v)[i-1] = luaL_checknumber(L, -1);                                        \
        } else {                                                                        \
            return false;                                                               \
        }                                                                               \
    }                                                                                   \
    return true;                                                                        \
}                                                                                       \
                                                                                        \
static int vec##D##_call(lua_State *L) {                                                \
    int nargs = am_check_nargs(L, 2);                                                   \
    glm::dvec##D v = am_get_userdata(L, am_vec##D, 1)->v;                               \
    switch (lua_type(L, 2)) {                                                           \
        case LUA_TSTRING:                                                               \
        case LUA_TNUMBER:                                                               \
            if (nargs != 3) {                                                           \
                return luaL_error(L, "expecting exactly 2 arguments when first is a string or number"); \
            }                                                                           \
            if (!vec##D##_set(L, &v)) {                                                 \
                return luaL_error(L, "cannot set field %s to a %s", lua_tostring(L, -2),\
                    am_get_typename(L, am_get_type(L, -1)));                            \
            }                                                                           \
            break;                                                                      \
        case LUA_TTABLE:                                                                \
            lua_pushnil(L);                                                             \
            while (lua_next(L, 2)) {                                                    \
                if (!vec##D##_set(L, &v)) {                                             \
                    return luaL_error(L, "cannot set field %s to a %s", lua_tostring(L, -2),\
                        am_get_typename(L, am_get_type(L, -1)));                        \
                }                                                                       \
                lua_pop(L, 1);                                                          \
            }                                                                           \
            break;                                                                      \
        default:                                                                        \
            return luaL_error(L, "expecting a table or field/value pair");              \
    }                                                                                   \
    am_new_userdata(L, am_vec##D)->v = v;                                               \
    return 1;                                                                           \
}

#define VEC_EQ_FUNC(D)                                                                  \
static int vec##D##_eq(lua_State *L) {                                                  \
    am_vec##D *a = am_get_userdata(L, am_vec##D, 1);                                    \
    am_vec##D *b = am_get_userdata(L, am_vec##D, 2);                                    \
    return a->v == b->v;                                                                \
}

//-------------------------- mat* helper macros ------------------//

#define MAT_SET_FUNC(D)                                                                 \
static int mat##D##_set(lua_State *L) {                                                 \
    am_check_nargs(L, 4);                                                               \
    am_mat##D *m = am_get_userdata(L, am_mat##D, 1);                                    \
    int col = luaL_checkinteger(L, 2);                                                  \
    int row = luaL_checkinteger(L, 3);                                                  \
    double val = luaL_checknumber(L, 4);                                                \
    if (col < 1 || row < 1 || col > D || row >  D) {                                    \
        return luaL_error(L, "invalid mat" #D " indices: [%d, %d]", col, row);          \
    }                                                                                   \
    m->m[col-1][row-1] = val;                                                           \
    return 0;                                                                           \
}

MAT_SET_FUNC(2)
MAT_SET_FUNC(3)
MAT_SET_FUNC(4)

#define MAT_OP_FUNC(D, OPNAME, OP)                                                      \
    static int mat##D##_##OPNAME(lua_State *L) {                                        \
        am_mat##D *z = am_new_userdata(L, am_mat##D);                                   \
        if (lua_isnumber(L, 1)) {                                                       \
            double x = lua_tonumber(L, 1);                                              \
            am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                            \
            z->m = x OP y->m;                                                           \
        } else if (lua_isnumber(L, 2)) {                                                \
            am_mat##D *x = am_get_userdata(L, am_mat##D, 1);                            \
            double y = lua_tonumber(L, 2);                                              \
            z->m = x->m OP y;                                                           \
        } else {                                                                        \
            am_mat##D *x = am_get_userdata(L, am_mat##D, 1);                            \
            am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                            \
            z->m = x->m OP y->m;                                                        \
        }                                                                               \
        return 1;                                                                       \
    }

#define MAT_MUL_FUNC(D)                                                                 \
    static int mat##D##_mul(lua_State *L) {                                             \
        if (lua_isnumber(L, 1)) {                                                       \
            double x = lua_tonumber(L, 1);                                              \
            am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                            \
            am_mat##D *z = am_new_userdata(L, am_mat##D);                               \
            z->m = x * y->m;                                                            \
        } else {                                                                        \
            int t = am_get_type(L, 2);                                                  \
            switch (t) {                                                                \
                case LUA_TNUMBER: {                                                     \
                    am_mat##D *x = am_get_userdata(L, am_mat##D, 1);                    \
                    double y = lua_tonumber(L, 2);                                      \
                    am_mat##D *z = am_new_userdata(L, am_mat##D);                       \
                    z->m = x->m * y;                                                    \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec##D: {                                                    \
                    am_mat##D *x = am_get_userdata(L, am_mat##D, 1);                    \
                    am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                    \
                    am_vec##D *z = am_new_userdata(L, am_vec##D);                       \
                    z->v = x->m * y->v;                                                 \
                    break;                                                              \
                }                                                                       \
                case MT_am_buffer_view: {                                               \
                    return am_mathv_mul(L);                                             \
                }                                                                       \
                default: {                                                              \
                    am_mat##D *x = am_get_userdata(L, am_mat##D, 1);                    \
                    am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                    \
                    am_mat##D *z = am_new_userdata(L, am_mat##D);                       \
                    z->m = x->m * y->m;                                                 \
                    break;                                                              \
                }                                                                       \
            }                                                                           \
        }                                                                               \
        return 1;                                                                       \
    }

#define MAT_UNM_FUNC(D)                                                                 \
static int mat##D##_unm(lua_State *L) {                                                 \
    am_mat##D *m = (am_mat##D*)lua_touserdata(L, 1);                                    \
    am_mat##D *n = am_new_userdata(L, am_mat##D);                                       \
    n->m = -(m->m);                                                                     \
    return 1;                                                                           \
}

#define MAT_LEN_FUNC(D)                                                                 \
static int mat##D##_len(lua_State *L) {                                                 \
    lua_pushinteger(L, D);                                                              \
    return 1;                                                                           \
}

#define MAT_EQ_FUNC(D)                                                                  \
static int mat##D##_eq(lua_State *L) {                                                  \
    am_mat##D *a = am_get_userdata(L, am_mat##D, 1);                                    \
    am_mat##D *b = am_get_userdata(L, am_mat##D, 2);                                    \
    return a->m == b->m;                                                                \
}

#define MAT_NEW_FUNC(D)                                                                 \
static int mat##D##_new(lua_State *L) {                                                 \
    int n = lua_gettop(L);                                                              \
    if (n == 0) {                                                                       \
        return luaL_error(L, "mat" #D " constructor requires at least one argument");   \
    }                                                                                   \
    am_mat##D *mat = am_new_userdata(L, am_mat##D);                                     \
    if (n == 1) {                                                                       \
        if (lua_isnumber(L, 1)) {                                                       \
            mat->m = glm::dmat##D((double)lua_tonumber(L, 1));                          \
        } else {                                                                        \
            switch (am_get_type(L, 1)) {                                                \
                case MT_am_mat2: {                                                      \
                    am_mat2 *m = (am_mat2*)lua_touserdata(L, 1);                        \
                    mat->m = glm::dmat##D(m->m);                                        \
                    break;                                                              \
                }                                                                       \
                case MT_am_mat3: {                                                      \
                    am_mat3 *m = (am_mat3*)lua_touserdata(L, 1);                        \
                    mat->m = glm::dmat##D(m->m);                                        \
                    break;                                                              \
                }                                                                       \
                case MT_am_mat4: {                                                      \
                    am_mat4 *m = (am_mat4*)lua_touserdata(L, 1);                        \
                    mat->m = glm::dmat##D(m->m);                                        \
                    break;                                                              \
                }                                                                       \
                case MT_am_quat: {                                                      \
                    am_quat *q = (am_quat*)lua_touserdata(L, 1);                        \
                    glm::dmat3 m3 = glm::mat3_cast(q->q);                               \
                    mat->m = glm::dmat##D(m3);                                          \
                    break;                                                              \
                }                                                                       \
                default:                                                                \
                    return luaL_error(L, "invalid mat" #D " constructor arguments");    \
            }                                                                           \
        }                                                                               \
    } else {                                                                            \
        int row = 0;                                                                    \
        int col = 0;                                                                    \
        int i;                                                                          \
        glm::dmat##D *m = &mat->m;                                                      \
        for (i = 1; i <= n; i++) {                                                      \
            if (lua_isnumber(L, i)) {                                                   \
                (*m)[col][row++] = lua_tonumber(L, i);                                  \
                if (row >= D) {col++; row=0;} if (col >= D) goto endfor;                \
            } else {                                                                    \
                switch (am_get_type(L, i)) {                                            \
                    case MT_am_vec2: {                                                  \
                        am_vec2 *va = (am_vec2*)lua_touserdata(L, i);                   \
                        (*m)[col][row++] = va->v.x;                                     \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->v.y;                                     \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        break;                                                          \
                    }                                                                   \
                    case MT_am_vec3: {                                                  \
                        am_vec3 *va = (am_vec3*)lua_touserdata(L, i);                   \
                        (*m)[col][row++] = va->v.x;                                     \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->v.y;                                     \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->v.z;                                     \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        break;                                                          \
                    }                                                                   \
                    case MT_am_vec4: {                                                  \
                        am_vec4 *va = (am_vec4*)lua_touserdata(L, i);                   \
                        (*m)[col][row++] = va->v.x;                                     \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->v.y;                                     \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->v.z;                                     \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->v.w;                                     \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        break;                                                          \
                    }                                                                   \
                    default: return luaL_error(L,                                       \
                        "unexpected type %s in vec" #D " argument list at position %d", \
                            am_get_typename(L, am_get_type(L, i)), i);                  \
                }                                                                       \
            }                                                                           \
        }                                                                               \
        endfor:                                                                         \
        if (i < n) {                                                                    \
            return luaL_error(L, "too many arguments to mat" #D " constructor");        \
        }                                                                               \
        if (col < D) {                                                                  \
            return luaL_error(L,                                                        \
                "vec" #D " constructor arguments have insufficient components");        \
        }                                                                               \
    }                                                                                   \
    return 1;                                                                           \
}

#define MAT_INDEX_FUNC(D)                                                               \
static int mat##D##_index(lua_State *L) {                                               \
    am_mat##D *m = (am_mat##D*)lua_touserdata(L, 1);                                    \
    int i = lua_tointeger(L, 2);                                                        \
    if (i >= 1 && i <= D) {                                                             \
        am_vec##D *v = am_new_userdata(L, am_vec##D);                                   \
        v->v = m->m[i-1];                                                               \
    } else {                                                                            \
        lua_getmetatable(L, 1);                                                         \
        lua_pushvalue(L, 2);                                                            \
        lua_rawget(L, -2);                                                              \
        lua_remove(L, -2);                                                              \
        return 1;                                                                       \
    }                                                                                   \
    return 1;                                                                           \
}

//-------------------------- vec2 --------------------------------//

VEC_NEW_FUNC(2)
VEC_INDEX_FUNC(2)
VEC_CALL_FUNC(2)
VEC_OP_FUNC(2, add, +)
VEC_OP_FUNC(2, sub, -)
VEC2_MUL_FUNC
VEC_DIV_FUNC(2)
VEC_UNM_FUNC(2)
VEC_LEN_FUNC(2)
VEC_EQ_FUNC(2)

//-------------------------- vec3 --------------------------------//

VEC_NEW_FUNC(3)
VEC_INDEX_FUNC(3)
VEC_CALL_FUNC(3)
VEC_OP_FUNC(3, add, +)
VEC_OP_FUNC(3, sub, -)
VEC_MUL_FUNC_Q(3)
VEC_DIV_FUNC(3)
VEC_UNM_FUNC(3)
VEC_LEN_FUNC(3)
VEC_EQ_FUNC(3)

//-------------------------- vec4 --------------------------------//

VEC_NEW_FUNC(4)
VEC_INDEX_FUNC(4)
VEC_CALL_FUNC(4)
VEC_OP_FUNC(4, add, +)
VEC_OP_FUNC(4, sub, -)
VEC_MUL_FUNC_Q(4)
VEC_DIV_FUNC(4)
VEC_UNM_FUNC(4)
VEC_LEN_FUNC(4)
VEC_EQ_FUNC(4)

//-------------------------- mat2 --------------------------------//

MAT_NEW_FUNC(2)
MAT_INDEX_FUNC(2)
MAT_OP_FUNC(2, add, +)
MAT_OP_FUNC(2, sub, -)
MAT_OP_FUNC(2, div, /)
MAT_MUL_FUNC(2)
MAT_UNM_FUNC(2)
MAT_LEN_FUNC(2)
MAT_EQ_FUNC(2)

//-------------------------- mat3 --------------------------------//

MAT_NEW_FUNC(3)
MAT_INDEX_FUNC(3)
MAT_OP_FUNC(3, add, +)
MAT_OP_FUNC(3, sub, -)
MAT_OP_FUNC(3, div, /)
MAT_MUL_FUNC(3)
MAT_UNM_FUNC(3)
MAT_LEN_FUNC(3)
MAT_EQ_FUNC(3)

//-------------------------- mat4 --------------------------------//

MAT_NEW_FUNC(4)
MAT_INDEX_FUNC(4)
MAT_OP_FUNC(4, add, +)
MAT_OP_FUNC(4, sub, -)
MAT_OP_FUNC(4, div, /)
MAT_MUL_FUNC(4)
MAT_UNM_FUNC(4)
MAT_LEN_FUNC(4)
MAT_EQ_FUNC(4)

//-------------------------- quat --------------------------------//

static int quat_mul(lua_State *L) {
    if (lua_isnumber(L, 1)) {
        am_quat *z = am_new_userdata(L, am_quat);
        double x = lua_tonumber(L, 1);
        am_quat *y = am_get_userdata(L, am_quat, 2);
        z->q = x * y->q;
    } else {
        int t = am_get_type(L, 2);
        switch (t) {
            case MT_am_vec2: {
                am_vec2 *z = am_new_userdata(L, am_vec2);
                am_quat *x = am_get_userdata(L, am_quat, 1);
                am_vec2 *y = am_get_userdata(L, am_vec2, 2);
                glm::dvec3 v3 = x->q * glm::dvec3(y->v.x, y->v.y, 0.0);
                z->v = glm::dvec2(v3.x, v3.y);
                break;
            }
            case MT_am_vec3: {
                am_vec3 *z = am_new_userdata(L, am_vec3);
                am_quat *x = am_get_userdata(L, am_quat, 1);
                am_vec3 *y = am_get_userdata(L, am_vec3, 2);
                z->v = x->q * y->v;
                break;
            }
            case MT_am_vec4: {
                am_vec4 *z = am_new_userdata(L, am_vec4);
                am_quat *x = am_get_userdata(L, am_quat, 1);
                am_vec4 *y = am_get_userdata(L, am_vec4, 2);
                z->v = x->q * y->v;
                break;
            }
            default: {
                am_quat *z = am_new_userdata(L, am_quat);
                am_quat *x = am_get_userdata(L, am_quat, 1);
                am_quat *y = am_get_userdata(L, am_quat, 2);
                z->q = x->q * y->q;
            }
        }
    }
    return 1;
}

static int quat_new(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    glm::dquat quat;
    switch (nargs) {
        case 1: {
            switch (am_get_type(L, 1)) {
                case LUA_TNUMBER: {
                    double angle = lua_tonumber(L, 1);
                    glm::dvec3 axis(0.0, 0.0, 1.0);
                    quat = glm::angleAxis(angle, axis);
                    break;
                }
                case MT_am_mat3:
                    quat = glm::dquat(am_get_userdata(L, am_mat3, 1)->m);
                    break;
                case MT_am_mat4:
                    quat = glm::dquat(am_get_userdata(L, am_mat4, 1)->m);
                    break;
                default:
                    return luaL_error(L, "unexpected argument type: %s", am_get_typename(L, am_get_type(L, 1)));
            }
            break;
        }
        case 2: {
            if (lua_isnumber(L, 1)) {
                double angle = luaL_checknumber(L, 1);
                quat = glm::angleAxis(angle, am_get_userdata(L, am_vec3, 2)->v);
            } else {
                quat = glm::dquat(am_get_userdata(L, am_vec3, 1)->v, am_get_userdata(L, am_vec3, 2)->v);
            }
            break;
        }
        case 3: {
            quat = glm::dquat(glm::dvec3(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3)));
            break;
        }
        case 4: {
            quat = glm::dquat(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
                luaL_checknumber(L, 3), luaL_checknumber(L, 4));
            break;
        }
        default: {
            return luaL_error(L, "too many arguments to quat");
        }
    }
    am_new_userdata(L, am_quat)->q = quat;
    return 1;
}

static int quat_index(lua_State *L) {
    am_quat *quat = (am_quat*)lua_touserdata(L, 1);
    if (lua_type(L, 2) != LUA_TSTRING) {
        lua_pushnil(L);
        return 1;
    }
    size_t len;
    const char *field = lua_tolstring(L, 2, &len);
    switch (*field) {
        case 'a':
            if (strcmp(field, "angle") == 0) {
                lua_pushnumber(L, glm::angle(quat->q));
                return 1;
            } else if (strcmp(field, "axis") == 0) {
                am_new_userdata(L, am_vec3)->v = glm::axis(quat->q);
                return 1;
            }
            break;
        case 'p':
            if (strcmp(field, "pitch") == 0) {
                lua_pushnumber(L, glm::pitch(quat->q));
                return 1;
            }
            break;
        case 'r':
            if (strcmp(field, "roll") == 0) {
                lua_pushnumber(L, glm::roll(quat->q));
                return 1;
            }
            break;
        case 'w':
            if (len == 1) {
                lua_pushnumber(L, quat->q.w);
                return 1;
            }
            break;
        case 'x':
            if (len == 1) {
                lua_pushnumber(L, quat->q.x);
                return 1;
            }
            break;
        case 'y':
            if (len == 1) {
                lua_pushnumber(L, quat->q.y);
                return 1;
            } else if (strcmp(field, "yaw") == 0) {
                lua_pushnumber(L, glm::yaw(quat->q));
                return 1;
            }
            break;
        case 'z':
            if (len == 1) {
                lua_pushnumber(L, quat->q.z);
                return 1;
            }
            break;
    }
    lua_pushnil(L);
    return 1;
}

static int quat_eq(lua_State *L) {
    am_quat *a = (am_quat*)lua_touserdata(L, 1);
    am_quat *b = (am_quat*)lua_touserdata(L, 2);
    return a->q == b->q;
}

//----------------------- vec functions -------------------------//

static int vec_length(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            lua_pushnumber(L, glm::length(x->v));
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            lua_pushnumber(L, glm::length(x->v));
            break;
        }
        case MT_am_vec4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            lua_pushnumber(L, glm::length(x->v));
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int vec_distance(lua_State *L) {
    am_check_nargs(L, 2);
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = am_get_userdata(L, am_vec2, 2);
            lua_pushnumber(L, glm::distance(x->v, y->v));
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = am_get_userdata(L, am_vec3, 2);
            lua_pushnumber(L, glm::distance(x->v, y->v));
            break;
        }
        case MT_am_vec4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = am_get_userdata(L, am_vec4, 2);
            lua_pushnumber(L, glm::distance(x->v, y->v));
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int vec_dot(lua_State *L) {
    am_check_nargs(L, 2);
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = am_get_userdata(L, am_vec2, 2);
            lua_pushnumber(L, glm::dot(x->v, y->v));
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = am_get_userdata(L, am_vec3, 2);
            lua_pushnumber(L, glm::dot(x->v, y->v));
            break;
        }
        case MT_am_vec4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = am_get_userdata(L, am_vec4, 2);
            lua_pushnumber(L, glm::dot(x->v, y->v));
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int vec_cross(lua_State *L) {
    am_check_nargs(L, 2);
    am_vec3 *x = am_get_userdata(L, am_vec3, 1);
    am_vec3 *y = am_get_userdata(L, am_vec3, 2);
    am_vec3 *z = am_new_userdata(L, am_vec3);
    z->v = glm::cross(x->v, y->v);
    return 1;
}

#define IS_ZERO(x) ((x) == 0.0)

static int vec_normalize(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = am_new_userdata(L, am_vec2);
            y->v = glm::dvec2(1.0, 0.0);
            if (!(
                IS_ZERO(x->v.x) &&
                IS_ZERO(x->v.y)
            )) y->v = glm::normalize(x->v);
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = am_new_userdata(L, am_vec3);
            y->v = glm::dvec3(1.0, 0.0, 0.0);
            if (!(
                IS_ZERO(x->v.x) &&
                IS_ZERO(x->v.y) &&
                IS_ZERO(x->v.z)
            )) y->v = glm::normalize(x->v);
            break;
        }
        case MT_am_vec4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = am_new_userdata(L, am_vec4);
            y->v = glm::dvec4(1.0, 0.0, 0.0, 0.0);
            if (!(
                IS_ZERO(x->v.x) &&
                IS_ZERO(x->v.y) &&
                IS_ZERO(x->v.z) &&
                IS_ZERO(x->v.w)
            )) y->v = glm::normalize(x->v);
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int vec_faceforward(lua_State *L) {
    am_check_nargs(L, 3);
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            am_vec2 *N = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *I = am_get_userdata(L, am_vec2, 2);
            am_vec2 *Nref = am_get_userdata(L, am_vec2, 3);
            am_vec2 *r = am_new_userdata(L, am_vec2);
            r->v = glm::faceforward(N->v, I->v, Nref->v);
            break;
        }
        case MT_am_vec3: {
            am_vec3 *N = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *I = am_get_userdata(L, am_vec3, 2);
            am_vec3 *Nref = am_get_userdata(L, am_vec3, 3);
            am_vec3 *r = am_new_userdata(L, am_vec3);
            r->v = glm::faceforward(N->v, I->v, Nref->v);
            break;
        }
        case MT_am_vec4: {
            am_vec4 *N = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *I = am_get_userdata(L, am_vec4, 2);
            am_vec4 *Nref = am_get_userdata(L, am_vec4, 3);
            am_vec4 *r = am_new_userdata(L, am_vec4);
            r->v = glm::faceforward(N->v, I->v, Nref->v);
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int vec_reflect(lua_State *L) {
    am_check_nargs(L, 2);
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = am_get_userdata(L, am_vec2, 2);
            am_vec2 *z = am_new_userdata(L, am_vec2);
            z->v = glm::reflect(x->v, y->v);
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = am_get_userdata(L, am_vec3, 2);
            am_vec3 *z = am_new_userdata(L, am_vec3);
            z->v = glm::reflect(x->v, y->v);
            break;
        }
        case MT_am_vec4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = am_get_userdata(L, am_vec4, 2);
            am_vec4 *z = am_new_userdata(L, am_vec4);
            z->v = glm::reflect(x->v, y->v);
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int vec_refract(lua_State *L) {
    am_check_nargs(L, 3);
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            am_vec2 *I = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *N = am_get_userdata(L, am_vec2, 2);
            double eta = luaL_checknumber(L, 3);
            am_vec2 *r = am_new_userdata(L, am_vec2);
            r->v = glm::refract(I->v, N->v, eta);
            break;
        }
        case MT_am_vec3: {
            am_vec3 *I = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *N = am_get_userdata(L, am_vec3, 2);
            double eta = luaL_checknumber(L, 3);
            am_vec3 *r = am_new_userdata(L, am_vec3);
            r->v = glm::refract(I->v, N->v, eta);
            break;
        }
        case MT_am_vec4: {
            am_vec4 *I = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *N = am_get_userdata(L, am_vec4, 2);
            double eta = luaL_checknumber(L, 3);
            am_vec4 *r = am_new_userdata(L, am_vec4);
            r->v = glm::refract(I->v, N->v, eta);
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

//----------------------- mat functions -------------------------//

static int perspective(lua_State *L) {
    am_check_nargs(L, 4);
    double fovy = luaL_checknumber(L, 1);
    double aspect = luaL_checknumber(L, 2);
    double near = luaL_checknumber(L, 3);
    double far = luaL_checknumber(L, 4);
    am_mat4 *m = am_new_userdata(L, am_mat4);
    m->m = glm::perspective(fovy, aspect, near, far);
    return 1;
}

static int ortho(lua_State *L) {
    int nargs = am_check_nargs(L, 4);
    double left = luaL_checknumber(L, 1);
    double right = luaL_checknumber(L, 2);
    double bottom = luaL_checknumber(L, 3);
    double top = luaL_checknumber(L, 4);
    double near = -1.0;
    double far = 1.0;
    if (nargs > 4) {
        near = luaL_checknumber(L, 5);
        far = luaL_checknumber(L, 6);
    }
    am_mat4 *m = am_new_userdata(L, am_mat4);
    m->m = glm::ortho(left, right, bottom, top, near, far);
    return 1;
}

static int oblique(lua_State *L) {
    int nargs = am_check_nargs(L, 4);
    double angle = luaL_checknumber(L, 1);
    double zScale = luaL_checknumber(L, 2);
    double left = luaL_checknumber(L, 3);
    double right = luaL_checknumber(L, 4);
    double bottom = luaL_checknumber(L, 5);
    double top = luaL_checknumber(L, 6);
    double near = -1.0;
    double far = 1.0;
    if (nargs > 6) {
        near = luaL_checknumber(L, 7);
        far = luaL_checknumber(L, 8);
    }
    glm::dmat4 m1 = glm::ortho(left, right, bottom, top, near, far);
    glm::dmat4 m2 = glm::transpose(glm::dmat4(1.0, 0.0, - zScale * cos(angle), 0.0,
                              0.0, 1.0, - zScale * sin(angle), 0.0,
                              0.0, 0.0, 1.0, 0.0,
                              0.0, 0.0, 0.0, 1.0));
    am_mat4 *m = am_new_userdata(L, am_mat4);
    m->m = m1 * m2;
    return 1;
}

static int lookat(lua_State *L) {
    am_check_nargs(L, 3);
    am_vec3 *eye = am_get_userdata(L, am_vec3, 1);
    am_vec3 *center = am_get_userdata(L, am_vec3, 2);
    am_vec3 *up = am_get_userdata(L, am_vec3, 3);
    am_mat4 *result = am_new_userdata(L, am_mat4);
    result->m = glm::lookAt(eye->v, center->v, up->v);
    return 1;
}

static int rotate3(lua_State *L) {
    am_check_nargs(L, 2);
    double angle = luaL_checknumber(L, 1);
    am_vec3 *about = am_get_userdata(L, am_vec3, 2);
    am_mat3 *result = am_new_userdata(L, am_mat3);
    result->m = glm::dmat3(glm::rotate(angle, about->v));
    return 1;
}

static int rotate4(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    double angle = luaL_checknumber(L, 1);
    glm::dvec3 about(0.0, 0.0, 1.0);
    if (nargs > 1) {
        about = am_get_userdata(L, am_vec3, 2)->v;
    }
    am_mat4 *result = am_new_userdata(L, am_mat4);
    result->m = glm::rotate(angle, about);
    return 1;
}

static int translate4(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    glm::dvec3 v;
    switch (am_get_type(L, 1)) {
        case MT_am_vec3:
            v = am_get_userdata(L, am_vec3, 1)->v;
            break;
        case MT_am_vec2:
            v = glm::dvec3(am_get_userdata(L, am_vec2, 1)->v, 0.0);
            break;
        case LUA_TNUMBER: {
            if (nargs < 2) {
                return luaL_error(L, "too few arguments");
            } else if (nargs == 2) {
                v = glm::dvec3((double)luaL_checknumber(L, 1), (double)luaL_checknumber(L, 2), 0.0);
            } else if (nargs == 3) {
                v = glm::dvec3((double)luaL_checknumber(L, 1), (double)luaL_checknumber(L, 2), (double)luaL_checknumber(L, 3));
            } else {
                return luaL_error(L, "too many arguments");
            }
            break;
        }
        default:
            return luaL_error(L, "expecting a vec3, vec2 or number args");
    }
    am_mat4 *result = am_new_userdata(L, am_mat4);
    result->m = glm::translate(v);
    return 1;
}

static int scale4(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    glm::dvec3 v;
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            v = glm::dvec3(am_get_userdata(L, am_vec2, 1)->v, 1.0);
            break;
        }
        case MT_am_vec3: {
            v = am_get_userdata(L, am_vec3, 1)->v;
            break;
        }
        case LUA_TNUMBER: {
            if (nargs == 1) {
                double s = (double)luaL_checknumber(L, 1);
                v = glm::dvec3(s, s, 1.0);
            } else if (nargs == 2) {
                v = glm::dvec3((double)luaL_checknumber(L, 1), (double)luaL_checknumber(L, 2), 1.0);
            } else if (nargs == 4) {
                v = glm::dvec3((double)luaL_checknumber(L, 1), (double)luaL_checknumber(L, 2), (double)luaL_checknumber(L, 3));
            } else {
                return luaL_error(L, "too many arguments");
            }
            break;
        }
        default:
            return luaL_error(L, "expecting a vec2 or vec3 argument");
    }
    am_mat4 *result = am_new_userdata(L, am_mat4);
    result->m = glm::scale(v);
    return 1;
}

static int euleryxz3(lua_State *L) {
    am_check_nargs(L, 1);
    am_vec3 *angles = am_get_userdata(L, am_vec3, 1);
    am_mat3 *result = am_new_userdata(L, am_mat3);
    glm::dmat4 m = glm::yawPitchRoll(angles->v.y, angles->v.x, angles->v.z);
    result->m[0] = glm::dvec3(m[0].x, m[0].y, m[0].z);
    result->m[1] = glm::dvec3(m[1].x, m[1].y, m[1].z);
    result->m[2] = glm::dvec3(m[2].x, m[2].y, m[2].z);
    return 1;
}

static int euleryxz4(lua_State *L) {
    am_check_nargs(L, 1);
    am_vec3 *angles = am_get_userdata(L, am_vec3, 1);
    am_mat4 *result = am_new_userdata(L, am_mat4);
    result->m = glm::yawPitchRoll(angles->v.y, angles->v.x, angles->v.z);
    return 1;
}

static glm::dvec4 plane_normalize(glm::dvec4 plane) {
    double l = glm::length(glm::dvec3(plane.x, plane.y, plane.z));
    return glm::dvec4(plane.x/l, plane.y/l, plane.z/l, plane.w/l);
}

bool am_sphere_visible(glm::dmat4 &matrix, glm::dvec3 &center3, double radius) {
    glm::dvec4 center = glm::dvec4(center3, 1.0);
    glm::dvec4 left_plane = plane_normalize(glm::row(matrix, 0) + glm::row(matrix, 3));
    double left_dist = glm::dot(left_plane, center);
    if (left_dist + radius < 0.0) {
        return false;
    }
    glm::dvec4 right_plane = plane_normalize(glm::row(-matrix, 0) + glm::row(matrix, 3));
    double right_dist = glm::dot(right_plane, center);
    if (right_dist + radius < 0.0) {
        return false;
    }
    glm::dvec4 bottom_plane = plane_normalize(glm::row(matrix, 1) + glm::row(matrix, 3));
    double bottom_dist = glm::dot(bottom_plane, center);
    if (bottom_dist + radius < 0.0) {
        return false;
    }
    glm::dvec4 top_plane = plane_normalize(glm::row(-matrix, 1) + glm::row(matrix, 3));
    double top_dist = glm::dot(top_plane, center);
    if (top_dist + radius < 0.0) {
        return false;
    }
    glm::dvec4 near_plane = plane_normalize(glm::row(matrix, 2) + glm::row(matrix, 3));
    double near_dist = glm::dot(near_plane, center);
    if (near_dist + radius < 0.0) {
        return false;
    }
    glm::dvec4 far_plane = plane_normalize(glm::row(-matrix, 2) + glm::row(matrix, 3));
    double far_dist = glm::dot(far_plane, center);
    if (far_dist + radius < 0.0) {
        return false;
    }
    return true;
}

static int sphere_visible(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    glm::dmat4 matrix = am_get_userdata(L, am_mat4, 1)->m;
    glm::dvec3 center = am_get_userdata(L, am_vec3, 2)->v;
    double radius = 0.0;
    if (nargs > 2) {
        radius = luaL_checknumber(L, 3);
    }
    lua_pushboolean(L, (int)am_sphere_visible(matrix, center, radius));
    return 1;
}

bool am_box_visible(glm::dmat4 &matrix, glm::dvec3 &min, glm::dvec3 &max) {
    glm::dvec4 p1 = matrix * glm::vec4(min.x, min.y, min.z, 1.0);
    glm::dvec4 p2 = matrix * glm::vec4(min.x, min.y, max.z, 1.0);
    glm::dvec4 p3 = matrix * glm::vec4(min.x, max.y, min.z, 1.0);
    glm::dvec4 p4 = matrix * glm::vec4(min.x, max.y, max.z, 1.0);
    glm::dvec4 p5 = matrix * glm::vec4(max.x, min.y, min.z, 1.0);
    glm::dvec4 p6 = matrix * glm::vec4(max.x, min.y, max.z, 1.0);
    glm::dvec4 p7 = matrix * glm::vec4(max.x, max.y, min.z, 1.0);
    glm::dvec4 p8 = matrix * glm::vec4(max.x, max.y, max.z, 1.0);

    return !(
        (
            p1.x > p1.w &&
            p2.x > p2.w &&
            p3.x > p3.w &&
            p4.x > p4.w &&
            p5.x > p5.w &&
            p6.x > p6.w &&
            p7.x > p7.w &&
            p8.x > p8.w
        ) || (
            p1.x < -p1.w &&
            p2.x < -p2.w &&
            p3.x < -p3.w &&
            p4.x < -p4.w &&
            p5.x < -p5.w &&
            p6.x < -p6.w &&
            p7.x < -p7.w &&
            p8.x < -p8.w
        ) || (
            p1.y > p1.w &&
            p2.y > p2.w &&
            p3.y > p3.w &&
            p4.y > p4.w &&
            p5.y > p5.w &&
            p6.y > p6.w &&
            p7.y > p7.w &&
            p8.y > p8.w
        ) || (
            p1.y < -p1.w &&
            p2.y < -p2.w &&
            p3.y < -p3.w &&
            p4.y < -p4.w &&
            p5.y < -p5.w &&
            p6.y < -p6.w &&
            p7.y < -p7.w &&
            p8.y < -p8.w
        ) || (
            p1.z > p1.w &&
            p2.z > p2.w &&
            p3.z > p3.w &&
            p4.z > p4.w &&
            p5.z > p5.w &&
            p6.z > p6.w &&
            p7.z > p7.w &&
            p8.z > p8.w
        ) || (
            p1.z < -p1.w &&
            p2.z < -p2.w &&
            p3.z < -p3.w &&
            p4.z < -p4.w &&
            p5.z < -p5.w &&
            p6.z < -p6.w &&
            p7.z < -p7.w &&
            p8.z < -p8.w
        )
    );
}

static int box_visible(lua_State *L) {
    am_check_nargs(L, 3);
    glm::dmat4 matrix = am_get_userdata(L, am_mat4, 1)->m;
    glm::dvec3 min = am_get_userdata(L, am_vec3, 2)->v;
    glm::dvec3 max = am_get_userdata(L, am_vec3, 3)->v;
    lua_pushboolean(L, (int)am_box_visible(matrix, min, max));
    return 1;
}

static int inverse(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case MT_am_mat2: {
            am_mat2 *m = am_new_userdata(L, am_mat2);
            m->m = glm::inverse(((am_mat2*)lua_touserdata(L, 1))->m);
            break;
        }
        case MT_am_mat3: {
            am_mat3 *m = am_new_userdata(L, am_mat3);
            m->m = glm::inverse(((am_mat3*)lua_touserdata(L, 1))->m);
            break;
        }
        case MT_am_mat4: {
            am_mat4 *m = am_new_userdata(L, am_mat4);
            m->m = glm::inverse(((am_mat4*)lua_touserdata(L, 1))->m);
            break;
        }
        case MT_am_quat: {
            am_quat *q = am_new_userdata(L, am_quat);
            q->q = glm::inverse(((am_quat*)lua_touserdata(L, 1))->q);
            break;
        }
        default:
            return luaL_error(L, "expecting a mat argument");
    }
    return 1;
}

//----------------------- noise functions -------------------------//

static int simplex(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case LUA_TNUMBER: {
            lua_pushnumber(L, glm::simplex(glm::vec2((float)lua_tonumber(L, 1), 0.0f)));
            break;
        }
        case MT_am_vec2: {
            lua_pushnumber(L, glm::simplex(glm::vec2(((am_vec2*)lua_touserdata(L, 1))->v)));
            break;
        }
        case MT_am_vec3: {
            lua_pushnumber(L, glm::simplex(glm::vec3(((am_vec3*)lua_touserdata(L, 1))->v)));
            break;
        }
        case MT_am_vec4: {
            lua_pushnumber(L, glm::simplex(glm::vec4(((am_vec4*)lua_touserdata(L, 1))->v)));
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int perlin(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    if (nargs == 1) {
        switch (am_get_type(L, 1)) {
            case LUA_TNUMBER: {
                lua_pushnumber(L, glm::perlin(glm::dvec2((double)lua_tonumber(L, 1), 0.0)));
                break;
            }
            case MT_am_vec2: {
                lua_pushnumber(L, glm::perlin(((am_vec2*)lua_touserdata(L, 1))->v));
                break;
            }
            case MT_am_vec3: {
                lua_pushnumber(L, glm::perlin(((am_vec3*)lua_touserdata(L, 1))->v));
                break;
            }
            case MT_am_vec4: {
                lua_pushnumber(L, glm::perlin(((am_vec4*)lua_touserdata(L, 1))->v));
                break;
            }
            default:
                return luaL_error(L, "expecting a vec argument");
        }
    } else if (nargs == 2) {
        switch (am_get_type(L, 1)) {
            case LUA_TNUMBER: {
                lua_pushnumber(L, glm::perlin(glm::dvec2((double)lua_tonumber(L, 1), 0.0),
                    glm::dvec2((double)luaL_checknumber(L, 2), 1.0)));
                break;
            }
            case MT_am_vec2: {
                lua_pushnumber(L,
                    glm::perlin(((am_vec2*)lua_touserdata(L, 1))->v,
                    am_get_userdata(L, am_vec2, 2)->v));
                break;
            }
            case MT_am_vec3: {
                lua_pushnumber(L,
                    glm::perlin(((am_vec3*)lua_touserdata(L, 1))->v,
                    am_get_userdata(L, am_vec3, 2)->v));
                break;
            }
            case MT_am_vec4: {
                lua_pushnumber(L,
                    glm::perlin(((am_vec4*)lua_touserdata(L, 1))->v,
                    am_get_userdata(L, am_vec4, 2)->v));
                break;
            }
            default:
                return luaL_error(L, "expecting a vec argument");
        }
    } else {
        return luaL_error(L, "too many arguments (at most 2)");
    }
    return 1;
}

//----------------------- interpolation functions -------------------------//

static int mix(lua_State *L) {
    am_check_nargs(L, 3);
    switch (am_get_type(L, 1)) {
        case LUA_TNUMBER: {
            lua_Number x = lua_tonumber(L, 1);
            lua_Number y = luaL_checknumber(L, 2);
            lua_Number a = luaL_checknumber(L, 3);
            lua_pushnumber(L, glm::mix(x, y, a));
            break;
        }
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = am_get_userdata(L, am_vec2, 2);
            am_vec2 *r = am_new_userdata(L, am_vec2);
            switch (am_get_type(L, 3)) {
                case LUA_TNUMBER: {
                    double a = lua_tonumber(L, 3);
                    r->v = glm::mix(x->v, y->v, a);
                    break;
                }
                case MT_am_vec2: {
                    am_vec2 *a = (am_vec2*)lua_touserdata(L, 3);
                    r->v = glm::mix(x->v, y->v, a->v);
                    break;
                }
                default:
                    return luaL_error(L, "expecting a number or vec2 in position 3");
            }
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = am_get_userdata(L, am_vec3, 2);
            am_vec3 *r = am_new_userdata(L, am_vec3);
            switch (am_get_type(L, 3)) {
                case LUA_TNUMBER: {
                    double a = lua_tonumber(L, 3);
                    r->v = glm::mix(x->v, y->v, a);
                    break;
                }
                case MT_am_vec3: {
                    am_vec3 *a = (am_vec3*)lua_touserdata(L, 3);
                    r->v = glm::mix(x->v, y->v, a->v);
                    break;
                }
                default:
                    return luaL_error(L, "expecting a number or vec3 in position 3");
            }
            break;
        }
        case MT_am_vec4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = am_get_userdata(L, am_vec4, 2);
            am_vec4 *r = am_new_userdata(L, am_vec4);
            switch (am_get_type(L, 3)) {
                case LUA_TNUMBER: {
                    double a = lua_tonumber(L, 3);
                    r->v = glm::mix(x->v, y->v, a);
                    break;
                }
                case MT_am_vec4: {
                    am_vec4 *a = (am_vec4*)lua_touserdata(L, 3);
                    r->v = glm::mix(x->v, y->v, a->v);
                    break;
                }
                default:
                    return luaL_error(L, "expecting a number or vec4 in position 3");
            }
            break;
        }
        case MT_am_quat: {
            am_quat *x = (am_quat*)lua_touserdata(L, 1);
            am_quat *y = am_get_userdata(L, am_quat, 2);
            am_quat *r = am_new_userdata(L, am_quat);
            double a = lua_tonumber(L, 3);
            r->q = glm::mix(x->q, y->q, a);
            break;
        }
        default:
            return luaL_error(L, "expecting a number, vec or quat in position 1");
    }
    return 1;
}

static int slerp(lua_State *L) {
    am_check_nargs(L, 3);
    am_quat *x = am_get_userdata(L, am_quat, 1);
    am_quat *y = am_get_userdata(L, am_quat, 2);
    double a = lua_tonumber(L, 3);
    am_quat *r = am_new_userdata(L, am_quat);
    r->q = glm::slerp(x->q, y->q, a);
    return 1;
}

/*
 * XXX does not compile with msvc.
static int smoothstep(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case LUA_TNUMBER: {
            double x = lua_tonumber(L, 1);
            double y = luaL_checknumber(L, 2);
            switch (am_get_type(L, 2)) {
                case LUA_TNUMBER: {
                    double a = luaL_checknumber(L, 3);
                    lua_pushnumber(L, glm::smoothstep(x, y, a));
                    break;
                }
                case MT_am_vec2: {
                    am_vec2 *a = (am_vec2*)lua_touserdata(L, 3);
                    am_new_userdata(L, am_vec2)->v = glm::smoothstep(x, y, a->v);
                    break;
                }
                case MT_am_vec3: {
                    am_vec3 *a = (am_vec3*)lua_touserdata(L, 3);
                    am_new_userdata(L, am_vec3)->v = glm::smoothstep(x, y, a->v);
                    break;
                }
                case MT_am_vec4: {
                    am_vec4 *a = (am_vec4*)lua_touserdata(L, 3);
                    am_new_userdata(L, am_vec4)->v = glm::smoothstep(x, y, a->v);
                    break;
                }
                default:
                    return luaL_error(L, "expecting a number or vec in position 3");
            }
            break;
        }
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = am_get_userdata(L, am_vec2, 2);
            am_vec2 *a = am_get_userdata(L, am_vec2, 3);
            am_vec2 *r = am_new_userdata(L, am_vec2);
            r->v = glm::smoothstep(x->v, y->v, a->v);
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = am_get_userdata(L, am_vec3, 2);
            am_vec3 *a = am_get_userdata(L, am_vec3, 3);
            am_vec3 *r = am_new_userdata(L, am_vec3);
            r->v = glm::smoothstep(x->v, y->v, a->v);
            break;
        }
        case MT_am_vec4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = am_get_userdata(L, am_vec4, 2);
            am_vec4 *a = am_get_userdata(L, am_vec4, 3);
            am_vec4 *r = am_new_userdata(L, am_vec4);
            r->v = glm::smoothstep(x->v, y->v, a->v);
            break;
        }
        default:
            return luaL_error(L, "expecting a number or vec in position 1");
    }
    return 1;
}
*/

/*
static lua_Number smootherstep0(lua_Number edge0, lua_Number edge1, lua_Number x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
    return x*x*x*(x*(x*6.0 - 15.0) + 10.0);
}

static glm::dvec2 smootherstep0(double edge0, double edge1, glm::dvec2 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
    return x*x*x*(x*(x*6.0 - 15.0) + 10.0);
}

static glm::dvec3 smootherstep0(double edge0, double edge1, glm::dvec3 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
    return x*x*x*(x*(x*6.0 - 15.0) + 10.0);
}

static glm::dvec4 smootherstep0(double edge0, double edge1, glm::dvec4 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
    return x*x*x*(x*(x*6.0 - 15.0) + 10.0);
}

static glm::dvec2 smootherstep0(glm::dvec2 edge0, glm::dvec2 edge1, glm::dvec2 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
    return x*x*x*(x*(x*6.0 - 15.0) + 10.0);
}

static glm::dvec3 smootherstep0(glm::dvec3 edge0, glm::dvec3 edge1, glm::dvec3 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
    return x*x*x*(x*(x*6.0 - 15.0) + 10.0);
}

static glm::dvec4 smootherstep0(glm::dvec4 edge0, glm::dvec4 edge1, glm::dvec4 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
    return x*x*x*(x*(x*6.0 - 15.0) + 10.0);
}

static int smootherstep(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case LUA_TNUMBER: {
            lua_Number x = lua_tonumber(L, 1);
            lua_Number y = luaL_checknumber(L, 2);
            switch (am_get_type(L, 2)) {
                case LUA_TNUMBER: {
                    lua_Number a = luaL_checknumber(L, 3);
                    lua_pushnumber(L, smootherstep0(x, y, a));
                    break;
                }
                case MT_am_vec2: {
                    am_vec2 *a = (am_vec2*)lua_touserdata(L, 3);
                    am_new_userdata(L, am_vec2)->v = smootherstep0(x, y, a->v);
                    break;
                }
                case MT_am_vec3: {
                    am_vec3 *a = (am_vec3*)lua_touserdata(L, 3);
                    am_new_userdata(L, am_vec3)->v = smootherstep0(x, y, a->v);
                    break;
                }
                case MT_am_vec4: {
                    am_vec4 *a = (am_vec4*)lua_touserdata(L, 3);
                    am_new_userdata(L, am_vec4)->v = smootherstep0(x, y, a->v);
                    break;
                }
                default:
                    return luaL_error(L, "expecting a number or vec in position 3");
            }
            break;
        }
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = am_get_userdata(L, am_vec2, 2);
            am_vec2 *a = am_get_userdata(L, am_vec2, 3);
            am_vec2 *r = am_new_userdata(L, am_vec2);
            r->v = smootherstep0(x->v, y->v, a->v);
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = am_get_userdata(L, am_vec3, 2);
            am_vec3 *a = am_get_userdata(L, am_vec3, 3);
            am_vec3 *r = am_new_userdata(L, am_vec3);
            r->v = smootherstep0(x->v, y->v, a->v);
            break;
        }
        case MT_am_vec4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = am_get_userdata(L, am_vec4, 2);
            am_vec4 *a = am_get_userdata(L, am_vec4, 3);
            am_vec4 *r = am_new_userdata(L, am_vec4);
            r->v = smootherstep0(x->v, y->v, a->v);
            break;
        }
        default:
            return luaL_error(L, "expecting a number or vec in position 1");
    }
    return 1;
}
*/

//-------------------- clamping functions ------------------------//

static int clamp(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case LUA_TNUMBER: {
            lua_Number x = lua_tonumber(L, 1);
            lua_Number a = luaL_checknumber(L, 2);
            lua_Number b = luaL_checknumber(L, 3);
            lua_pushnumber(L, glm::clamp(x, a, b));
            break;
        }
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *r = am_new_userdata(L, am_vec2);
            switch (am_get_type(L, 2)) {
                case LUA_TNUMBER: {
                    double a = lua_tonumber(L, 2);
                    double b = lua_tonumber(L, 3);
                    r->v = glm::clamp(x->v, a, b);
                    break;
                }
                case MT_am_vec2: {
                    am_vec2 *a = (am_vec2*)lua_touserdata(L, 2);
                    am_vec2 *b = (am_vec2*)lua_touserdata(L, 3);
                    r->v = glm::clamp(x->v, a->v, b->v);
                    break;
                }
                default:
                    return luaL_error(L, "expecting a number or vec2 in position 2");
            }
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *r = am_new_userdata(L, am_vec3);
            switch (am_get_type(L, 2)) {
                case LUA_TNUMBER: {
                    double a = lua_tonumber(L, 2);
                    double b = lua_tonumber(L, 3);
                    r->v = glm::clamp(x->v, a, b);
                    break;
                }
                case MT_am_vec3: {
                    am_vec3 *a = (am_vec3*)lua_touserdata(L, 2);
                    am_vec3 *b = (am_vec3*)lua_touserdata(L, 3);
                    r->v = glm::clamp(x->v, a->v, b->v);
                    break;
                }
                default:
                    return luaL_error(L, "expecting a number or vec3 in position 2");
            }
            break;
        }
        case MT_am_vec4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *r = am_new_userdata(L, am_vec4);
            switch (am_get_type(L, 2)) {
                case LUA_TNUMBER: {
                    double a = lua_tonumber(L, 2);
                    double b = lua_tonumber(L, 3);
                    r->v = glm::clamp(x->v, a, b);
                    break;
                }
                case MT_am_vec4: {
                    am_vec4 *a = (am_vec4*)lua_touserdata(L, 2);
                    am_vec4 *b = (am_vec4*)lua_touserdata(L, 3);
                    r->v = glm::clamp(x->v, a->v, b->v);
                    break;
                }
                default:
                    return luaL_error(L, "expecting a number or vec4 in position 2");
            }
            break;
        }
        default:
            return luaL_error(L, "expecting a number or vec in position 1");
    }
    return 1;
}

static int fract(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case LUA_TNUMBER: {
            lua_pushnumber(L, glm::fract(lua_tonumber(L, 1)));
            break;
        }
        case MT_am_vec2: {
            am_new_userdata(L, am_vec2)->v = glm::fract(am_get_userdata(L, am_vec2, 1)->v);
            break;
        }
        case MT_am_vec3: {
            am_new_userdata(L, am_vec3)->v = glm::fract(am_get_userdata(L, am_vec3, 1)->v);
            break;
        }
        case MT_am_vec4: {
            am_new_userdata(L, am_vec4)->v = glm::fract(am_get_userdata(L, am_vec4, 1)->v);
            break;
        }
        default:
            return luaL_error(L, "expecting a number or vec in position 1");
    }
    return 1;
}

//-------------------------------------------------//

#define REGISTER_VEC_MT(T, MTID)                        \
    lua_newtable(L);                                    \
    lua_pushcclosure(L, T##_index, 0);                  \
    lua_setfield(L, -2, "__index");                     \
    lua_pushcclosure(L, immutable_newindex, 0);         \
    lua_setfield(L, -2, "__newindex");                  \
    lua_pushcclosure(L, T##_call, 0);                   \
    lua_setfield(L, -2, "__call");                      \
    lua_pushcclosure(L, T##_add, 0);                    \
    lua_setfield(L, -2, "__add");                       \
    lua_pushcclosure(L, T##_sub, 0);                    \
    lua_setfield(L, -2, "__sub");                       \
    lua_pushcclosure(L, T##_mul, 0);                    \
    lua_setfield(L, -2, "__mul");                       \
    lua_pushcclosure(L, T##_div, 0);                    \
    lua_setfield(L, -2, "__div");                       \
    lua_pushcclosure(L, T##_unm, 0);                    \
    lua_setfield(L, -2, "__unm");                       \
    lua_pushcclosure(L, T##_len, 0);                    \
    lua_setfield(L, -2, "__len");                       \
    lua_pushcclosure(L, T##_eq, 0);                     \
    lua_setfield(L, -2, "__eq");                        \
    am_register_metatable(L, #T, MTID, 0);

#define REGISTER_MAT_MT(T, MTID)                        \
    lua_newtable(L);                                    \
    lua_pushcclosure(L, T##_index, 0);                  \
    lua_setfield(L, -2, "__index");                     \
    lua_pushcclosure(L, immutable_newindex, 0);         \
    lua_setfield(L, -2, "__newindex");                  \
    lua_pushcclosure(L, T##_add, 0);                    \
    lua_setfield(L, -2, "__add");                       \
    lua_pushcclosure(L, T##_sub, 0);                    \
    lua_setfield(L, -2, "__sub");                       \
    lua_pushcclosure(L, T##_mul, 0);                    \
    lua_setfield(L, -2, "__mul");                       \
    lua_pushcclosure(L, T##_div, 0);                    \
    lua_setfield(L, -2, "__div");                       \
    lua_pushcclosure(L, T##_unm, 0);                    \
    lua_setfield(L, -2, "__unm");                       \
    lua_pushcclosure(L, T##_len, 0);                    \
    lua_setfield(L, -2, "__len");                       \
    lua_pushcclosure(L, T##_eq, 0);                     \
    lua_setfield(L, -2, "__eq");                        \
    lua_pushcclosure(L, T##_set, 0);                    \
    lua_setfield(L, -2, "set");                         \
    am_register_metatable(L, #T, MTID, 0);

static void register_quat_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, quat_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, immutable_newindex, 0);
    lua_setfield(L, -2, "__newindex");
    lua_pushcclosure(L, quat_mul, 0);
    lua_setfield(L, -2, "__mul");
    lua_pushcclosure(L, quat_eq, 0);
    lua_setfield(L, -2, "__eq");
    am_register_metatable(L, "quat", MT_am_quat, 0);
}

void am_open_math_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"vec2",        vec2_new},
        {"vec3",        vec3_new},
        {"vec4",        vec4_new},
        {"mat2",        mat2_new},
        {"mat3",        mat3_new},
        {"mat4",        mat4_new},
        {"quat",        quat_new},
        {"length",      vec_length},
        {"distance",    vec_distance},
        {"dot",         vec_dot},
        {"cross",       vec_cross},
        {"normalize",   vec_normalize},
        {"faceforward", vec_faceforward},
        {"reflect",     vec_reflect},
        {"refract",     vec_refract},
        {"perspective", perspective},
        {"ortho",       ortho},
        {"oblique",     oblique},
        {"lookat",      lookat},
        {"rotate3",     rotate3},
        {"rotate4",     rotate4},
        {"translate4",  translate4},
        {"scale4",      scale4},
        {"euleryxz3",   euleryxz3},
        {"euleryxz4",   euleryxz4},
        {"sphere_visible", sphere_visible},
        {"box_visible", box_visible},
        {"inverse",     inverse},
        {"simplex",     simplex},
        {"perlin",      perlin},
        {"mix",         mix},
        {"slerp",       slerp},
        {"clamp",       clamp},
        {"fract",       fract},
        //{"smoothstep",  smoothstep},
        //{"smootherstep",smootherstep},
        {NULL, NULL}
    };
    am_open_module(L, "math", funcs);
    REGISTER_VEC_MT(vec2, MT_am_vec2)
    REGISTER_VEC_MT(vec3, MT_am_vec3)
    REGISTER_VEC_MT(vec4, MT_am_vec4)
    REGISTER_MAT_MT(mat2, MT_am_mat2)
    REGISTER_MAT_MT(mat3, MT_am_mat3)
    REGISTER_MAT_MT(mat4, MT_am_mat4)
    register_quat_mt(L);
}
