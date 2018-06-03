#include "amulet.h"

void am_buffer_view::update_max_elem_if_required() {
    if (last_max_elem_version < buffer->version) {
        switch (type) {
            case AM_VIEW_TYPE_USHORT_ELEM: {
                uint8_t *ptr = buffer->data + offset;
                uint16_t max = 0;
                for (int i = 0; i < size; i++) {
                    uint16_t val = *((uint16_t*)ptr);
                    if (val > max) max = val;
                    ptr += stride;
                }
                max_elem = max;
                break;
            }
            case AM_VIEW_TYPE_UINT_ELEM: {
                uint8_t *ptr = buffer->data + offset;
                uint32_t max = 0;
                for (int i = 0; i < size; i++) {
                    uint32_t val = *((uint32_t*)ptr);
                    if (val > max) max = val;
                    ptr += stride;
                }
                max_elem = max;
                break;
            }
            default:
                break;
        }
        last_max_elem_version = buffer->version;
    }
}

am_buffer_view* am_check_buffer_view(lua_State *L, int idx) {
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, idx);
    if (view->buffer->data == NULL && view->buffer->size > 0) {
        luaL_error(L, "attempt to access freed buffer");
    }
    return view;
}

am_buffer_view* am_new_buffer_view(lua_State *L, am_buffer_view_type type) {
    int mtid = (int)MT_am_buffer_view + (int)type + 1;
    assert(mtid < (int)MT_VIEW_TYPE_END_MARKER);
    // use the element-type specific metatable
    am_buffer_view *view = (am_buffer_view*)
        am_set_metatable(L,
            new (lua_newuserdata(L, sizeof(am_buffer_view))) am_buffer_view(),
            mtid);
    view->type = type;
    view->buffer = NULL;
    view->buffer_ref = LUA_NOREF;
    view->offset = 0;
    view->stride = 0;
    view->size = 0;
    view->max_elem = 0;
    view->last_max_elem_version = 0;
    return view;
}

static lua_Number read_num_float(uint8_t *ptr) {
    return *((float*)ptr);
}

static lua_Number read_num_ubyte(uint8_t *ptr) {
    return *((uint8_t*)ptr);
}

static lua_Number read_num_byte(uint8_t *ptr) {
    return *((int8_t*)ptr);
}

static lua_Number read_num_ubyte_norm(uint8_t *ptr) {
    return ((lua_Number)(*((uint8_t*)ptr))) / 255.0;
}

static lua_Number read_num_byte_norm(uint8_t *ptr) {
    return am_max(((lua_Number)(*((int8_t*)ptr))) / 127.0, -1.0);
}

static lua_Number read_num_ushort(uint8_t *ptr) {
    return *((uint16_t*)ptr);
}

static lua_Number read_num_short(uint8_t *ptr) {
    return *((int16_t*)ptr);
}

static lua_Number read_num_ushort_elem(uint8_t *ptr) {
    return *((uint16_t*)ptr) + 1;
}

static lua_Number read_num_ushort_norm(uint8_t *ptr) {
    return ((lua_Number)(*((uint16_t*)ptr))) / 65535.0;
}

static lua_Number read_num_short_norm(uint8_t *ptr) {
    return am_max(((lua_Number)(*((int16_t*)ptr))) / 32767.0, -1.0);
}

static lua_Number read_num_uint(uint8_t *ptr) {
    return *((uint32_t*)ptr);
}

static lua_Number read_num_int(uint8_t *ptr) {
    return *((int32_t*)ptr);
}

static lua_Number read_num_uint_elem(uint8_t *ptr) {
    return *((uint32_t*)ptr) + 1;
}

struct view_type_info {
    const char *name;
    int size;
    bool normalized;
    int components;
    am_buffer_view_type base_type;
    bool can_be_gl_attrib;
    am_attribute_client_type gl_client_type;
    lua_Number(*num_reader)(uint8_t*);
};

