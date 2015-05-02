#include "amulet.h"

static int vec2_new(lua_State *L);
static int vec2_index(lua_State *L);
static int vec2_newindex(lua_State *L);
static int vec2_add(lua_State *L);
static int vec2_sub(lua_State *L);
static int vec2_mul(lua_State *L);
static int vec2_div(lua_State *L);
static int vec2_unm(lua_State *L);
static int vec2_len(lua_State *L);

static int vec3_new(lua_State *L);
static int vec3_index(lua_State *L);
static int vec3_newindex(lua_State *L);
static int vec3_add(lua_State *L);
static int vec3_sub(lua_State *L);
static int vec3_mul(lua_State *L);
static int vec3_div(lua_State *L);
static int vec3_unm(lua_State *L);
static int vec3_len(lua_State *L);

static int vec4_new(lua_State *L);
static int vec4_index(lua_State *L);
static int vec4_newindex(lua_State *L);
static int vec4_add(lua_State *L);
static int vec4_sub(lua_State *L);
static int vec4_mul(lua_State *L);
static int vec4_div(lua_State *L);
static int vec4_unm(lua_State *L);
static int vec4_len(lua_State *L);

static int mat2_new(lua_State *L);
static int mat2_index(lua_State *L);
static int mat2_newindex(lua_State *L);
static int mat2_add(lua_State *L);
static int mat2_sub(lua_State *L);
static int mat2_mul(lua_State *L);
static int mat2_div(lua_State *L);
static int mat2_unm(lua_State *L);
static int mat2_set(lua_State *L);
static int mat2_len(lua_State *L);

static int mat3_new(lua_State *L);
static int mat3_index(lua_State *L);
static int mat3_newindex(lua_State *L);
static int mat3_add(lua_State *L);
static int mat3_sub(lua_State *L);
static int mat3_mul(lua_State *L);
static int mat3_div(lua_State *L);
static int mat3_unm(lua_State *L);
static int mat3_set(lua_State *L);
static int mat3_len(lua_State *L);

static int mat4_new(lua_State *L);
static int mat4_index(lua_State *L);
static int mat4_newindex(lua_State *L);
static int mat4_add(lua_State *L);
static int mat4_sub(lua_State *L);
static int mat4_mul(lua_State *L);
static int mat4_div(lua_State *L);
static int mat4_unm(lua_State *L);
static int mat4_set(lua_State *L);
static int mat4_len(lua_State *L);

static int vec_length(lua_State *L);
static int vec_distance(lua_State *L);
static int vec_dot(lua_State *L);
static int vec_cross(lua_State *L);
static int vec_normalize(lua_State *L);
static int vec_faceforward(lua_State *L);
static int vec_reflect(lua_State *L);
static int vec_refract(lua_State *L);

static int perspective(lua_State *L);

//-------------------------- vec* helper macros ------------------//

#define VEC_OP_FUNC(D, OPNAME, OP)                                              \
static int vec##D##_##OPNAME(lua_State *L) {                                    \
    am_vec##D *z = am_new_userdata(L, am_vec##D);                               \
    if (lua_isnumber(L, 1)) {                                                   \
        float x = lua_tonumber(L, 1);                                           \
        am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                        \
        z->v = x OP y->v;                                                       \
    } else if (lua_isnumber(L, 2)) {                                            \
        am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                        \
        float y = lua_tonumber(L, 2);                                           \
        z->v = x->v OP y;                                                       \
    } else {                                                                    \
        am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                        \
        am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                        \
        z->v = x->v OP y->v;                                                    \
    }                                                                           \
    return 1;                                                                   \
}

#define VEC_MUL_FUNC(D)                                                         \
static int vec##D##_mul(lua_State *L) {                                         \
    am_vec##D *z = am_new_userdata(L, am_vec##D);                               \
    if (lua_isnumber(L, 1)) {                                                   \
        float x = lua_tonumber(L, 1);                                           \
        am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                        \
        z->v = x * y->v;                                                        \
    } else if (lua_isnumber(L, 2)) {                                            \
        am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                        \
        float y = lua_tonumber(L, 2);                                           \
        z->v = x->v * y;                                                        \
    } else if (am_get_type(L, 2) == MT_am_mat##D) {                             \
        am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                        \
        am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                        \
        z->v = x->v * y->m;                                                     \
    } else {                                                                    \
        am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                        \
        am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                        \
        z->v = x->v * y->v;                                                     \
    }                                                                           \
    return 1;                                                                   \
}

