#include "amulet.h"

static int vec2_new(lua_State *L);
static int vec2_index(lua_State *L);
static int vec2_newindex(lua_State *L);
static int vec2_add(lua_State *L);
static int vec2_sub(lua_State *L);
static int vec2_mul(lua_State *L);
static int vec2_div(lua_State *L);
static int vec2_unm(lua_State *L);

static int vec3_new(lua_State *L);
static int vec3_index(lua_State *L);
static int vec3_newindex(lua_State *L);
static int vec3_add(lua_State *L);
static int vec3_sub(lua_State *L);
static int vec3_mul(lua_State *L);
static int vec3_div(lua_State *L);
static int vec3_unm(lua_State *L);

static int vec4_new(lua_State *L);
static int vec4_index(lua_State *L);
static int vec4_newindex(lua_State *L);
static int vec4_add(lua_State *L);
static int vec4_sub(lua_State *L);
static int vec4_mul(lua_State *L);
static int vec4_div(lua_State *L);
static int vec4_unm(lua_State *L);

static int mat2_new(lua_State *L);
static int mat2_index(lua_State *L);
static int mat2_newindex(lua_State *L);
static int mat2_add(lua_State *L);
static int mat2_sub(lua_State *L);
static int mat2_mul(lua_State *L);
static int mat2_div(lua_State *L);
static int mat2_unm(lua_State *L);

static int mat3_new(lua_State *L);
static int mat3_index(lua_State *L);
static int mat3_newindex(lua_State *L);
static int mat3_add(lua_State *L);
static int mat3_sub(lua_State *L);
static int mat3_mul(lua_State *L);
static int mat3_div(lua_State *L);
static int mat3_unm(lua_State *L);

static int mat4_new(lua_State *L);
static int mat4_index(lua_State *L);
static int mat4_newindex(lua_State *L);
static int mat4_add(lua_State *L);
static int mat4_sub(lua_State *L);
static int mat4_mul(lua_State *L);
static int mat4_div(lua_State *L);
static int mat4_unm(lua_State *L);

static int vec_length(lua_State *L);
static int vec_distance(lua_State *L);
static int vec_dot(lua_State *L);
static int vec_cross(lua_State *L);
static int vec_normalize(lua_State *L);
static int vec_faceforward(lua_State *L);
static int vec_reflect(lua_State *L);
static int vec_refract(lua_State *L);

#define REGISTER_MATH_MT(T, MTID)                       \
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
    lua_pushstring(L, #T);                              \
    lua_setfield(L, -2, "tname");                       \
    am_register_metatable(L, MTID, 0);

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
        {NULL, NULL}
    };
    am_open_module(L, "math", funcs);
    REGISTER_MATH_MT(vec2, AM_MT_VEC2)
    REGISTER_MATH_MT(vec3, AM_MT_VEC3)
    REGISTER_MATH_MT(vec4, AM_MT_VEC4)
    REGISTER_MATH_MT(mat2, AM_MT_MAT2)
    REGISTER_MATH_MT(mat3, AM_MT_MAT3)
    REGISTER_MATH_MT(mat4, AM_MT_MAT4)
}

//-------------------------- vec* helper macros ------------------//

#define VEC_OP_FUNC(D, OPNAME, OP)                                              \
static int vec##D##_##OPNAME(lua_State *L) {                                    \
    am_vec##D *z = new (lua_newuserdata(L, sizeof(am_vec##D))) glm::vec##D();   \
    am_set_metatable(L, AM_MT_VEC##D, -1);                                      \
    if (lua_isnumber(L, 1)) {                                                   \
        float x = lua_tonumber(L, 1);                                           \
        am_vec##D *y = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 2);   \
        *z = x OP *y;                                                           \
    } else if (lua_isnumber(L, 2)) {                                            \
        am_vec##D *x = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 1);   \
        float y = lua_tonumber(L, 2);                                           \
        *z = *x OP y;                                                           \
    } else {                                                                    \
        am_vec##D *x = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 1);   \
        am_vec##D *y = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 2);   \
        *z = *x OP *y;                                                          \
    }                                                                           \
    return 1;                                                                   \
}