static view_type_info view_type_infos[] = {
    {"float",          4,  false,  1, AM_VIEW_TYPE_FLOAT,         true,  AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   &read_num_float,        },
    {"vec2",           8,  false,  2, AM_VIEW_TYPE_FLOAT,         true,  AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   NULL,                   },
    {"vec3",          12,  false,  3, AM_VIEW_TYPE_FLOAT,         true,  AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   NULL,                   },
    {"vec4",          16,  false,  4, AM_VIEW_TYPE_FLOAT,         true,  AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   NULL,                   },
    {"ubyte",          1,  false,  1, AM_VIEW_TYPE_UBYTE,         true,  AM_ATTRIBUTE_CLIENT_TYPE_UBYTE,   &read_num_ubyte,        },
    {"byte",           1,  false,  1, AM_VIEW_TYPE_BYTE,          true,  AM_ATTRIBUTE_CLIENT_TYPE_BYTE,    &read_num_byte,         },
    {"ubyte_norm",     1,  true,   1, AM_VIEW_TYPE_UBYTE_NORM,    true,  AM_ATTRIBUTE_CLIENT_TYPE_UBYTE,   &read_num_ubyte_norm,   },
    {"ubyte_norm4",    4,  true,   4, AM_VIEW_TYPE_UBYTE_NORM,    true,  AM_ATTRIBUTE_CLIENT_TYPE_UBYTE,   NULL,                   },
    {"byte_norm",      1,  true,   1, AM_VIEW_TYPE_BYTE_NORM,     true,  AM_ATTRIBUTE_CLIENT_TYPE_BYTE,    &read_num_byte_norm,    },
    {"ushort",         2,  false,  1, AM_VIEW_TYPE_USHORT,        true,  AM_ATTRIBUTE_CLIENT_TYPE_USHORT,  &read_num_ushort,       },
    {"short",          2,  false,  1, AM_VIEW_TYPE_SHORT,         true,  AM_ATTRIBUTE_CLIENT_TYPE_SHORT,   &read_num_short,        },
    {"ushort_elem",    2,  false,  1, AM_VIEW_TYPE_USHORT_ELEM,   false, AM_ATTRIBUTE_CLIENT_TYPE_USHORT,  &read_num_ushort_elem,  },
    {"ushort_norm",    2,  true,   1, AM_VIEW_TYPE_USHORT_NORM,   true,  AM_ATTRIBUTE_CLIENT_TYPE_USHORT,  &read_num_ushort_norm,  },
    {"short_norm",     2,  true,   1, AM_VIEW_TYPE_SHORT_NORM,    true,  AM_ATTRIBUTE_CLIENT_TYPE_SHORT,   &read_num_short_norm,   },
    {"uint",           4,  false,  1, AM_VIEW_TYPE_UINT,          false, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   &read_num_uint,         },
    {"int",            4,  false,  1, AM_VIEW_TYPE_INT,           false, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   &read_num_int,          },
    {"uint_elem",      4,  false,  1, AM_VIEW_TYPE_UINT_ELEM,     false, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   &read_num_uint_elem,    },
};
ct_check_array_size(view_type_infos, AM_NUM_VIEW_TYPES);

bool am_buffer_view::is_normalized() {
    return view_type_infos[type].normalized;
}

int am_buffer_view::num_components() {
    return view_type_infos[type].components;
}

bool am_buffer_view::can_be_gl_attrib() {
    return view_type_infos[type].can_be_gl_attrib;
}

am_attribute_client_type am_buffer_view::gl_client_type() {
    return view_type_infos[type].gl_client_type;
}

int am_create_buffer_view(lua_State *L) {
    int nargs = am_check_nargs(L, 2);

    am_buffer *buf = am_check_buffer(L, 1);
    am_buffer_view_type type = am_get_enum(L, am_buffer_view_type, 2);

    int type_size = view_type_infos[type].size;

    int offset = 0;
    if (nargs > 2) {
        offset = luaL_checkinteger(L, 3);
    }
    int stride = type_size;
    if (nargs > 3) {
        stride = luaL_checkinteger(L, 4);
    }

    int max_size = 0;
    if (buf->size - offset - type_size >= 0) {
        max_size = (buf->size - offset - type_size) / stride + 1;
    }
    int size;
    if (nargs > 4) {
        size = luaL_checkinteger(L, 5);
        if (size > max_size) {
            return luaL_error(L,
                "the buffer is too small for %d %ss with stride %d (max %d)",
                size, lua_tostring(L, 2), stride, max_size);
        }
    } else {
        size = max_size;
    }

    am_buffer_view *view = am_new_buffer_view(L, type);

    view->buffer = buf;
    view->buffer_ref = view->ref(L, 1);
    view->offset = offset;
    view->stride = stride;
    view->size = size;

    return 1;
}

