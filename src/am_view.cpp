#include "amulet.h"

static void decode_view_type_lua(am_buffer_view_type_lua ltype, am_buffer_view_type *type, int *components) {
    switch (ltype) {
        case AM_VIEW_TYPE_LUA_F32_1:
            *type = AM_VIEW_TYPE_F32;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_F32_2:
            *type = AM_VIEW_TYPE_F32;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_F32_3:
            *type = AM_VIEW_TYPE_F32;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_F32_4:
            *type = AM_VIEW_TYPE_F32;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_F32_3x3:
            *type = AM_VIEW_TYPE_F32;
            *components = 9;
            return;
        case AM_VIEW_TYPE_LUA_F32_4x4:
            *type = AM_VIEW_TYPE_F32;
            *components = 16;
            return;
        case AM_VIEW_TYPE_LUA_F64_1:
            *type = AM_VIEW_TYPE_F64;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_F64_2:
            *type = AM_VIEW_TYPE_F64;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_F64_3:
            *type = AM_VIEW_TYPE_F64;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_F64_4:
            *type = AM_VIEW_TYPE_F64;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_F64_3x3:
            *type = AM_VIEW_TYPE_F64;
            *components = 9;
            return;
        case AM_VIEW_TYPE_LUA_F64_4x4:
            *type = AM_VIEW_TYPE_F64;
            *components = 16;
            return;
        case AM_VIEW_TYPE_LUA_U8_1:
            *type = AM_VIEW_TYPE_U8;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_U8_2:
            *type = AM_VIEW_TYPE_U8;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_U8_3:
            *type = AM_VIEW_TYPE_U8;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_U8_4:
            *type = AM_VIEW_TYPE_U8;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_I8_1:
            *type = AM_VIEW_TYPE_I8;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_I8_2:
            *type = AM_VIEW_TYPE_I8;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_I8_3:
            *type = AM_VIEW_TYPE_I8;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_I8_4:
            *type = AM_VIEW_TYPE_I8;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_U8N_1:
            *type = AM_VIEW_TYPE_U8N;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_U8N_2:
            *type = AM_VIEW_TYPE_U8N;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_U8N_3:
            *type = AM_VIEW_TYPE_U8N;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_U8N_4:
            *type = AM_VIEW_TYPE_U8N;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_I8N_1:
            *type = AM_VIEW_TYPE_I8N;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_I8N_2:
            *type = AM_VIEW_TYPE_I8N;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_I8N_3:
            *type = AM_VIEW_TYPE_I8N;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_I8N_4:
            *type = AM_VIEW_TYPE_I8N;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_U16_1:
            *type = AM_VIEW_TYPE_U16;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_U16_2:
            *type = AM_VIEW_TYPE_U16;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_U16_3:
            *type = AM_VIEW_TYPE_U16;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_U16_4:
            *type = AM_VIEW_TYPE_U16;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_I16_1:
            *type = AM_VIEW_TYPE_I16;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_I16_2:
            *type = AM_VIEW_TYPE_I16;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_I16_3:
            *type = AM_VIEW_TYPE_I16;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_I16_4:
            *type = AM_VIEW_TYPE_I16;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_U16E:
            *type = AM_VIEW_TYPE_U16E;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_U16N_1:
            *type = AM_VIEW_TYPE_U16N;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_U16N_2:
            *type = AM_VIEW_TYPE_U16N;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_U16N_3:
            *type = AM_VIEW_TYPE_U16N;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_U16N_4:
            *type = AM_VIEW_TYPE_U16N;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_I16N_1:
            *type = AM_VIEW_TYPE_I16N;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_I16N_2:
            *type = AM_VIEW_TYPE_I16N;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_I16N_3:
            *type = AM_VIEW_TYPE_I16N;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_I16N_4:
            *type = AM_VIEW_TYPE_I16N;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_U32_1:
            *type = AM_VIEW_TYPE_U32;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_U32_2:
            *type = AM_VIEW_TYPE_U32;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_U32_3:
            *type = AM_VIEW_TYPE_U32;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_U32_4:
            *type = AM_VIEW_TYPE_U32;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_I32_1:
            *type = AM_VIEW_TYPE_I32;
            *components = 1;
            return;
        case AM_VIEW_TYPE_LUA_I32_2:
            *type = AM_VIEW_TYPE_I32;
            *components = 2;
            return;
        case AM_VIEW_TYPE_LUA_I32_3:
            *type = AM_VIEW_TYPE_I32;
            *components = 3;
            return;
        case AM_VIEW_TYPE_LUA_I32_4:
            *type = AM_VIEW_TYPE_I32;
            *components = 4;
            return;
        case AM_VIEW_TYPE_LUA_U32E:
            *type = AM_VIEW_TYPE_U32E;
            *components = 1;
            return;
    }
    return;
}