#define VEC_MUL_FUNC(D)                                                         \
static int vec##D##_mul(lua_State *L) {                                         \
    am_vec##D *z = new (lua_newuserdata(L, sizeof(am_vec##D))) glm::vec##D();   \
    am_set_metatable(L, AM_MT_VEC##D, -1);                                      \
    if (lua_isnumber(L, 1)) {                                                    \
        float x = lua_tonumber(L, 1);                                           \
        am_vec##D *y = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 2);   \
        *z = x * *y;                                                            \
    } else if (lua_isnumber(L, 2)) {                                            \
        am_vec##D *x = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 1);   \
        float y = lua_tonumber(L, 2);                                           \
        *z = *x * y;                                                            \
    } else if (am_get_metatable_id(L, 2) == AM_MT_MAT##D) {                     \
        am_vec##D *x = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 1);   \
        am_mat##D *y = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 2);   \
        *z = *x * *y;                                                           \
    } else {                                                                    \
        am_vec##D *x = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 1);   \
        am_vec##D *y = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 2);   \
        *z = *x * *y;                                                           \
    }                                                                           \
    return 1;                                                                   \
}

#define VEC_UNM_FUNC(D)                                                     \
static int vec##D##_unm(lua_State *L) {                                     \
    am_vec##D *v = (am_vec##D*)lua_touserdata(L, 1);                        \
    new (lua_newuserdata(L, sizeof(am_vec##D))) glm::vec##D(-(*v));         \
    am_set_metatable(L, AM_MT_VEC##D, -1);                                  \
    return 1;                                                               \
}

#define VEC_NEW_FUNC(D)                                                                 \
static int vec##D##_new(lua_State *L) {                                                 \
    int n = lua_gettop(L);                                                              \
    if (n == 0) {                                                                       \
        return luaL_error(L, "vec" #D " constructor requires at least one argument");   \
    }                                                                                   \
    if (n == 1 && lua_isnumber(L, 1)) {                                                 \
        new (lua_newuserdata(L, sizeof(am_vec##D))) glm::vec##D((float)lua_tonumber(L, 1)); \
        am_set_metatable(L, AM_MT_VEC##D, -1);                                          \
    } else {                                                                            \
        am_vec##D *v = new (lua_newuserdata(L, sizeof(am_vec##D))) glm::vec##D();       \
        am_set_metatable(L, AM_MT_VEC##D, -1);                                          \
        int i = 0;                                                                      \
        int j;                                                                          \
        for (j = 1; j <= n; j++) {                                                      \
            if (lua_isnumber(L, j)) {                                                   \
                (*v)[i++] = lua_tonumber(L, j);                                         \
                if (i >= D) goto endfor;                                                \
            } else {                                                                    \
                switch (am_get_metatable_id(L, j)) {                                    \
                    case AM_MT_VEC2: {                                                  \
                        am_vec2 *va = (am_vec2*)lua_touserdata(L, j);                   \
                        (*v)[i++] = va->x;                                              \
                        if (i >= D) goto endfor;                                        \
                        (*v)[i++] = va->y;                                              \
                        if (i >= D) goto endfor;                                        \
                        break;                                                          \
                    }                                                                   \
                    case AM_MT_VEC3: {                                                  \
                        am_vec3 *va = (am_vec3*)lua_touserdata(L, j);                   \
                        (*v)[i++] = va->x;                                              \
                        if (i >= D) goto endfor;                                        \
                        (*v)[i++] = va->y;                                              \
                        if (i >= D) goto endfor;                                        \
                        (*v)[i++] = va->z;                                              \
                        if (i >= D) goto endfor;                                        \
                        break;                                                          \
                    }                                                                   \
                    case AM_MT_VEC4: {                                                  \
                        am_vec4 *va = (am_vec4*)lua_touserdata(L, j);                   \
                        (*v)[i++] = va->x;                                              \
                        if (i >= D) goto endfor;                                        \
                        (*v)[i++] = va->y;                                              \
                        if (i >= D) goto endfor;                                        \
                        (*v)[i++] = va->z;                                              \
                        if (i >= D) goto endfor;                                        \
                        (*v)[i++] = va->w;                                              \
                        if (i >= D) goto endfor;                                        \
                        break;                                                          \
                    }                                                                   \
                    default: return luaL_error(L,                                       \
                        "unexpected type in vec" #D " argument list (%d)", am_get_metatable_id(L, i));                  \
                }                                                                       \
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
static int vec##D##_index(lua_State *L) {                                               \
    am_vec##D *v = (am_vec##D*)lua_touserdata(L, 1);                                    \
    if (lua_type(L, 2) == LUA_TSTRING) {                                                \
        size_t len;                                                                     \
        const char *str = lua_tolstring(L, 2, &len);                                    \
        if (len == 1) {                                                                 \
            switch (str[0]) {                                                           \
                case 'x':                                                               \
                case 'r':                                                               \
                case 's':                                                               \
                    lua_pushnumber(L, (*v)[0]);                                         \
                    break;                                                              \
                case 'y':                                                               \
                case 'g':                                                               \
                case 't':                                                               \
                    lua_pushnumber(L, (*v)[1]);                                         \
                    break;                                                              \
                case 'z':                                                               \
                case 'b':                                                               \
                case 'p':                                                               \
                    if (D > 2) {                                                        \
                        lua_pushnumber(L, (*v)[2]);                                     \
                    } else {                                                            \
                        lua_pushnil(L);                                                 \
                    }                                                                   \
                    break;                                                              \
                case 'w':                                                               \
                case 'a':                                                               \
                case 'q':                                                               \
                    if (D > 3) {                                                        \
                        lua_pushnumber(L, (*v)[3]);                                     \
                    } else {                                                            \
                        lua_pushnil(L);                                                 \
                    }                                                                   \
                    break;                                                              \
                default:                                                                \
                    lua_pushnil(L);                                                     \
            }                                                                           \
        } else {                                                                        \
            switch (len) {                                                              \
                case 2: {                                                               \
                    am_vec2 *nv = new (lua_newuserdata(L, sizeof(am_vec2))) glm::vec2(); \
                    am_set_metatable(L, AM_MT_VEC2, -1);                                \
                    for (int i = 0; i < 2; i++) {                                       \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*nv)[i] = (*v)[os];                                        \
                        } else {                                                        \
                            lua_pop(L, 1);                                              \
                            lua_pushnil(L);                                             \
                            break;                                                      \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                case 3: {                                                               \
                    am_vec3 *nv = new (lua_newuserdata(L, sizeof(am_vec3))) glm::vec3(); \
                    am_set_metatable(L, AM_MT_VEC3, -1);                                \
                    for (int i = 0; i < 3; i++) {                                       \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*nv)[i] = (*v)[os];                                        \
                        } else {                                                        \
                            lua_pop(L, 1);                                              \
                            lua_pushnil(L);                                             \
                            break;                                                      \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
                case 4: {                                                               \
                    am_vec4 *nv = new (lua_newuserdata(L, sizeof(am_vec4))) glm::vec4(); \
                    am_set_metatable(L, AM_MT_VEC4, -1);                                \
                    for (int i = 0; i < 4; i++) {                                       \
                        int os = VEC_COMPONENT_OFFSET(str[i]);                          \
                        if (os >= 0 && os < D) {                                        \
                            (*nv)[i] = (*v)[os];                                        \
                        } else {                                                        \
                            lua_pop(L, 1);                                              \
                            lua_pushnil(L);                                             \
                            break;                                                      \
                        }                                                               \
                    }                                                                   \
                    break;                                                              \
                }                                                                       \
            }                                                                           \
        }                                                                               \
    } else {                                                                            \
        int i = lua_tointeger(L, 2);                                                    \
        if (i > 0 && i <= D) {                                                          \
            lua_pushnumber(L, (*v)[i-1]);                                               \
        } else {                                                                        \
            lua_pushnil(L);                                                             \
        }                                                                               \
    }                                                                                   \
    return 1;                                                                           \
}


#define VEC_NEWINDEX_FUNC(D)                                                            \
static int vec##D##_newindex(lua_State *L) {                                            \
    am_vec##D *v = (am_vec##D*)lua_touserdata(L, 1);                                    \
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
            switch (am_get_metatable_id(L, 3)) {                                        \
                case AM_MT_VEC2: {                                                      \
                    if (len != 2) goto fail;                                            \
                    am_vec2 *nv = (am_vec2*)lua_touserdata(L, 3);                       \
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
                case AM_MT_VEC3: {                                                      \
                    if (len != 3) goto fail;                                            \
                    am_vec3 *nv = (am_vec3*)lua_touserdata(L, 3);                       \
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
                case AM_MT_VEC4: {                                                      \
                    if (len != 4) goto fail;                                            \
                    am_vec4 *nv = (am_vec4*)lua_touserdata(L, 4);                       \
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
    return luaL_error(L, "invalid vec" #D " index or value");                           \
}

//-------------------------- mat* helper macros ------------------//

#define MAT_OP_FUNC(D, OPNAME, OP)                                                      \
    static int mat##D##_##OPNAME(lua_State *L) {                                        \
        am_mat##D *z = new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D();       \
        am_set_metatable(L, AM_MT_MAT##D, -1);                                          \
        if (lua_isnumber(L, 1)) {                                                       \
            float x = lua_tonumber(L, 1);                                               \
            am_mat##D *y = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 2);       \
            *z = x OP *y;                                                               \
        } else if (lua_isnumber(L, 2)) {                                                \
            am_mat##D *x = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 1);       \
            float y = lua_tonumber(L, 2);                                               \
            *z = *x OP y;                                                               \
        } else {                                                                        \
            am_mat##D *x = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 1);       \
            am_mat##D *y = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 2);       \
            *z = *x OP *y;                                                              \
        }                                                                               \
        return 1;                                                                       \
    }

#define MAT_MUL_FUNC(D)                                                                 \
    static int mat##D##_mul(lua_State *L) {                                             \
        if (lua_isnumber(L, 1)) {                                                       \
            float x = lua_tonumber(L, 1);                                               \
            am_mat##D *y = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 2);       \
            am_mat##D *z = new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D();   \
            am_set_metatable(L, AM_MT_MAT##D, -1);                                      \
            *z = x * *y;                                                                \
        } else if (lua_isnumber(L, 2)) {                                                \
            am_mat##D *x = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 1);       \
            float y = lua_tonumber(L, 2);                                               \
            am_mat##D *z = new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D();   \
            am_set_metatable(L, AM_MT_MAT##D, -1);                                      \
            *z = *x * y;                                                                \
        } else if (am_get_metatable_id(L, 2) == AM_MT_VEC##D) {                         \
            am_mat##D *x = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 1);       \
            am_vec##D *y = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 2);       \
            am_vec##D *z = new (lua_newuserdata(L, sizeof(am_vec##D))) glm::vec##D();   \
            am_set_metatable(L, AM_MT_VEC##D, -1);                                      \
            *z = *x * *y;                                                               \
        } else {                                                                        \
            am_mat##D *x = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 1);       \
            am_mat##D *y = (am_mat##D*)am_check_metatable_id(L, AM_MT_MAT##D, 2);       \
            am_mat##D *z = new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D();   \
            am_set_metatable(L, AM_MT_MAT##D, -1);                                      \
            *z = *x * *y;                                                               \
        }                                                                               \
        return 1;                                                                       \
    }

#define MAT_UNM_FUNC(D)                                                                 \
static int mat##D##_unm(lua_State *L) {                                                 \
    am_mat##D *m = (am_mat##D*)lua_touserdata(L, 1);                                    \
    new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D(-(*m));                     \
    am_set_metatable(L, AM_MT_MAT##D, -1);                                              \
    return 1;                                                                           \
}

#define MAT_NEW_FUNC(D)                                                                 \
static int mat##D##_new(lua_State *L) {                                                 \
    int n = lua_gettop(L);                                                              \
    if (n == 0) {                                                                       \
        return luaL_error(L, "mat" #D " constructor requires at least one argument");   \
    }                                                                                   \
    if (n == 1) {                                                                       \
        if (lua_isnumber(L, 1)) {                                                       \
            new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D((float)lua_tonumber(L, 1));\
            am_set_metatable(L, AM_MT_MAT##D, -1);                                      \
        } else {                                                                        \
            switch (am_get_metatable_id(L, 1)) {                                        \
                case AM_MT_MAT2: {                                                      \
                    am_mat2 *m = (am_mat2*)lua_touserdata(L, 1);                        \
                    new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D(*m);        \
                    am_set_metatable(L, AM_MT_MAT##D, -1);                              \
                    break;                                                              \
                }                                                                       \
                case AM_MT_MAT3: {                                                      \
                    am_mat3 *m = (am_mat3*)lua_touserdata(L, 1);                        \
                    new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D(*m);        \
                    am_set_metatable(L, AM_MT_MAT##D, -1);                              \
                    break;                                                              \
                }                                                                       \
                case AM_MT_MAT4: {                                                      \
                    am_mat4 *m = (am_mat4*)lua_touserdata(L, 1);                        \
                    new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D(*m);        \
                    am_set_metatable(L, AM_MT_MAT##D, -1);                              \
                    break;                                                              \
                }                                                                       \
                default:                                                                \
                    return luaL_error(L, "invalid mat" #D " constructor arguments");    \
            }                                                                           \
        }                                                                               \
    } else {                                                                            \
        am_mat##D *m = new (lua_newuserdata(L, sizeof(am_mat##D))) glm::mat##D();       \
        am_set_metatable(L, AM_MT_MAT##D, -1);                                          \
        int row = 0;                                                                    \
        int col = 0;                                                                    \
        int i;                                                                          \
        for (i = 1; i <= n; i++) {                                                      \
            if (lua_isnumber(L, i)) {                                                   \
                (*m)[col][row++] = lua_tonumber(L, i);                                  \
                if (row >= D) {col++; row=0;} if (col >= D) goto endfor;                \
            } else {                                                                    \
                switch (am_get_metatable_id(L, i)) {                                    \
                    case AM_MT_VEC2: {                                                  \
                        am_vec2 *va = (am_vec2*)lua_touserdata(L, i);                   \
                        (*m)[col][row++] = va->x;                                       \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->y;                                       \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        break;                                                          \
                    }                                                                   \
                    case AM_MT_VEC3: {                                                  \
                        am_vec3 *va = (am_vec3*)lua_touserdata(L, i);                   \
                        (*m)[col][row++] = va->x;                                       \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->y;                                       \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->z;                                       \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        break;                                                          \
                    }                                                                   \
                    case AM_MT_VEC4: {                                                  \
                        am_vec4 *va = (am_vec4*)lua_touserdata(L, i);                   \
                        (*m)[col][row++] = va->x;                                       \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->y;                                       \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->z;                                       \
                        if (row >= D) {col++; row=0;} if (col >= D) goto endfor;        \
                        (*m)[col][row++] = va->w;                                       \
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
    switch (i) {                                                                        \
        case 1:                                                                         \
        case 2:                                                                         \
        case 3:                                                                         \
        case 4:                                                                         \
            new (lua_newuserdata(L, sizeof(am_vec##D))) glm::vec##D((*m)[i-1]);         \
            am_set_metatable(L, AM_MT_VEC##D, -1);                                      \
            break;                                                                      \
        case 11:                                                                        \
            lua_pushnumber(L, (*m)[0][0]);                                              \
            break;                                                                      \
        case 12:                                                                        \
            lua_pushnumber(L, (*m)[0][1]);                                              \
            break;                                                                      \
        case 13:                                                                        \
            if (D > 2) lua_pushnumber(L, (*m)[0][2]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 14:                                                                        \
            if (D > 3) lua_pushnumber(L, (*m)[0][3]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 21:                                                                        \
            lua_pushnumber(L, (*m)[1][0]);                                              \
            break;                                                                      \
        case 22:                                                                        \
            lua_pushnumber(L, (*m)[1][1]);                                              \
            break;                                                                      \
        case 23:                                                                        \
            if (D > 2) lua_pushnumber(L, (*m)[1][2]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 24:                                                                        \
            if (D > 3) lua_pushnumber(L, (*m)[1][3]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 31:                                                                        \
            if (D > 2) lua_pushnumber(L, (*m)[2][0]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 32:                                                                        \
            if (D > 2) lua_pushnumber(L, (*m)[2][1]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 33:                                                                        \
            if (D > 2) lua_pushnumber(L, (*m)[2][2]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 34:                                                                        \
            if (D > 3) lua_pushnumber(L, (*m)[2][3]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 41:                                                                        \
            if (D > 3) lua_pushnumber(L, (*m)[3][0]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 42:                                                                        \
            if (D > 3) lua_pushnumber(L, (*m)[3][1]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 43:                                                                        \
            if (D > 3) lua_pushnumber(L, (*m)[3][2]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        case 44:                                                                        \
            if (D > 3) lua_pushnumber(L, (*m)[3][3]);                                   \
            else lua_pushnil(L);                                                        \
            break;                                                                      \
        default:                                                                        \
            lua_pushnil(L);                                                             \
    }                                                                                   \
    return 1;                                                                           \
}

#define MAT_NEWINDEX_FUNC(D)                                                            \
static int mat##D##_newindex(lua_State *L) {                                            \
    am_mat##D *m = (am_mat##D*)lua_touserdata(L, 1);                                    \
    int i = lua_tointeger(L, 2);                                                        \
    switch (i) {                                                                        \
        case 1:                                                                         \
        case 2:                                                                         \
        case 3:                                                                         \
        case 4: {                                                                       \
            am_vec##D *v = (am_vec##D*)am_check_metatable_id(L, AM_MT_VEC##D, 3);       \
            (*m)[i-1] = *v;                                                             \
            break;                                                                      \
        }                                                                               \
        case 11:                                                                        \
            (*m)[0][0] = luaL_checknumber(L, 3);                                        \
            break;                                                                      \
        case 12:                                                                        \
            (*m)[0][1] = luaL_checknumber(L, 3);                                        \
            break;                                                                      \
        case 13:                                                                        \
            if (D > 2) (*m)[0][2] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 14:                                                                        \
            if (D > 3) (*m)[0][3] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 21:                                                                        \
            (*m)[1][0] = luaL_checknumber(L, 3);                                        \
            break;                                                                      \
        case 22:                                                                        \
            (*m)[1][1] = luaL_checknumber(L, 3);                                        \
            break;                                                                      \
        case 23:                                                                        \
            if (D > 2) (*m)[1][2] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 24:                                                                        \
            if (D > 3) (*m)[1][3] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 31:                                                                        \
            if (D > 2) (*m)[2][0] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 32:                                                                        \
            if (D > 2) (*m)[2][1] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 33:                                                                        \
            if (D > 2) (*m)[2][2] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 34:                                                                        \
            if (D > 3) (*m)[2][3] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 41:                                                                        \
            if (D > 3) (*m)[3][0] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 42:                                                                        \
            if (D > 3) (*m)[3][1] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 43:                                                                        \
            if (D > 3) (*m)[3][2] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        case 44:                                                                        \
            if (D > 3) (*m)[3][3] = luaL_checknumber(L, 3);                             \
            else goto fail;                                                             \
            break;                                                                      \
        default:                                                                        \
            goto fail;                                                                  \
    }                                                                                   \
    return 1;                                                                           \
    fail:                                                                               \
    return luaL_error(L, "invalid mat" #D " index or value");                           \
}

//-------------------------- vec2 --------------------------------//

VEC_NEW_FUNC(2)
VEC_INDEX_FUNC(2)
VEC_NEWINDEX_FUNC(2)
VEC_OP_FUNC(2, add, +)
VEC_OP_FUNC(2, sub, -)
VEC_OP_FUNC(2, div, /)
VEC_MUL_FUNC(2)
VEC_UNM_FUNC(2)

//-------------------------- vec3 --------------------------------//

VEC_NEW_FUNC(3)
VEC_INDEX_FUNC(3)
VEC_NEWINDEX_FUNC(3)
VEC_OP_FUNC(3, add, +)
VEC_OP_FUNC(3, sub, -)
VEC_OP_FUNC(3, div, /)
VEC_MUL_FUNC(3)
VEC_UNM_FUNC(3)

//-------------------------- vec4 --------------------------------//

VEC_NEW_FUNC(4)
VEC_INDEX_FUNC(4)
VEC_NEWINDEX_FUNC(4)
VEC_OP_FUNC(4, add, +)
VEC_OP_FUNC(4, sub, -)
VEC_OP_FUNC(4, div, /)
VEC_MUL_FUNC(4)
VEC_UNM_FUNC(4)

//-------------------------- mat2 --------------------------------//

MAT_NEW_FUNC(2)
MAT_INDEX_FUNC(2)
MAT_NEWINDEX_FUNC(2)
MAT_OP_FUNC(2, add, +)
MAT_OP_FUNC(2, sub, -)
MAT_OP_FUNC(2, div, -)
MAT_MUL_FUNC(2)
MAT_UNM_FUNC(2)

//-------------------------- mat3 --------------------------------//

MAT_NEW_FUNC(3)
MAT_INDEX_FUNC(3)
MAT_NEWINDEX_FUNC(3)
MAT_OP_FUNC(3, add, +)
MAT_OP_FUNC(3, sub, -)
MAT_OP_FUNC(3, div, -)
MAT_MUL_FUNC(3)
MAT_UNM_FUNC(3)

//-------------------------- mat4 --------------------------------//

MAT_NEW_FUNC(4)
MAT_INDEX_FUNC(4)
MAT_NEWINDEX_FUNC(4)
MAT_OP_FUNC(4, add, +)
MAT_OP_FUNC(4, sub, -)
MAT_OP_FUNC(4, div, -)
MAT_MUL_FUNC(4)
MAT_UNM_FUNC(4)

//----------------------- vec functions -------------------------//

static int vec_length(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_metatable_id(L, 1)) {
        case AM_MT_VEC2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            lua_pushnumber(L, glm::length(*x));
            break;
        }
        case AM_MT_VEC3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            lua_pushnumber(L, glm::length(*x));
            break;
        }
        case AM_MT_VEC4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            lua_pushnumber(L, glm::length(*x));
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int vec_distance(lua_State *L) {
    am_check_nargs(L, 2);
    switch (am_get_metatable_id(L, 1)) {
        case AM_MT_VEC2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = (am_vec2*)am_check_metatable_id(L, AM_MT_VEC2, 2);
            lua_pushnumber(L, glm::distance(*x, *y));
            break;
        }
        case AM_MT_VEC3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = (am_vec3*)am_check_metatable_id(L, AM_MT_VEC3, 2);
            lua_pushnumber(L, glm::distance(*x, *y));
            break;
        }
        case AM_MT_VEC4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = (am_vec4*)am_check_metatable_id(L, AM_MT_VEC4, 2);
            lua_pushnumber(L, glm::distance(*x, *y));
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int vec_dot(lua_State *L) {
    am_check_nargs(L, 2);
    switch (am_get_metatable_id(L, 1)) {
        case AM_MT_VEC2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = (am_vec2*)am_check_metatable_id(L, AM_MT_VEC2, 2);
            lua_pushnumber(L, glm::dot(*x, *y));
            break;
        }
        case AM_MT_VEC3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = (am_vec3*)am_check_metatable_id(L, AM_MT_VEC3, 2);
            lua_pushnumber(L, glm::dot(*x, *y));
            break;
        }
        case AM_MT_VEC4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = (am_vec4*)am_check_metatable_id(L, AM_MT_VEC4, 2);
            lua_pushnumber(L, glm::dot(*x, *y));
            break;
        }
        default:
            return luaL_error(L, "expecting a vec argument");
    }
    return 1;
}

static int vec_cross(lua_State *L) {
    am_check_nargs(L, 2);
    am_vec3 *x = (am_vec3*)am_check_metatable_id(L, AM_MT_VEC3, 1);
    am_vec3 *y = (am_vec3*)am_check_metatable_id(L, AM_MT_VEC3, 2);
    am_vec3 *z = new (lua_newuserdata(L, sizeof(am_vec3))) glm::vec3();
    am_set_metatable(L, AM_MT_VEC3, -1);
    *z = glm::cross(*x, *y);
    return 1;
}

static int vec_normalize(lua_State *L) {
    am_check_nargs(L, 1);
    switch (am_get_metatable_id(L, 1)) {
        case AM_MT_VEC2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = new (lua_newuserdata(L, sizeof(am_vec2))) glm::vec2();
            am_set_metatable(L, AM_MT_VEC2, -1);
            *y = glm::normalize(*x);
            break;
        }
        case AM_MT_VEC3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = new (lua_newuserdata(L, sizeof(am_vec3))) glm::vec3();
            am_set_metatable(L, AM_MT_VEC3, -1);
            *y = glm::normalize(*x);
            break;
        }
        case AM_MT_VEC4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = new (lua_newuserdata(L, sizeof(am_vec4))) glm::vec4();
            am_set_metatable(L, AM_MT_VEC4, -1);
            *y = glm::normalize(*x);
            break;
        }
    }
    return 1;
}

static int vec_faceforward(lua_State *L) {
    am_check_nargs(L, 3);
    switch (am_get_metatable_id(L, 1)) {
        case AM_MT_VEC2: {
            am_vec2 *N = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *I = (am_vec2*)am_check_metatable_id(L, AM_MT_VEC2, 2);
            am_vec2 *Nref = (am_vec2*)am_check_metatable_id(L, AM_MT_VEC2, 3);
            am_vec2 *r = new (lua_newuserdata(L, sizeof(am_vec2))) glm::vec2();
            am_set_metatable(L, AM_MT_VEC2, -1);
            *r = glm::faceforward(*N, *I, *Nref);
            break;
        }
        case AM_MT_VEC3: {
            am_vec3 *N = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *I = (am_vec3*)am_check_metatable_id(L, AM_MT_VEC3, 2);
            am_vec3 *Nref = (am_vec3*)am_check_metatable_id(L, AM_MT_VEC3, 3);
            am_vec3 *r = new (lua_newuserdata(L, sizeof(am_vec3))) glm::vec3();
            am_set_metatable(L, AM_MT_VEC3, -1);
            *r = glm::faceforward(*N, *I, *Nref);
            break;
        }
        case AM_MT_VEC4: {
            am_vec4 *N = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *I = (am_vec4*)am_check_metatable_id(L, AM_MT_VEC4, 2);
            am_vec4 *Nref = (am_vec4*)am_check_metatable_id(L, AM_MT_VEC4, 3);
            am_vec4 *r = new (lua_newuserdata(L, sizeof(am_vec4))) glm::vec4();
            am_set_metatable(L, AM_MT_VEC4, -1);
            *r = glm::faceforward(*N, *I, *Nref);
            break;
        }
    }
    return 1;
}

static int vec_reflect(lua_State *L) {
    am_check_nargs(L, 2);
    switch (am_get_metatable_id(L, 1)) {
        case AM_MT_VEC2: {
            am_vec2 *x = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *y = (am_vec2*)am_check_metatable_id(L, AM_MT_VEC2, 2);
            am_vec2 *z = new (lua_newuserdata(L, sizeof(am_vec2))) glm::vec2();
            am_set_metatable(L, AM_MT_VEC2, -1);
            *z = glm::reflect(*x, *y);
            break;
        }
        case AM_MT_VEC3: {
            am_vec3 *x = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *y = (am_vec3*)am_check_metatable_id(L, AM_MT_VEC3, 2);
            am_vec3 *z = new (lua_newuserdata(L, sizeof(am_vec3))) glm::vec3();
            am_set_metatable(L, AM_MT_VEC3, -1);
            *z = glm::reflect(*x, *y);
            break;
        }
        case AM_MT_VEC4: {
            am_vec4 *x = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *y = (am_vec4*)am_check_metatable_id(L, AM_MT_VEC4, 2);
            am_vec4 *z = new (lua_newuserdata(L, sizeof(am_vec4))) glm::vec4();
            am_set_metatable(L, AM_MT_VEC4, -1);
            *z = glm::reflect(*x, *y);
            break;
        }
    }
    return 1;
}

static int vec_refract(lua_State *L) {
    am_check_nargs(L, 3);
    switch (am_get_metatable_id(L, 1)) {
        case AM_MT_VEC2: {
            am_vec2 *I = (am_vec2*)lua_touserdata(L, 1);
            am_vec2 *N = (am_vec2*)am_check_metatable_id(L, AM_MT_VEC2, 2);
            float eta = luaL_checknumber(L, 3);
            am_vec2 *r = new (lua_newuserdata(L, sizeof(am_vec2))) glm::vec2();
            am_set_metatable(L, AM_MT_VEC2, -1);
            *r = glm::refract(*I, *N, eta);
            break;
        }
        case AM_MT_VEC3: {
            am_vec3 *I = (am_vec3*)lua_touserdata(L, 1);
            am_vec3 *N = (am_vec3*)am_check_metatable_id(L, AM_MT_VEC3, 2);
            float eta = luaL_checknumber(L, 3);
            am_vec3 *r = new (lua_newuserdata(L, sizeof(am_vec3))) glm::vec3();
            am_set_metatable(L, AM_MT_VEC3, -1);
            *r = glm::refract(*I, *N, eta);
            break;
        }
        case AM_MT_VEC4: {
            am_vec4 *I = (am_vec4*)lua_touserdata(L, 1);
            am_vec4 *N = (am_vec4*)am_check_metatable_id(L, AM_MT_VEC4, 2);
            float eta = luaL_checknumber(L, 3);
            am_vec4 *r = new (lua_newuserdata(L, sizeof(am_vec4))) glm::vec4();
            am_set_metatable(L, AM_MT_VEC4, -1);
            *r = glm::refract(*I, *N, eta);
            break;
        }
    }
    return 1;
}