static int view_slice(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_buffer_view *view = am_check_buffer_view(L, 1);
    int start = luaL_checkinteger(L, 2);
    if (start < 1 || start > view->size) {
        return luaL_error(L, "slice start must be in the range [1, %d] (in fact %d)",
            view->size, start);
    }
    int size = -1;
    if (nargs > 2 && !lua_isnil(L, 3)) {
        size = luaL_checkinteger(L, 3);
        if (size < 0) {
            return luaL_error(L, "size must be non-negative");
        }
    }
    int stride_multiplier = 1;
    if (nargs > 3) {
        stride_multiplier = luaL_checkinteger(L, 4);
        if (stride_multiplier < 1) {
            return luaL_error(L, "stride multiplier must be positive");
        }
    }
    if (size == -1) {
        size = (view->size - start) / stride_multiplier + 1;
    } else {
        if (size > ((view->size - start) / stride_multiplier + 1)) {
            return luaL_error(L, "slice size must be <= %d (in fact %d)",
                (view->size - start) / stride_multiplier + 1, size);
        }
    }
    am_buffer_view *slice = am_new_buffer_view(L, view->type);
    slice->buffer = view->buffer;
    view->pushref(L, view->buffer_ref);
    slice->buffer_ref = slice->ref(L, -1);
    lua_pop(L, 1); // pop buffer
    slice->offset = view->offset + (start-1) * view->stride;
    slice->stride = view->stride * stride_multiplier;
    slice->size = size;
    slice->max_elem = view->max_elem;
    slice->last_max_elem_version = view->last_max_elem_version;
    return 1;
}

static int gen_range(lua_State *L) {
    am_check_nargs(L, 3);
    int n = luaL_checkinteger(L, 1);
    if (n < 2) {
        return luaL_error(L, "range count must be at least 2");
    }
    float start = (float)luaL_checknumber(L, 2);
    float end = (float)luaL_checknumber(L, 3);
    float inc = (end - start) / (float)(n-1);

    am_buffer *res_buf = am_push_new_buffer_and_init(L, 4 * n);
    float *data = (float*)res_buf->data;
    float x = start;
    for (int i = 0; i < n - 1; i++) {
        data[i] = x;
        x += inc;
    }
    data[n-1] = end;
    am_buffer_view *res_view = am_new_buffer_view(L, AM_VIEW_TYPE_FLOAT);
    res_view->buffer = res_buf;
    res_view->buffer_ref = res_view->ref(L, -2);
    lua_remove(L, -2); // buffer
    res_view->stride = 4;
    res_view->size = n;
    return 1;
}

static int gen_random(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_rand *rand;
    int arg0 = 1;
    if (am_get_type(L, 1) == MT_am_rand) {
        rand = am_get_userdata(L, am_rand, 1);
        arg0 = 2;
        nargs--;
    } else {
        rand = am_get_default_rand(L);
    }
    if (nargs == 0) {
        return luaL_error(L, "expecting more arguments");
    }
    int n = luaL_checkinteger(L, arg0);
    if (n < 1) {
        return luaL_error(L, "number of random numbers to generate must be positive");
    }
    float lo = 0.0;
    float hi = 1.0;
    if (nargs > 1) {
        lo = (float)luaL_checknumber(L, arg0 + 1);
    }
    if (nargs > 2) {
        hi = (float)luaL_checknumber(L, arg0 + 2);
    }
    float range = hi - lo;

    am_buffer *res_buf = am_push_new_buffer_and_init(L, 4 * n);
    float *data = (float*)res_buf->data;
    for (int i = 0; i < n; i++) {
        data[i] = rand->get_randf() * range + lo;
    }
    am_buffer_view *res_view = am_new_buffer_view(L, AM_VIEW_TYPE_FLOAT);
    res_view->buffer = res_buf;
    res_view->buffer_ref = res_view->ref(L, -2);
    lua_remove(L, -2); // buffer
    res_view->stride = 4;
    res_view->size = n;
    return 1;
}