#define VEC_DIV_FUNC(D)                                                         \
static int vec##D##_div(lua_State *L) {                                         \
    am_vec##D *z = am_new_userdata(L, am_vec##D);                               \
    if (lua_isnumber(L, 1)) {                                                   \
        float x = lua_tonumber(L, 1);                                           \
        am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                        \
        z->v = x / y->v;                                                        \
    } else if (lua_isnumber(L, 2)) {                                            \
        am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                        \
        float y = lua_tonumber(L, 2);                                           \
        z->v = x->v / y;                                                        \
    } else if (am_get_type(L, 2) == MT_am_mat##D) {                             \
        am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                        \
        am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                        \
        z->v = x->v * glm::inverse(y->m);                                       \
    } else {                                                                    \
        am_vec##D *x = am_get_userdata(L, am_vec##D, 1);                        \
        am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                        \
        z->v = x->v / y->v;                                                     \
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
        v->v = glm::vec##D((float)lua_tonumber(L, 1));                                  \
    } else {                                                                            \
        am_vec##D *vv = am_new_userdata(L, am_vec##D);                                  \
        glm::vec##D *v = &vv->v;                                                        \
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
                    "unexpected type at position %d in vec" #D " argument list (%d)", j, am_get_type(L, i)); \
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

#define AM_READ_VEC_FUNC(D)                                                             \
void am_read_vec##D(lua_State *L, glm::vec##D *v, int start, int end) {                 \
    int j = 0;                                                                          \
    int k;                                                                              \
    for (int i = start; i <= end && j < D; i++) {                                       \
        switch (am_get_type(L, i)) {                                                    \
            case LUA_TNUMBER:                                                           \
                (*v)[j++] = lua_tonumber(L, i);                                         \
                break;                                                                  \
            case MT_am_vec2: {                                                          \
                am_vec2 *v2 = (am_vec2*)lua_touserdata(L, i);                           \
                k = 0;                                                                  \
                while (j < D && k < 2) {                                                \
                    (*v)[j++] = v2->v[k++];                                             \
                }                                                                       \
                break;                                                                  \
            }                                                                           \
            case MT_am_vec3: {                                                          \
                am_vec3 *v3 = (am_vec3*)lua_touserdata(L, i);                           \
                k = 0;                                                                  \
                while (j < D && k < 3) {                                                \
                    (*v)[j++] = v3->v[k++];                                             \
                }                                                                       \
                break;                                                                  \
            }                                                                           \
            case MT_am_vec4: {                                                          \
                am_vec4 *v4 = (am_vec4*)lua_touserdata(L, i);                           \
                k = 0;                                                                  \
                while (j < D && k < 4) {                                                \
                    (*v)[j++] = v4->v[k++];                                             \
                }                                                                       \
                break;                                                                  \
            }                                                                           \
            default:                                                                    \
                luaL_error(L, "unexpected value of type %s at position %d", lua_typename(L, lua_type(L, i)), i); \
        }                                                                               \
    }                                                                                   \
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

#define AM_VEC_INDEX_FUNC(D)                                                            \
int am_vec##D##_index(lua_State *L, glm::vec##D *v) {                                   \
    if (lua_type(L, 2) == LUA_TSTRING) {                                                \
        size_t len;                                                                     \
        const char *str = lua_tolstring(L, 2, &len);                                    \
        switch (len) {                                                                  \
            case 1:                                                                     \
                switch (str[0]) {                                                       \
                    case 'x':                                                           \
                    case 'r':                                                           \
                    case 's':                                                           \
                        lua_pushnumber(L, (*v)[0]);                                     \
                        return 1;                                                       \
                    case 'y':                                                           \
                    case 'g':                                                           \
                    case 't':                                                           \
                        lua_pushnumber(L, (*v)[1]);                                     \
                        return 1;                                                       \
                    case 'z':                                                           \
                    case 'b':                                                           \
                    case 'p':                                                           \
                        if (D > 2) {                                                    \
                            lua_pushnumber(L, (*v)[2]);                                 \
                            return 1;                                                   \
                        } else {                                                        \
                            return 0;                                                   \
                        }                                                               \
                    case 'w':                                                           \
                    case 'a':                                                           \
                    case 'q':                                                           \
                        if (D > 3) {                                                    \
                            lua_pushnumber(L, (*v)[3]);                                 \
                            return 1;                                                   \
                        } else {                                                        \
                            return 0;                                                   \
                        }                                                               \
                    default:                                                            \
                        return 0;                                                       \
                }                                                                       \
            case 2: {                                                                   \
                glm::vec2 vv;                                                           \
                for (int i = 0; i < 2; i++) {                                           \
                    int os = VEC_COMPONENT_OFFSET(str[i]);                              \
                    if (os >= 0 && os < D) {                                            \
                        vv[i] = (*v)[os];                                               \
                    } else {                                                            \
                        return 0;                                                       \
                    }                                                                   \
                }                                                                       \
                am_vec2 *nv = am_new_userdata(L, am_vec2);                              \
                nv->v = vv;                                                             \
                return 1;                                                               \
            }                                                                           \
            case 3: {                                                                   \
                glm::vec3 vv;                                                           \
                for (int i = 0; i < 3; i++) {                                           \
                    int os = VEC_COMPONENT_OFFSET(str[i]);                              \
                    if (os >= 0 && os < D) {                                            \
                        vv[i] = (*v)[os];                                               \
                    } else {                                                            \
                        return 0;                                                       \
                    }                                                                   \
                }                                                                       \
                am_vec3 *nv = am_new_userdata(L, am_vec3);                              \
                nv->v = vv;                                                             \
                return 1;                                                               \
            }                                                                           \
            case 4: {                                                                   \
                glm::vec4 vv;                                                           \
                for (int i = 0; i < 4; i++) {                                           \
                    int os = VEC_COMPONENT_OFFSET(str[i]);                              \
                    if (os >= 0 && os < D) {                                            \
                        vv[i] = (*v)[os];                                               \
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
            lua_pushnumber(L, (*v)[i-1]);                                               \
            return 1;                                                                   \
        } else {                                                                        \
            return 0;                                                                   \
        }                                                                               \
    }                                                                                   \
}

#define VEC_INDEX_FUNC(D)                                                               \
static int vec##D##_index(lua_State *L) {                                               \
    am_vec##D *vv = (am_vec##D*)lua_touserdata(L, 1);                                   \
    return am_vec##D##_index(L, &vv->v);                                                \
}

#define AM_VEC_NEWINDEX_FUNC(D)                                                         \
int am_vec##D##_newindex(lua_State *L, glm::vec##D *v) {                                \
    if (lua_type(L, 2) == LUA_TSTRING) {                                                \
        size_t len;                                                                     \
        const char *str = lua_tolstring(L, 2, &len);                                    \
        if (len == 1) {                                                                 \
            switch (str[0]) {                                                           \
                case 'x':                                                               \
                case 'r':                                                               \
                case 's':                                                               \
                    v->x = luaL_checknumber(L, 3);                                      \
                    break;                                                              \
                case 'y':                                                               \
                case 'g':                                                               \
                case 't':                                                               \
                    v->y = luaL_checknumber(L, 3);                                      \
                    break;                                                              \
                case 'z':                                                               \
                case 'b':                                                               \
                case 'p':                                                               \
                    if (D > 2) {                                                        \
                        (*v)[2] = luaL_checknumber(L, 3);                               \
                    } else {                                                            \
                        goto fail;                                                      \
                    }                                                                   \
                    break;                                                              \
                case 'w':                                                               \
                case 'a':                                                               \
                case 'q':                                                               \
                    if (D > 3) {                                                        \
                        (*v)[3] = luaL_checknumber(L, 3);                               \
                    } else {                                                            \
                        goto fail;                                                      \
                    }                                                                   \
                    break;                                                              \
                default:                                                                \
                    goto fail;                                                          \
            }                                                                           \
        } else {                                                                        \
            if (len == 0 || len > D) goto fail;                                         \
            switch (am_get_type(L, 3)) {                                                \
                case LUA_TNUMBER: {                                                     \
                    float num = lua_tonumber(L, 3);                                     \
                    for (unsigned int i = 0; i < len; i++) {                            \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*v)[os] = num;                                             \
                        } else {                                                        \
                            goto fail;                                                  \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec2: {                                                      \
                    if (len != 2) goto fail;                                            \
                    glm::vec2 *nv = &((am_vec2*)lua_touserdata(L, 3))->v;               \
                    for (int i = 0; i < 2; i++) {                                       \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*v)[os] = (*nv)[i];                                        \
                        } else {                                                        \
                            goto fail;                                                  \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec3: {                                                      \
                    if (len != 3) goto fail;                                            \
                    glm::vec3 *nv = &((am_vec3*)lua_touserdata(L, 3))->v;               \
                    for (int i = 0; i < 3; i++) {                                       \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*v)[os] = (*nv)[i];                                        \
                        } else {                                                        \
                            goto fail;                                                  \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                case MT_am_vec4: {                                                      \
                    if (len != 4) goto fail;                                            \
                    glm::vec4 *nv = &((am_vec4*)lua_touserdata(L, 3))->v;               \
                    for (int i = 0; i < 4; i++) {                                       \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*v)[os] = (*nv)[i];                                        \
                        } else {                                                        \
                            goto fail;                                                  \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                default:                                                                \
                    goto fail;                                                          \
            }                                                                           \
        }                                                                               \
    } else {                                                                            \
        int i = lua_tointeger(L, 2);                                                    \
        if (i > 0 && i <= D) {                                                          \
            (*v)[i-1] = luaL_checknumber(L, 3);                                         \
        } else {                                                                        \
            goto fail;                                                                  \
        }                                                                               \
    }                                                                                   \
    return 0;                                                                           \
    fail:                                                                               \
    return -1;                                                                          \
}

#define VEC_NEWINDEX_FUNC(D)                                                            \
static int vec##D##_newindex(lua_State *L) {                                            \
    am_vec##D *vv = (am_vec##D*)lua_touserdata(L, 1);                                   \
    glm::vec##D *v = &vv->v;                                                            \
    int r = am_vec##D##_newindex(L, v);                                                 \
    if (r < 0) {                                                                        \
        return luaL_error(L, "invalid vec" #D " index or value");                       \
    } else {                                                                            \
        return r;                                                                       \
    }                                                                                   \
}

//-------------------------- mat* helper macros ------------------//

#define MAT_SET_FUNC(D)                                                                 \
static int mat##D##_set(lua_State *L) {                                                 \
    am_check_nargs(L, 4);                                                               \
    am_mat##D *m = am_get_userdata(L, am_mat##D, 1);                                    \
    int col = luaL_checkinteger(L, 2);                                                  \
    int row = luaL_checkinteger(L, 3);                                                  \
    float val = luaL_checknumber(L, 4);                                                 \
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
            float x = lua_tonumber(L, 1);                                               \
            am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                            \
            z->m = x OP y->m;                                                           \
        } else if (lua_isnumber(L, 2)) {                                                \
            am_mat##D *x = am_get_userdata(L, am_mat##D, 1);                            \
            float y = lua_tonumber(L, 2);                                               \
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
            float x = lua_tonumber(L, 1);                                               \
            am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                            \
            am_mat##D *z = am_new_userdata(L, am_mat##D);                               \
            z->m = x * y->m;                                                            \
        } else if (lua_isnumber(L, 2)) {                                                \
            am_mat##D *x = am_get_userdata(L, am_mat##D, 1);                            \
            float y = lua_tonumber(L, 2);                                               \
            am_mat##D *z = am_new_userdata(L, am_mat##D);                               \
            z->m = x->m * y;                                                            \
        } else if (am_get_type(L, 2) == MT_am_vec##D) {                                 \
            am_mat##D *x = am_get_userdata(L, am_mat##D, 1);                            \
            am_vec##D *y = am_get_userdata(L, am_vec##D, 2);                            \
            am_vec##D *z = am_new_userdata(L, am_vec##D);                               \
            z->v = x->m * y->v;                                                         \
        } else {                                                                        \
            am_mat##D *x = am_get_userdata(L, am_mat##D, 1);                            \
            am_mat##D *y = am_get_userdata(L, am_mat##D, 2);                            \
            am_mat##D *z = am_new_userdata(L, am_mat##D);                               \
            z->m = x->m * y->m;                                                         \
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

#define MAT_NEW_FUNC(D)                                                                 \
static int mat##D##_new(lua_State *L) {                                                 \
    int n = lua_gettop(L);                                                              \
    if (n == 0) {                                                                       \
        return luaL_error(L, "mat" #D " constructor requires at least one argument");   \
    }                                                                                   \
    am_mat##D *mat = am_new_userdata(L, am_mat##D);                                     \
    if (n == 1) {                                                                       \
        if (lua_isnumber(L, 1)) {                                                       \
            mat->m = glm::mat##D((float)lua_tonumber(L, 1));                            \
        } else {                                                                        \
            switch (am_get_type(L, 1)) {                                                \
                case MT_am_mat2: {                                                      \
                    am_mat2 *m = (am_mat2*)lua_touserdata(L, 1);                        \
                    mat->m = glm::mat##D(m->m);                                         \
                    break;                                                              \
                }                                                                       \
                case MT_am_mat3: {                                                      \
                    am_mat3 *m = (am_mat3*)lua_touserdata(L, 1);                        \
                    mat->m = glm::mat##D(m->m);                                         \
                    break;                                                              \
                }                                                                       \
                case MT_am_mat4: {                                                      \
                    am_mat4 *m = (am_mat4*)lua_touserdata(L, 1);                        \
                    mat->m = glm::mat##D(m->m);                                         \
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
        glm::mat##D *m = &mat->m;                                                       \
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
                        "unexpected type in vec" #D " argument list");                  \
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

#define MAT_NEWINDEX_FUNC(D)                                                            \
static int mat##D##_newindex(lua_State *L) {                                            \
    am_mat##D *m = (am_mat##D*)lua_touserdata(L, 1);                                    \
    int i = lua_tointeger(L, 2);                                                        \
    if (i >= 1 && i <= D) {                                                             \
        am_vec##D *v = am_get_userdata(L, am_vec##D, 3);                                \
        m->m[i-1] = v->v;                                                               \
        return 1;                                                                       \
    } else {                                                                            \
        return luaL_error(L, "invalid mat" #D " index: %s", lua_tostring(L, 2));        \
    }                                                                                   \
}

//-------------------------- vec2 --------------------------------//

VEC_NEW_FUNC(2)
AM_VEC_INDEX_FUNC(2)
VEC_INDEX_FUNC(2)
AM_VEC_NEWINDEX_FUNC(2)
VEC_NEWINDEX_FUNC(2)
VEC_OP_FUNC(2, add, +)
VEC_OP_FUNC(2, sub, -)
VEC_MUL_FUNC(2)
VEC_DIV_FUNC(2)
VEC_UNM_FUNC(2)
VEC_LEN_FUNC(2)
AM_READ_VEC_FUNC(2)

//-------------------------- vec3 --------------------------------//

VEC_NEW_FUNC(3)
AM_VEC_INDEX_FUNC(3)
VEC_INDEX_FUNC(3)
AM_VEC_NEWINDEX_FUNC(3)
VEC_NEWINDEX_FUNC(3)
VEC_OP_FUNC(3, add, +)
VEC_OP_FUNC(3, sub, -)
VEC_MUL_FUNC(3)
VEC_DIV_FUNC(3)
VEC_UNM_FUNC(3)
VEC_LEN_FUNC(3)
AM_READ_VEC_FUNC(3)

//-------------------------- vec4 --------------------------------//

VEC_NEW_FUNC(4)
AM_VEC_INDEX_FUNC(4)
VEC_INDEX_FUNC(4)
AM_VEC_NEWINDEX_FUNC(4)
VEC_NEWINDEX_FUNC(4)
VEC_OP_FUNC(4, add, +)
VEC_OP_FUNC(4, sub, -)
VEC_MUL_FUNC(4)
VEC_DIV_FUNC(4)
VEC_UNM_FUNC(4)
VEC_LEN_FUNC(4)
AM_READ_VEC_FUNC(4)

//-------------------------- mat2 --------------------------------//

MAT_NEW_FUNC(2)
MAT_INDEX_FUNC(2)
MAT_NEWINDEX_FUNC(2)
MAT_OP_FUNC(2, add, +)
MAT_OP_FUNC(2, sub, -)
MAT_OP_FUNC(2, div, /)
MAT_MUL_FUNC(2)
MAT_UNM_FUNC(2)
MAT_LEN_FUNC(2)

//-------------------------- mat3 --------------------------------//

MAT_NEW_FUNC(3)
MAT_INDEX_FUNC(3)
MAT_NEWINDEX_FUNC(3)
MAT_OP_FUNC(3, add, +)
MAT_OP_FUNC(3, sub, -)
MAT_OP_FUNC(3, div, /)
MAT_MUL_FUNC(3)
MAT_UNM_FUNC(3)
MAT_LEN_FUNC(3)

//-------------------------- mat4 --------------------------------//

MAT_NEW_FUNC(4)
MAT_INDEX_FUNC(4)
MAT_NEWINDEX_FUNC(4)
MAT_OP_FUNC(4, add, +)
MAT_OP_FUNC(4, sub, -)
MAT_OP_FUNC(4, div, /)
MAT_MUL_FUNC(4)
MAT_UNM_FUNC(4)
MAT_LEN_FUNC(4)

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

#define IS_ZERO(x) ((x) == 0.0f)

static int vec_normalize(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = am_new_userdata(L, am_vec2);
            y->v = glm::vec2(1.0f, 0.0f);
            if (!(
                IS_ZERO(x->v.x) &&
                IS_ZERO(x->v.y)
            )) y->v = glm::normalize(x->v);
            break;
        }
        case MT_am_vec3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = am_new_userdata(L, am_vec3);
            y->v = glm::vec3(1.0f, 0.0f, 0.0f);
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
            y->v = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
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
            float eta = luaL_checknumber(L, 3);
            am_vec2 *r = am_new_userdata(L, am_vec2);
            r->v = glm::refract(I->v, N->v, eta);
            break;
        }
        case MT_am_vec3: {
            am_vec3 *I = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *N = am_get_userdata(L, am_vec3, 2);
            float eta = luaL_checknumber(L, 3);
            am_vec3 *r = am_new_userdata(L, am_vec3);
            r->v = glm::refract(I->v, N->v, eta);
            break;
        }
        case MT_am_vec4: {
            am_vec4 *I = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *N = am_get_userdata(L, am_vec4, 2);
            float eta = luaL_checknumber(L, 3);
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
    float fovy = luaL_checknumber(L, 1);
    float aspect = luaL_checknumber(L, 2);
    float near = luaL_checknumber(L, 3);
    float far = luaL_checknumber(L, 4);
    am_mat4 *m = am_new_userdata(L, am_mat4);
    m->m = glm::perspective(fovy, aspect, near, far);
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

static int euleryxz3(lua_State *L) {
    am_check_nargs(L, 1);
    am_vec3 *angles = am_get_userdata(L, am_vec3, 1);
    am_mat3 *result = am_new_userdata(L, am_mat3);
    glm::mat4 m = glm::yawPitchRoll(angles->v.y, angles->v.x, angles->v.z);
    result->m[0] = glm::vec3(m[0].x, m[0].y, m[0].z);
    result->m[1] = glm::vec3(m[1].x, m[1].y, m[1].z);
    result->m[2] = glm::vec3(m[2].x, m[2].y, m[2].z);
    return 1;
}

static int euleryxz4(lua_State *L) {
    am_check_nargs(L, 1);
    am_vec3 *angles = am_get_userdata(L, am_vec3, 1);
    am_mat4 *result = am_new_userdata(L, am_mat4);
    result->m = glm::yawPitchRoll(angles->v.y, angles->v.x, angles->v.z);
    return 1;
}

inline static glm::vec4 plane_normalize(glm::vec4 plane) {
    float l = glm::length(glm::vec3(plane.x, plane.y, plane.z));
    return glm::vec4(plane.x/l, plane.y/l, plane.z/l, plane.w/l);
}

static int sphere_visible(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    glm::mat4 matrix = am_get_userdata(L, am_mat4, 1)->m;
    glm::vec4 center = glm::vec4(am_get_userdata(L, am_vec3, 2)->v, 1.0f);
    float radius = 0.0f;
    if (nargs > 2) {
        radius = luaL_checknumber(L, 3);
    }
    glm::vec4 left_plane = plane_normalize(glm::row(matrix, 0) + glm::row(matrix, 3));
    float left_dist = glm::dot(left_plane, center);
    if (left_dist + radius < 0.0f) {
        lua_pushboolean(L, 0);
        return 1;
    }
    glm::vec4 right_plane = plane_normalize(glm::row(-matrix, 0) + glm::row(matrix, 3));
    float right_dist = glm::dot(right_plane, center);
    if (right_dist + radius < 0.0f) {
        lua_pushboolean(L, 0);
        return 1;
    }
    glm::vec4 bottom_plane = plane_normalize(glm::row(matrix, 1) + glm::row(matrix, 3));
    float bottom_dist = glm::dot(bottom_plane, center);
    if (bottom_dist + radius < 0.0f) {
        lua_pushboolean(L, 0);
        return 1;
    }
    glm::vec4 top_plane = plane_normalize(glm::row(-matrix, 1) + glm::row(matrix, 3));
    float top_dist = glm::dot(top_plane, center);
    if (top_dist + radius < 0.0f) {
        lua_pushboolean(L, 0);
        return 1;
    }
    glm::vec4 near_plane = plane_normalize(glm::row(matrix, 2) + glm::row(matrix, 3));
    float near_dist = glm::dot(near_plane, center);
    if (near_dist + radius < 0.0f) {
        lua_pushboolean(L, 0);
        return 1;
    }
    glm::vec4 far_plane = plane_normalize(glm::row(-matrix, 2) + glm::row(matrix, 3));
    float far_dist = glm::dot(far_plane, center);
    if (far_dist + radius < 0.0f) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, 1);
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
        default:
            return luaL_error(L, "expecting a mat argument");
    }
    return 1;
}

//----------------------- noise functions -------------------------//

static int simplex(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case MT_am_vec2: {
            lua_pushnumber(L, glm::simplex(((am_vec2*)lua_touserdata(L, 1))->v));
            break;
        }
        case MT_am_vec3: {
            lua_pushnumber(L, glm::simplex(((am_vec3*)lua_touserdata(L, 1))->v));
            break;
        }
        case MT_am_vec4: {
            lua_pushnumber(L, glm::simplex(((am_vec4*)lua_touserdata(L, 1))->v));
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
    am_check_nargs(L, 1);
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
                    float a = lua_tonumber(L, 3);
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
                    float a = lua_tonumber(L, 3);
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
                    float a = lua_tonumber(L, 3);
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
        default:
            return luaL_error(L, "expecting a number or vec in position 1");
    }
    return 1;
}

/*
 * XXX does not compile with msvc.
static int smoothstep(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_type(L, 1)) {
        case LUA_TNUMBER: {
            float x = lua_tonumber(L, 1);
            float y = luaL_checknumber(L, 2);
            switch (am_get_type(L, 2)) {
                case LUA_TNUMBER: {
                    float a = luaL_checknumber(L, 3);
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

static inline lua_Number smootherstep0(lua_Number edge0, lua_Number edge1, lua_Number x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
    return x*x*x*(x*(x*6.0 - 15.0) + 10.0);
}

static inline glm::vec2 smootherstep0(float edge0, float edge1, glm::vec2 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0f, 1.0f);
    return x*x*x*(x*(x*6.0f - 15.0f) + 10.0f);
}

static inline glm::vec3 smootherstep0(float edge0, float edge1, glm::vec3 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0f, 1.0f);
    return x*x*x*(x*(x*6.0f - 15.0f) + 10.0f);
}

static inline glm::vec4 smootherstep0(float edge0, float edge1, glm::vec4 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0f, 1.0f);
    return x*x*x*(x*(x*6.0f - 15.0f) + 10.0f);
}

static inline glm::vec2 smootherstep0(glm::vec2 edge0, glm::vec2 edge1, glm::vec2 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0f, 1.0f);
    return x*x*x*(x*(x*6.0f - 15.0f) + 10.0f);
}

static inline glm::vec3 smootherstep0(glm::vec3 edge0, glm::vec3 edge1, glm::vec3 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0f, 1.0f);
    return x*x*x*(x*(x*6.0f - 15.0f) + 10.0f);
}

static inline glm::vec4 smootherstep0(glm::vec4 edge0, glm::vec4 edge1, glm::vec4 x) {
    x = glm::clamp((x - edge0)/(edge1 - edge0), 0.0f, 1.0f);
    return x*x*x*(x*(x*6.0f - 15.0f) + 10.0f);
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
                    float a = lua_tonumber(L, 2);
                    float b = lua_tonumber(L, 3);
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
                    float a = lua_tonumber(L, 2);
                    float b = lua_tonumber(L, 3);
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
                    float a = lua_tonumber(L, 2);
                    float b = lua_tonumber(L, 3);
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
    lua_pushcclosure(L, T##_newindex, 0);               \
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
    am_register_metatable(L, #T, MTID, 0);

#define REGISTER_MAT_MT(T, MTID)                        \
    lua_newtable(L);                                    \
    lua_pushcclosure(L, T##_index, 0);                  \
    lua_setfield(L, -2, "__index");                     \
    lua_pushcclosure(L, T##_newindex, 0);               \
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
    lua_pushcclosure(L, T##_set, 0);                    \
    lua_setfield(L, -2, "set");                         \
    am_register_metatable(L, #T, MTID, 0);

void am_open_math_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"vec2",        vec2_new},
        {"vec3",        vec3_new},
        {"vec4",        vec4_new},
        {"mat2",        mat2_new},
        {"mat3",        mat3_new},
        {"mat4",        mat4_new},
        {"length",      vec_length},
        {"distance",    vec_distance},
        {"dot",         vec_dot},
        {"cross",       vec_cross},
        {"normalize",   vec_normalize},
        {"faceforward", vec_faceforward},
        {"reflect",     vec_reflect},
        {"refract",     vec_refract},
        {"perspective", perspective},
        {"lookat",      lookat},
        {"euleryxz3",   euleryxz3},
        {"euleryxz4",   euleryxz4},
        {"sphere_visible", sphere_visible},
        {"inverse",     inverse},
        {"simplex",     simplex},
        {"perlin",      perlin},
        {"mix",         mix},
        {"clamp",       clamp},
        {"fract",       fract},
        //{"smoothstep",  smoothstep},
        {"smootherstep",smootherstep},
        {NULL, NULL}
    };
    am_open_module(L, "math", funcs);
    REGISTER_VEC_MT(vec2, MT_am_vec2)
    REGISTER_VEC_MT(vec3, MT_am_vec3)
    REGISTER_VEC_MT(vec4, MT_am_vec4)
    REGISTER_MAT_MT(mat2, MT_am_mat2)
    REGISTER_MAT_MT(mat3, MT_am_mat3)
    REGISTER_MAT_MT(mat4, MT_am_mat4)
}