void am_buffer_view::update_max_elem_if_required() {
    if (last_max_elem_version < buffer->version) {
        switch (type) {
            case AM_VIEW_TYPE_U16:
            case AM_VIEW_TYPE_U16E: {
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
            case AM_VIEW_TYPE_U32:
            case AM_VIEW_TYPE_U32E: {
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
    am_check_buffer_data(L, view->buffer);
    return view;
}

am_buffer_view* am_new_buffer_view(lua_State *L, am_buffer_view_type type, int components) {
    int mtid = (int)MT_am_buffer_view + (int)type + 1;
    assert(mtid < (int)MT_VIEW_TYPE_END_MARKER);
    // use the element-type specific metatable
    am_buffer_view *view = (am_buffer_view*)
        am_set_metatable(L,
            new (lua_newuserdata(L, sizeof(am_buffer_view))) am_buffer_view(),
            mtid);
    view->type = type;
    view->components = components;
    view->buffer = NULL;
    view->buffer_ref = LUA_NOREF;
    view->offset = 0;
    view->stride = 0;
    view->size = 0;
    view->max_elem = 0;
    view->last_max_elem_version = 0;
    return view;
}

static lua_Number read_num_f32(uint8_t *ptr) {
    return *((float*)ptr);
}

static lua_Number read_num_f64(uint8_t *ptr) {
    return *((double*)ptr);
}

static lua_Number read_num_u8(uint8_t *ptr) {
    return *((uint8_t*)ptr);
}

static lua_Number read_num_i8(uint8_t *ptr) {
    return *((int8_t*)ptr);
}

static lua_Number read_num_u8n(uint8_t *ptr) {
    return ((lua_Number)(*((uint8_t*)ptr))) / 255.0;
}

static lua_Number read_num_i8n(uint8_t *ptr) {
    return am_max(((lua_Number)(*((int8_t*)ptr))) / 127.0, -1.0);
}

static lua_Number read_num_u16(uint8_t *ptr) {
    return *((uint16_t*)ptr);
}

static lua_Number read_num_i16(uint8_t *ptr) {
    return *((int16_t*)ptr);
}

static lua_Number read_num_u16e(uint8_t *ptr) {
    return *((uint16_t*)ptr) + 1;
}

static lua_Number read_num_u16n(uint8_t *ptr) {
    return ((lua_Number)(*((uint16_t*)ptr))) / 65535.0;
}

static lua_Number read_num_i16n(uint8_t *ptr) {
    return am_max(((lua_Number)(*((int16_t*)ptr))) / 32767.0, -1.0);
}

static lua_Number read_num_u32(uint8_t *ptr) {
    return *((uint32_t*)ptr);
}

static lua_Number read_num_i32(uint8_t *ptr) {
    return *((int32_t*)ptr);
}

static lua_Number read_num_u32e(uint8_t *ptr) {
    return (double)(*((uint32_t*)ptr)) + 1.0;
}

am_view_type_info am_view_type_infos[] = {
    {"float",          4,  false, AM_VIEW_TYPE_F32,  true,  AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   &read_num_f32,     },
    {"double",         8,  false, AM_VIEW_TYPE_F64,  false, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   &read_num_f64,     },
    {"ubyte",          1,  false, AM_VIEW_TYPE_U8,   true,  AM_ATTRIBUTE_CLIENT_TYPE_UBYTE,   &read_num_u8,      },
    {"byte",           1,  false, AM_VIEW_TYPE_I8,   true,  AM_ATTRIBUTE_CLIENT_TYPE_BYTE,    &read_num_i8,      },
    {"ubyte_norm",     1,  true,  AM_VIEW_TYPE_U8N,  true,  AM_ATTRIBUTE_CLIENT_TYPE_UBYTE,   &read_num_u8n,     },
    {"byte_norm",      1,  true,  AM_VIEW_TYPE_I8N,  true,  AM_ATTRIBUTE_CLIENT_TYPE_BYTE,    &read_num_i8n,     },
    {"ushort",         2,  false, AM_VIEW_TYPE_U16,  true,  AM_ATTRIBUTE_CLIENT_TYPE_USHORT,  &read_num_u16,     },
    {"short",          2,  false, AM_VIEW_TYPE_I16,  true,  AM_ATTRIBUTE_CLIENT_TYPE_SHORT,   &read_num_i16,     },
    {"ushort_elem",    2,  false, AM_VIEW_TYPE_U16E, false, AM_ATTRIBUTE_CLIENT_TYPE_USHORT,  &read_num_u16e,    },
    {"ushort_norm",    2,  true,  AM_VIEW_TYPE_U16N, true,  AM_ATTRIBUTE_CLIENT_TYPE_USHORT,  &read_num_u16n,    },
    {"short_norm",     2,  true,  AM_VIEW_TYPE_I16N, true,  AM_ATTRIBUTE_CLIENT_TYPE_SHORT,   &read_num_i16n,    },
    {"uint",           4,  false, AM_VIEW_TYPE_U32,  false, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   &read_num_u32,     },
    {"int",            4,  false, AM_VIEW_TYPE_I32,  false, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   &read_num_i32,     },
    {"uint_elem",      4,  false, AM_VIEW_TYPE_U32E, false, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,   &read_num_u32e,    },
};
ct_check_array_size(am_view_type_infos, AM_NUM_VIEW_TYPES);

am_buffer_view::am_buffer_view() {
    type = AM_NUM_VIEW_TYPES;
    components = 0;

    buffer = NULL;
    buffer_ref = LUA_NOREF;

    offset = 0;
    stride = 0;
    size = 0;

    max_elem = 0;
    last_max_elem_version = 0;
}

bool am_buffer_view::is_normalized() {
    return am_view_type_infos[type].normalized;
}

bool am_buffer_view::can_be_gl_attrib() {
    return am_view_type_infos[type].can_be_gl_attrib;
}

am_attribute_client_type am_buffer_view::gl_client_type() {
    return am_view_type_infos[type].gl_client_type;
}

int am_create_buffer_view(lua_State *L) {
    int nargs = am_check_nargs(L, 2);

    am_buffer *buf = am_check_buffer(L, 1);
    am_buffer_view_type_lua ltype = am_get_enum(L, am_buffer_view_type_lua, 2);
    am_buffer_view_type type = AM_NUM_VIEW_TYPES; // avoid gcc warning
    int components = 0;
    decode_view_type_lua(ltype, &type, &components);

    int type_size = am_view_type_infos[type].size * components;

    int offset = 0;
    if (nargs > 2) {
        offset = luaL_checkinteger(L, 3);
    }
    int stride = type_size;
    if (nargs > 3) {
        stride = luaL_checkinteger(L, 4);
        if (stride < type_size) {
            return luaL_error(L, "stride (%d) must be greater than or equal to element size (%d)", stride, type_size);
        }
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

    am_buffer_view *view = am_new_buffer_view(L, type, components);

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
    if (nargs > 3 && !lua_isnil(L, 4)) {
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
    am_buffer_view *slice = am_new_buffer_view(L, view->type, view->components);
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

static int create_component_slice(lua_State *L, am_buffer_view *view, int start, int count) {
    int comp_sz = am_view_type_infos[view->type].size;
    am_buffer_view *slice = am_new_buffer_view(L, view->type, count);
    slice->buffer = view->buffer;
    view->pushref(L, view->buffer_ref);
    slice->buffer_ref = slice->ref(L, -1);
    lua_pop(L, 1); // pop buffer
    slice->offset = view->offset + start * comp_sz;
    slice->stride = view->stride;
    slice->size = view->size;
    slice->max_elem = view->max_elem;
    slice->last_max_elem_version = view->last_max_elem_version;
    return 1;
}

static int view_swizzle_index(lua_State *L, am_buffer_view *view) {
    if (lua_type(L, 2) == LUA_TSTRING) {
        size_t len;
        const char *str = lua_tolstring(L, 2, &len);
        switch (len) {
            case 1:
                switch (str[0]) {
                    case 'x':
                    case 'r':
                    case 's':
                        return create_component_slice(L, view, 0, 1);
                    case 'y':
                    case 'g':
                    case 't':
                        return create_component_slice(L, view, 1, 1);
                    case 'z':
                    case 'b':
                    case 'p':
                        return create_component_slice(L, view, 2, 1);
                    case 'w':
                    case 'a':
                    case 'q':
                        return create_component_slice(L, view, 3, 1);
                }
                break;
            case 2: {
                int c1 = VEC_COMPONENT_OFFSET(str[0]);
                int c2 = VEC_COMPONENT_OFFSET(str[1]);
                if (c1 >= 0 && c2 == c1 + 1 && view->components > c2) {
                    return create_component_slice(L, view, c1, 2);
                }
                break;
            }
            case 3: {
                int c1 = VEC_COMPONENT_OFFSET(str[0]);
                int c2 = VEC_COMPONENT_OFFSET(str[1]);
                int c3 = VEC_COMPONENT_OFFSET(str[2]);
                if (c1 >= 0 && c2 == c1 + 1 && c3 == c2 + 1 && view->components > c3) {
                    return create_component_slice(L, view, c1, 3);
                }
                break;
            }
            case 4: {
                int c1 = VEC_COMPONENT_OFFSET(str[0]);
                int c2 = VEC_COMPONENT_OFFSET(str[1]);
                int c3 = VEC_COMPONENT_OFFSET(str[2]);
                int c4 = VEC_COMPONENT_OFFSET(str[3]);
                if (c1 >= 0 && c2 == c1 + 1 && c3 == c2 + 1 && c4 == c3 + 1 && view->components > c4) {
                    return create_component_slice(L, view, c1, 4);
                }
                break;
            }
        }
    }
    return am_default_index_func(L);
}

#define TNAME F32
#define CTYPE float
#define FROM_LUA_NUM(x) ((float)(x))
#define READ_NUM(x) read_num_f32(x)
#include "am_view_template.inc"

#define TNAME F64
#define CTYPE double
#define FROM_LUA_NUM(x) ((double)(x))
#define READ_NUM(x) read_num_f64(x)
#include "am_view_template.inc"

#define TNAME U32
#define CTYPE uint32_t
#define FROM_LUA_NUM(x) ((uint32_t)am_clamp((x), 0.0, (double)UINT32_MAX))
#define READ_NUM(x) read_num_u32(x)
#include "am_view_template.inc"

#define TNAME I32
#define CTYPE int32_t
#define FROM_LUA_NUM(x) ((int32_t)am_clamp((x), (double)INT32_MIN, (double)INT32_MAX))
#define READ_NUM(x) read_num_i32(x)
#include "am_view_template.inc"

#define TNAME U16E
#define CTYPE uint16_t
#define FROM_LUA_NUM(x) ((uint16_t)(x-1.0))
#define READ_NUM(x) read_num_u16e(x)
#include "am_view_template.inc"

#define TNAME U32E
#define CTYPE uint32_t
#define FROM_LUA_NUM(x) ((uint32_t)(x-1.0))
#define READ_NUM(x) read_num_u32e(x)
#include "am_view_template.inc"

#define TNAME U8
#define CTYPE uint8_t
#define FROM_LUA_NUM(x) ((uint8_t)am_clamp((x), 0.0, (double)UINT8_MAX))
#define READ_NUM(x) read_num_u8(x)
#include "am_view_template.inc"

#define TNAME I8
#define CTYPE int8_t
#define FROM_LUA_NUM(x) ((int8_t)am_clamp((x), (double)INT8_MIN, (double)INT8_MAX))
#define READ_NUM(x) read_num_i8(x)
#include "am_view_template.inc"

#define TNAME U16
#define CTYPE uint16_t
#define FROM_LUA_NUM(x) ((uint16_t)am_clamp((x), 0.0, (double)UINT16_MAX))
#define READ_NUM(x) read_num_u16(x)
#include "am_view_template.inc"

#define TNAME I16
#define CTYPE int16_t
#define FROM_LUA_NUM(x) ((int16_t)am_clamp((x), (double)INT16_MIN, (double)INT16_MAX))
#define READ_NUM(x) read_num_i16(x)
#include "am_view_template.inc"

#define TNAME U8N
#define CTYPE uint8_t
#define FROM_LUA_NUM(x) ((uint8_t)floor((x) * (double)UINT8_MAX + 0.5))
#define READ_NUM(x) read_num_u8n(x)
#include "am_view_template.inc"

#define TNAME I8N
#define CTYPE int8_t
#define FROM_LUA_NUM(x) ((int8_t)((x) * (double)INT8_MAX))
#define READ_NUM(x) read_num_i8n(x)
#include "am_view_template.inc"

#define TNAME U16N
#define CTYPE uint16_t
#define FROM_LUA_NUM(x) ((uint16_t)((x) * (double)UINT16_MAX))
#define READ_NUM(x) read_num_u16n(x)
#include "am_view_template.inc"

#define TNAME I16N
#define CTYPE int16_t
#define FROM_LUA_NUM(x) ((int16_t)((x) * (double)INT16_MAX))
#define READ_NUM(x) read_num_i16n(x)
#include "am_view_template.inc"

static void register_view_mt(lua_State *L) {
    lua_newtable(L);

    am_register_mathv_view_methods(L);

    lua_pushcclosure(L, view_len, 0);
    lua_setfield(L, -2, "__len");

    am_register_property(L, "buffer", &view_buffer_property);

    lua_pushcclosure(L, view_slice, 0);
    lua_setfield(L, -2, "slice");

    am_register_metatable(L, "view", MT_am_buffer_view, 0);
}

void am_open_view_module(lua_State *L) {
    am_enum_value view_type_enum[] = {
        {"float",           AM_VIEW_TYPE_F32},
        {"double",          AM_VIEW_TYPE_F64},
        {"ubyte",           AM_VIEW_TYPE_U8},
        {"byte",            AM_VIEW_TYPE_I8},
        {"ubyte_norm",      AM_VIEW_TYPE_U8N},
        {"byte_norm",       AM_VIEW_TYPE_I8N},
        {"ushort",          AM_VIEW_TYPE_U16},
        {"short",           AM_VIEW_TYPE_I16},
        {"ushort_elem",     AM_VIEW_TYPE_U16E},
        {"ushort_norm",     AM_VIEW_TYPE_U16N},
        {"short_norm",      AM_VIEW_TYPE_I16N},
        {"uint",            AM_VIEW_TYPE_U32},
        {"int",             AM_VIEW_TYPE_I32},
        {"uint_elem",       AM_VIEW_TYPE_U32E},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_buffer_view_type, view_type_enum);

    am_enum_value view_type_lua_enum[] = {
        {"float",           AM_VIEW_TYPE_LUA_F32_1},
        {"vec2",            AM_VIEW_TYPE_LUA_F32_2},
        {"vec3",            AM_VIEW_TYPE_LUA_F32_3},
        {"vec4",            AM_VIEW_TYPE_LUA_F32_4},
        {"mat3",            AM_VIEW_TYPE_LUA_F32_3x3},
        {"mat4",            AM_VIEW_TYPE_LUA_F32_4x4},
        {"double",          AM_VIEW_TYPE_LUA_F64_1},
        {"dvec2",           AM_VIEW_TYPE_LUA_F64_2},
        {"dvec3",           AM_VIEW_TYPE_LUA_F64_3},
        {"dvec4",           AM_VIEW_TYPE_LUA_F64_4},
        {"dmat3",           AM_VIEW_TYPE_LUA_F64_3x3},
        {"dmat4",           AM_VIEW_TYPE_LUA_F64_4x4},
        {"ubyte",           AM_VIEW_TYPE_LUA_U8_1},
        {"ubyte2",          AM_VIEW_TYPE_LUA_U8_2},
        {"ubyte3",          AM_VIEW_TYPE_LUA_U8_3},
        {"ubyte4",          AM_VIEW_TYPE_LUA_U8_4},
        {"byte",            AM_VIEW_TYPE_LUA_I8_1},
        {"byte2",           AM_VIEW_TYPE_LUA_I8_2},
        {"byte3",           AM_VIEW_TYPE_LUA_I8_3},
        {"byte4",           AM_VIEW_TYPE_LUA_I8_4},
        {"ubyte_norm",      AM_VIEW_TYPE_LUA_U8N_1},
        {"ubyte_norm2",     AM_VIEW_TYPE_LUA_U8N_2},
        {"ubyte_norm3",     AM_VIEW_TYPE_LUA_U8N_3},
        {"ubyte_norm4",     AM_VIEW_TYPE_LUA_U8N_4},
        {"byte_norm",       AM_VIEW_TYPE_LUA_I8N_1},
        {"byte_norm2",      AM_VIEW_TYPE_LUA_I8N_2},
        {"byte_norm3",      AM_VIEW_TYPE_LUA_I8N_3},
        {"byte_norm4",      AM_VIEW_TYPE_LUA_I8N_4},
        {"ushort",          AM_VIEW_TYPE_LUA_U16_1},
        {"ushort2",         AM_VIEW_TYPE_LUA_U16_2},
        {"ushort3",         AM_VIEW_TYPE_LUA_U16_3},
        {"ushort4",         AM_VIEW_TYPE_LUA_U16_4},
        {"short",           AM_VIEW_TYPE_LUA_I16_1},
        {"short2",          AM_VIEW_TYPE_LUA_I16_2},
        {"short3",          AM_VIEW_TYPE_LUA_I16_3},
        {"short4",          AM_VIEW_TYPE_LUA_I16_4},
        {"ushort_elem",     AM_VIEW_TYPE_LUA_U16E},
        {"ushort_norm",     AM_VIEW_TYPE_LUA_U16N_1},
        {"ushort_norm2",    AM_VIEW_TYPE_LUA_U16N_2},
        {"ushort_norm3",    AM_VIEW_TYPE_LUA_U16N_3},
        {"ushort_norm4",    AM_VIEW_TYPE_LUA_U16N_4},
        {"short_norm",      AM_VIEW_TYPE_LUA_I16N_1},
        {"short_norm2",     AM_VIEW_TYPE_LUA_I16N_2},
        {"short_norm3",     AM_VIEW_TYPE_LUA_I16N_3},
        {"short_norm4",     AM_VIEW_TYPE_LUA_I16N_4},
        {"uint",            AM_VIEW_TYPE_LUA_U32_1},
        {"uint2",           AM_VIEW_TYPE_LUA_U32_2},
        {"uint3",           AM_VIEW_TYPE_LUA_U32_3},
        {"uint4",           AM_VIEW_TYPE_LUA_U32_4},
        {"int",             AM_VIEW_TYPE_LUA_I32_1},
        {"int2",            AM_VIEW_TYPE_LUA_I32_2},
        {"int3",            AM_VIEW_TYPE_LUA_I32_3},
        {"int4",            AM_VIEW_TYPE_LUA_I32_4},
        {"uint_elem",       AM_VIEW_TYPE_LUA_U32E},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_buffer_view_type_lua, view_type_lua_enum);

    register_view_mt(L);
    register_F32_view_mt(L);
    register_F64_view_mt(L);
    register_U8_view_mt(L);
    register_I8_view_mt(L);
    register_U8N_view_mt(L);
    register_I8N_view_mt(L);
    register_U16_view_mt(L);
    register_I16_view_mt(L);
    register_U16E_view_mt(L);
    register_U16N_view_mt(L);
    register_I16N_view_mt(L);
    register_U32_view_mt(L);
    register_I32_view_mt(L);
    register_U32E_view_mt(L);
}