static int view_cart(lua_State *L) {
    am_check_nargs(L, 2);
    am_buffer_view *view1 = am_check_buffer_view(L, 1);
    am_buffer_view *view2 = am_check_buffer_view(L, 2);
    int type1 = view1->type;
    int type2 = view2->type;
    view_type_info info1 = view_type_infos[type1];
    view_type_info info2 = view_type_infos[type2];
    if (info1.base_type != AM_VIEW_TYPE_FLOAT || info2.base_type != AM_VIEW_TYPE_FLOAT) {
        return luaL_error(L, "cart not supported for views of type %s and %s", info1.name, info2.name);
    }
    if (view1->offset & 3 || view1->stride & 3) {
        return luaL_error(L, "view must be 4-byte aligned");
    }
    if (view2->offset & 3 || view2->stride & 3) {
        return luaL_error(L, "view must be 4-byte aligned");
    }
    int components1 = view_type_infos[type1].components;
    int components2 = view_type_infos[type2].components;
    int components = components1 + components2;
    if (components > 4) {
        return luaL_error(L, "sum of cart view components must be <= 4");
    }
    am_buffer_view_type type = (am_buffer_view_type)((int)AM_VIEW_TYPE_FLOAT + (components - 1));
    am_buffer *buf = am_push_new_buffer_and_init(L, 4 * components * view1->size * view2->size);
    float *data1 = (float*)view1->buffer->data;
    float *data2 = (float*)view2->buffer->data;
    int stride1 = view1->stride >> 2;
    int stride2 = view2->stride >> 2;
    float *data = (float*)buf->data;
    int d = 0;
    for (int i = 0; i < view2->size; ++i) {
        for (int j = 0; j < view1->size; ++j) {
            for (int c = 0; c < components1; ++c) {
                data[d++] = data1[c];
            }
            for (int c = 0; c < components2; ++c) {
                data[d++] = data2[c];
            }
            data1 += stride1;
        }
        data1 = (float*)view1->buffer->data;
        data2 += stride2;
    }
    am_buffer_view *view = am_new_buffer_view(L, type);
    view->buffer = buf;
    view->buffer_ref = view->ref(L, -2);
    lua_remove(L, -2); // buffer
    view->stride = 4 * components;
    view->size = view1->size * view2->size;
    return 1;
}

static void get_view_buffer(lua_State *L, void *obj) {
    am_buffer_view *view = (am_buffer_view*)obj;
    view->pushref(L, view->buffer_ref);
}

static am_property view_buffer_property = {get_view_buffer, NULL};

static int view_len(lua_State *L) {
    am_buffer_view *view = am_check_buffer_view(L, 1);
    lua_pushinteger(L, view->size);
    return 1;
}

static void view_float_iter_setup(lua_State *L, int arg, int *type, 
        float **buf, int *stride, int *size, int *components, float *farr, const char *opname)
{
    am_check_nargs(L, arg);
    *type = am_get_type(L, arg);

    switch (*type) {
        case MT_am_buffer_view:  {
            am_buffer_view *view = am_check_buffer_view(L, arg);
            if (view->offset & 3 || view->stride & 3) {
                luaL_error(L, "view must be 4-byte aligned for op %s", opname);
            }
            if (view_type_infos[view->type].base_type != AM_VIEW_TYPE_FLOAT) {
                luaL_error(L, "op %s not supported for views of type %s", opname, view_type_infos[view->type].name);
            }
            *buf = (float*)(view->buffer->data + view->offset);
            *stride = view->stride >> 2;
            *size = view->size;
            *components = view_type_infos[view->type].components;
            break;
        }
        case LUA_TNUMBER: {
            lua_Number n = lua_tonumber(L, arg);
            farr[0] = (float)n;
            farr[1] = (float)n;
            farr[2] = (float)n;
            farr[3] = (float)n;
            *buf = farr;
            *stride = 0;
            *size = 1;
            *components = 1;
            break;
        }
        case MT_am_vec2: {
            am_vec2 *v = am_get_userdata(L, am_vec2, arg);
            farr[0] = v->v.x;
            farr[1] = v->v.y;
            *buf = farr;
            *stride = 0;
            *size = 1;
            *components = 2;
            break;
        }
        case MT_am_vec3: {
            am_vec3 *v = am_get_userdata(L, am_vec3, arg);
            farr[0] = v->v.x;
            farr[1] = v->v.y;
            farr[2] = v->v.z;
            *buf = farr;
            *stride = 0;
            *size = 1;
            *components = 3;
            break;
        }
        case MT_am_vec4: {
            am_vec4 *v = am_get_userdata(L, am_vec4, arg);
            farr[0] = v->v.x;
            farr[1] = v->v.y;
            farr[2] = v->v.z;
            farr[3] = v->v.w;
            *buf = farr;
            *stride = 0;
            *size = 1;
            *components = 4;
            break;
        }
        default:
            luaL_error(L, "op %s not supported for views of type %s", opname, am_get_typename(L, *type));
    }
}

#define TNAME float
#define CTYPE float
#define LUA_TYPE LUA_TNUMBER
#define GET_CTYPE(L, idx) ((float)(lua_tonumber(L, idx)))
#define PUSH_CTYPE(L, x) lua_pushnumber(L, x)
#include "am_view_template.inc"

#define TNAME vec2
#define CTYPE glm::vec2
#define LUA_TYPE MT_am_vec2
#define GET_CTYPE(L, idx) glm::vec2(am_get_userdata(L, am_vec2, idx)->v)
#define PUSH_CTYPE(L, x) am_new_userdata(L, am_vec2)->v = glm::dvec2(x)
#define VEC_SZ 2
#define GET_VEC_COMPONENT(L, idx) ((float)(lua_tonumber(L, idx)))
#include "am_view_template.inc"

#define TNAME vec3
#define CTYPE glm::vec3
#define LUA_TYPE MT_am_vec3
#define GET_CTYPE(L, idx) glm::vec3(am_get_userdata(L, am_vec3, idx)->v)
#define PUSH_CTYPE(L, x) am_new_userdata(L, am_vec3)->v = glm::dvec3(x)
#define VEC_SZ 3
#define GET_VEC_COMPONENT(L, idx) ((float)(lua_tonumber(L, idx)))
#include "am_view_template.inc"

#define TNAME vec4
#define CTYPE glm::vec4
#define LUA_TYPE MT_am_vec4
#define GET_CTYPE(L, idx) glm::vec4(am_get_userdata(L, am_vec4, idx)->v)
#define PUSH_CTYPE(L, x) am_new_userdata(L, am_vec4)->v = glm::dvec4(x)
#define VEC_SZ 4
#define GET_VEC_COMPONENT(L, idx) ((float)(lua_tonumber(L, idx)))
#include "am_view_template.inc"

struct ubyte_norm4_ctype {
    uint8_t vals[4];
    uint8_t& operator[](int index) {
        return vals[index];
    }
};

static ubyte_norm4_ctype get_ubyte_norm4_ctype(am_vec4 *v) {
    ubyte_norm4_ctype ub;
    ub.vals[0] = (uint8_t)(v->v.x * 255.0);
    ub.vals[1] = (uint8_t)(v->v.y * 255.0);
    ub.vals[2] = (uint8_t)(v->v.z * 255.0);
    ub.vals[3] = (uint8_t)(v->v.w * 255.0);
    return ub;
}

static glm::dvec4 ubyte_norm4_ctype_to_vec4d(ubyte_norm4_ctype ub) {
    glm::dvec4 v(
        (double)ub.vals[0] / 255.0,
        (double)ub.vals[1] / 255.0,
        (double)ub.vals[2] / 255.0,
        (double)ub.vals[3] / 255.0
    );
    return v;
}

#define TNAME ubyte_norm4
#define CTYPE ubyte_norm4_ctype
#define LUA_TYPE MT_am_vec4
#define GET_CTYPE(L, idx) (get_ubyte_norm4_ctype(am_get_userdata(L, am_vec4, idx)))
#define PUSH_CTYPE(L, x) am_new_userdata(L, am_vec4)->v = ubyte_norm4_ctype_to_vec4d(x)
#define VEC_SZ 4
#define GET_VEC_COMPONENT(L, idx) ((uint8_t)(lua_tonumber(L, idx)*255.0))
#include "am_view_template.inc"

#define TNAME uint
#define CTYPE uint32_t
#define MINVAL 0
#define MAXVAL UINT32_MAX
#include "am_clamped_view_template.inc"

#define TNAME int
#define CTYPE int32_t
#define MINVAL INT32_MIN
#define MAXVAL INT32_MAX
#include "am_clamped_view_template.inc"

#define TNAME ushort_elem
#define CTYPE uint16_t
#define LUA_TYPE LUA_TNUMBER
#define GET_CTYPE(L, idx) ((uint16_t)(lua_tonumber(L, idx)-1))
#define PUSH_CTYPE(L, x) lua_pushinteger(L, (x)+1)
#include "am_view_template.inc"

#define TNAME uint_elem
#define CTYPE uint32_t
#define LUA_TYPE LUA_TNUMBER
#define GET_CTYPE(L, idx) ((uint32_t)(lua_tonumber(L, idx)-1.0))
#define PUSH_CTYPE(L, x) lua_pushnumber(L, ((lua_Number)(x))+1.0)
#include "am_view_template.inc"

#include "am_generated_view_defs.inc"

#define SUFFIX add
#define OPNAME "+"
#define ARGS 2
#define COMPONENT_WISE
#define EXPORT 1
#define OP(a, b) ((a) + (b))
#include "am_view_op.inc"

#define SUFFIX mul
#define OPNAME "*"
#define ARGS 2
#define COMPONENT_WISE
#define EXPORT 1
#define OP(a, b) ((a) * (b))
#include "am_view_op.inc"

#define SUFFIX sub
#define OPNAME "-"
#define ARGS 2
#define COMPONENT_WISE
#define EXPORT 1
#define OP(a, b) ((a) - (b))
#include "am_view_op.inc"

#define SUFFIX div
#define OPNAME "/"
#define ARGS 2
#define COMPONENT_WISE
#define EXPORT 1
#define OP(a, b) ((a) / (b))
#include "am_view_op.inc"

#define SUFFIX mod
#define OPNAME "%"
#define ARGS 2
#define COMPONENT_WISE
#define OP(a, b) ((a) - floor((a)/(b))*(b))
#include "am_view_op.inc"

#define SUFFIX sin
#define OPNAME "sin"
#define ARGS 1
#define COMPONENT_WISE
#define OP(a) (sinf(a))
#include "am_view_op.inc"

#define SUFFIX cos
#define OPNAME "cos"
#define ARGS 1
#define COMPONENT_WISE
#define OP(a) (cosf(a))
#include "am_view_op.inc"

#define SUFFIX perlin
#define OPNAME "perlin"
#define ARGS 1
#define ELEMENT_WISE
#define RESULT_COMPONENTS 1
#define OP1_1(dst, src) (*(dst) = glm::perlin(glm::vec2((src)[0], 0.0f)))
#define OP1_2(dst, src) (*(dst) = glm::perlin(glm::vec2((src)[0], (src)[1])))
#define OP1_3(dst, src) (*(dst) = glm::perlin(glm::vec3((src)[0], (src)[1], (src)[2])))
#define OP1_4(dst, src) (*(dst) = glm::perlin(glm::vec4((src)[0], (src)[1], (src)[2], (src)[3])))
#include "am_view_op.inc"

#define SUFFIX simplex
#define OPNAME "simplex"
#define ARGS 1
#define ELEMENT_WISE
#define RESULT_COMPONENTS 1
#define OP1_1(dst, src) (*(dst) = glm::simplex(glm::vec2((src)[0], 0.0f)))
#define OP1_2(dst, src) (*(dst) = glm::simplex(glm::vec2((src)[0], (src)[1])))
#define OP1_3(dst, src) (*(dst) = glm::simplex(glm::vec3((src)[0], (src)[1], (src)[2])))
#define OP1_4(dst, src) (*(dst) = glm::simplex(glm::vec4((src)[0], (src)[1], (src)[2], (src)[3])))
#include "am_view_op.inc"

#define SUFFIX dot
#define OPNAME "dot"
#define ARGS 2
#define ELEMENT_WISE
#define RESULT_COMPONENTS 1
#define OP2_2(dst, src1, src2) (*(dst) = glm::dot(glm::vec2((src1)[0], (src1)[1]), glm::vec2((src2)[0], (src2)[1])))
#define OP2_3(dst, src1, src2) (*(dst) = glm::dot(glm::vec3((src1)[0], (src1)[1], (src1)[2]), glm::vec3((src2)[0], (src2)[1], (src2)[2])))
#define OP2_4(dst, src1, src2) (*(dst) = glm::dot(glm::vec4((src1)[0], (src1)[1], (src1)[2], (src1)[3]), glm::vec4((src2)[0], (src2)[1], (src2)[2], (src2)[3])))
#include "am_view_op.inc"

#define SUFFIX vec2
#define OPNAME "vec2"
#define MIN_ARGS 1
#define MAX_ARGS 2
#define VEC_BUILDER
#define RESULT_COMPONENTS 2
#define OP(dst, f1, f2) {(dst)[0] = f1; (dst)[1] = f2;}
#include "am_view_op.inc"

#define SUFFIX vec3
#define OPNAME "vec3"
#define MIN_ARGS 1
#define MAX_ARGS 3
#define VEC_BUILDER
#define RESULT_COMPONENTS 3
#define OP(dst, f1, f2, f3) {(dst)[0] = f1; (dst)[1] = f2; (dst)[2] = f3;}
#include "am_view_op.inc"

#define SUFFIX vec4
#define OPNAME "vec4"
#define MIN_ARGS 1
#define MAX_ARGS 4
#define VEC_BUILDER
#define RESULT_COMPONENTS 4
#define OP(dst, f1, f2, f3, f4) {(dst)[0] = f1; (dst)[1] = f2; (dst)[2] = f3; (dst)[3] = f4;}
#include "am_view_op.inc"

static void register_view_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, view_len, 0);
    lua_setfield(L, -2, "__len");

    lua_pushcclosure(L, am_view_op_add, 0);
    lua_setfield(L, -2, "__add");
    lua_pushcclosure(L, am_view_op_sub, 0);
    lua_setfield(L, -2, "__sub");
    lua_pushcclosure(L, am_view_op_mul, 0);
    lua_setfield(L, -2, "__mul");
    lua_pushcclosure(L, am_view_op_div, 0);
    lua_setfield(L, -2, "__div");
    lua_pushcclosure(L, view_op_mod, 0);
    lua_setfield(L, -2, "__mod");

    am_register_property(L, "buffer", &view_buffer_property);

    lua_pushcclosure(L, view_slice, 0);
    lua_setfield(L, -2, "slice");

    am_register_metatable(L, "view", MT_am_buffer_view, 0);
}

void am_open_view_module(lua_State *L) {
    luaL_Reg vfuncs[] = {
        // generators
        {"range", gen_range},
        {"random", gen_random},
        // shape changing
        {"cart", view_cart},
        {"vec2", view_op_vec2},
        {"vec3", view_op_vec3},
        {"vec4", view_op_vec4},
        // math functions
        {"sin", view_op_sin},
        {"cos", view_op_cos},
        // other functions
        {"perlin", view_op_perlin},
        {"simplex", view_op_simplex},
        {"dot", view_op_dot},
        {NULL, NULL}
    };
    am_open_module(L, "mathv", vfuncs);

    am_enum_value view_type_enum[] = {
        {"float",           AM_VIEW_TYPE_FLOAT},
        {"vec2",            AM_VIEW_TYPE_FLOAT2},
        {"vec3",            AM_VIEW_TYPE_FLOAT3},
        {"vec4",            AM_VIEW_TYPE_FLOAT4},
        {"ubyte",           AM_VIEW_TYPE_UBYTE},
        {"byte",            AM_VIEW_TYPE_BYTE},
        {"ubyte_norm",      AM_VIEW_TYPE_UBYTE_NORM},
        {"ubyte_norm4",     AM_VIEW_TYPE_UBYTE_NORM4},
        {"byte_norm",       AM_VIEW_TYPE_BYTE_NORM},
        {"ushort",          AM_VIEW_TYPE_USHORT},
        {"short",           AM_VIEW_TYPE_SHORT},
        {"ushort_elem",     AM_VIEW_TYPE_USHORT_ELEM},
        {"ushort_norm",     AM_VIEW_TYPE_USHORT_NORM},
        {"short_norm",      AM_VIEW_TYPE_SHORT_NORM},
        {"uint",            AM_VIEW_TYPE_UINT},
        {"int",             AM_VIEW_TYPE_INT},
        {"uint_elem",       AM_VIEW_TYPE_UINT_ELEM},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_buffer_view_type, view_type_enum);

    register_view_mt(L);
    register_float_view_mt(L);
    register_vec2_view_mt(L);
    register_vec3_view_mt(L);
    register_vec4_view_mt(L);
    register_ubyte_view_mt(L);
    register_byte_view_mt(L);
    register_ubyte_norm_view_mt(L);
    register_ubyte_norm4_view_mt(L);
    register_byte_norm_view_mt(L);
    register_ushort_view_mt(L);
    register_short_view_mt(L);
    register_ushort_elem_view_mt(L);
    register_ushort_norm_view_mt(L);
    register_short_norm_view_mt(L);
    register_uint_view_mt(L);
    register_int_view_mt(L);
    register_uint_elem_view_mt(L);
}
