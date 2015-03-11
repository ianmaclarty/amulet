#include "amulet.h"

am_buffer::am_buffer() {
    origin = "unnamed buffer";
    origin_ref = LUA_NOREF;
    arraybuf_id = 0;
    elembuf_id = 0;
    texture2d = NULL;
    texture2d_ref = LUA_NOREF;
    dirty_start = INT_MAX;
    dirty_end = 0;
    track_dirty = false;
    version = 1;
}

am_buffer::am_buffer(int sz) {
    size = sz;
    data = (uint8_t*)malloc(sz);
    memset(data, 0, size);
    origin = "unnamed buffer";
    origin_ref = LUA_NOREF;
    arraybuf_id = 0;
    elembuf_id = 0;
    texture2d = NULL;
    texture2d_ref = LUA_NOREF;
    dirty_start = INT_MAX;
    dirty_end = 0;
    track_dirty = false;
    version = 1;
}

void am_buffer::destroy() {
    free(data);
    if (arraybuf_id != 0) {
        am_delete_buffer(arraybuf_id);
        arraybuf_id = 0;
    }
    if (elembuf_id != 0) {
        am_delete_buffer(elembuf_id);
        elembuf_id = 0;
    }
}

void am_buffer::update_if_dirty() {
    if (dirty_start < dirty_end) {
        if (arraybuf_id != 0) {
            am_bind_buffer(AM_ARRAY_BUFFER, arraybuf_id);
            am_set_buffer_sub_data(AM_ARRAY_BUFFER, dirty_start, dirty_end - dirty_start, data + dirty_start);
        } 
        if (elembuf_id != 0) {
            am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, elembuf_id);
            am_set_buffer_sub_data(AM_ELEMENT_ARRAY_BUFFER, dirty_start, dirty_end - dirty_start, data + dirty_start);
        } 
        if (texture2d != NULL) {
            texture2d->update_from_buffer();
        }
        dirty_start = INT_MAX;
        dirty_end = 0;
        version++;
    }
}

void am_buffer::create_arraybuf() {
    assert(arraybuf_id == 0);
    update_if_dirty();
    arraybuf_id = am_create_buffer_object();
    am_bind_buffer(AM_ARRAY_BUFFER, arraybuf_id);
    am_set_buffer_data(AM_ARRAY_BUFFER, size, &data[0], AM_BUFFER_USAGE_STATIC_DRAW);
    track_dirty = true;
}

void am_buffer::create_elembuf() {
    assert(elembuf_id == 0);
    update_if_dirty();
    elembuf_id = am_create_buffer_object();
    am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, elembuf_id);
    am_set_buffer_data(AM_ELEMENT_ARRAY_BUFFER, size, &data[0], AM_BUFFER_USAGE_STATIC_DRAW);
    track_dirty = true;
}

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

static int create_buffer(lua_State *L) {
    am_check_nargs(L, 1);
    int size = luaL_checkinteger(L, 1);
    if (size <= 0) return luaL_error(L, "size should be greater than 0");
    am_new_userdata(L, am_buffer, size);
    return 1;
}

static int read_buffer(lua_State *L) {
    am_check_nargs(L, 1);
    const char *filename = luaL_checkstring(L, 1);
    int len;
    char *errmsg;
    void *data = am_read_resource(filename, &len, false, &errmsg);
    if (data == NULL) {
        if (errmsg != NULL) {
            lua_pushfstring(L, "error reading buffer '%s': %s", filename, errmsg);
            free(errmsg);
            return lua_error(L);
        } else {
            return luaL_error(L, "unknown error reading buffer '%s'", filename);
        }
    }
    am_buffer *buf = am_new_userdata(L, am_buffer);
    buf->data = (uint8_t*)data;
    buf->size = len;
    buf->origin = filename;
    buf->origin_ref = buf->ref(L, 1);
    return 1;
}

static int buffer_len(lua_State *L) {
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    lua_pushinteger(L, buf->size);
    return 1;
}

static int buffer_gc(lua_State *L) {
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    buf->destroy();
    return 0;
}

static am_buffer_view* new_buffer_view(lua_State *L, int metatable_id) {
    return (am_buffer_view*)
        am_init_userdata(L,
            new (lua_newuserdata(L, sizeof(am_buffer_view))) am_buffer_view(),
            metatable_id);
}

static int create_buffer_view(lua_State *L) {
    int nargs = am_check_nargs(L, 4);

    am_buffer *buf = am_get_userdata(L, am_buffer, 1);

    am_buffer_view_type type = am_get_enum(L, am_buffer_view_type, 2);

    int offset = luaL_checkinteger(L, 3);
    int stride = luaL_checkinteger(L, 4);

    int type_size = 0;
    bool normalized = false;
    switch (type) {
        case AM_VIEW_TYPE_FLOAT:
            type_size = 4;
            break;
        case AM_VIEW_TYPE_FLOAT2:
            type_size = 8;
            break;
        case AM_VIEW_TYPE_FLOAT3:
            type_size = 12;
            break;
        case AM_VIEW_TYPE_FLOAT4:
            type_size = 16;
            break;
        case AM_VIEW_TYPE_UBYTE:
        case AM_VIEW_TYPE_BYTE:
            type_size = 1;
            break;
        case AM_VIEW_TYPE_UBYTE_NORM:
        case AM_VIEW_TYPE_BYTE_NORM:
            type_size = 1;
            normalized = true;
            break;
        case AM_VIEW_TYPE_USHORT:
        case AM_VIEW_TYPE_SHORT:
        case AM_VIEW_TYPE_USHORT_ELEM:
            type_size = 2;
            break;
        case AM_VIEW_TYPE_USHORT_NORM:
        case AM_VIEW_TYPE_SHORT_NORM:
            type_size = 2;
            normalized = true;
            break;
        case AM_VIEW_TYPE_UINT:
        case AM_VIEW_TYPE_INT:
        case AM_VIEW_TYPE_UINT_ELEM:
            type_size = 4;
            break;
        case AM_NUM_VIEW_TYPES:
            assert(false);
            break;
    }
    assert(type_size > 0);

    int available_bytes = buf->size - offset - type_size;
    int max_size = 0;
    if (available_bytes > 0) {
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

    int mtid = (int)MT_am_buffer_view + (int)type + 1;
    assert(mtid < (int)MT_VIEW_TYPE_END_MARKER);
    am_buffer_view *view = new_buffer_view(L, mtid);

    view->buffer = buf;
    view->buffer_ref = view->ref(L, 1);
    view->offset = offset;
    view->stride = stride;
    view->size = size;
    view->type = type;
    view->type_size = type_size;
    view->normalized = normalized;
    view->last_max_elem_version = 0;
    view->max_elem = 0;

    return 1;
}

static int view_slice(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 1);
    int start = luaL_checkinteger(L, 2);
    if (start < 1 || start > view->size) {
        return luaL_error(L, "slice start must be in the range [1, %d] (in fact %d)",
            view->size, start);
    }
    int size;
    if (nargs > 2) {
        size = luaL_checkinteger(L, 2);
        if (size < 0 || size > (view->size - start + 1)) {
            return luaL_error(L, "slice size must be in the range [0, %d] (in fact %d)",
                view->size - start + 1, size);
        }
    } else {
        size = view->size - start + 1;
    }
    am_buffer_view *slice = new_buffer_view(L, view->metatable_id);
    slice->buffer = view->buffer;
    view->pushref(L, view->buffer_ref);
    slice->buffer_ref = slice->ref(L, -1);
    lua_pop(L, 1); // pop buffer
    slice->offset = view->offset + (start-1) * view->stride;
    slice->stride = view->stride;
    slice->size = size;
    slice->type = view->type;
    slice->type_size = view->type_size;
    slice->normalized = view->normalized;
    slice->max_elem = view->max_elem;
    slice->last_max_elem_version = view->last_max_elem_version;
    return 1;
}

static void register_buffer_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, buffer_len, 0);
    lua_setfield(L, -2, "__len");
    lua_pushcclosure(L, buffer_gc, 0);
    lua_setfield(L, -2, "__gc");

    lua_pushstring(L, "buffer");
    lua_setfield(L, -2, "tname");

    lua_pushcclosure(L, create_buffer_view, 0);
    lua_setfield(L, -2, "view");

    am_register_metatable(L, MT_am_buffer, 0);
}

static void register_view_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L, view_slice, 0);
    lua_setfield(L, -2, "slice");

    lua_pushstring(L, "view");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_buffer_view, 0);
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
    return ((lua_Number)(*((int8_t*)ptr))) / 127.0;
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
    return ((lua_Number)(*((int16_t*)ptr))) / 32767.0;
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

static lua_Number(*view_number_reader[])(uint8_t*) = {
    &read_num_float,
    NULL,
    NULL,
    NULL,
    &read_num_ubyte,
    &read_num_byte,
    &read_num_ubyte_norm,
    &read_num_byte_norm,
    &read_num_ushort,
    &read_num_short,
    &read_num_ushort_elem,
    &read_num_ushort_norm,
    &read_num_short_norm,
    &read_num_uint,
    &read_num_int,
    &read_num_uint_elem,
};
ct_check_array_size(view_number_reader, AM_NUM_VIEW_TYPES);

static const char *view_type_name[] = {
    "float",
    "float2",
    "float3",
    "float4",
    "ubyte",
    "byte",
    "ubyte_norm",
    "byte_norm",
    "ushort",
    "short",
    "ushort_elem",
    "ushort_norm",
    "short_norm",
    "uint",
    "int",
    "uint_elem",
};
ct_check_array_size(view_type_name, AM_NUM_VIEW_TYPES);

#define TNAME float
#define CTYPE float
#define LUA_TYPE LUA_TNUMBER
#define GET_CTYPE(L, idx) ((float)(lua_tonumber(L, idx)))
#define PUSH_CTYPE(L, x) lua_pushnumber(L, x)
#include "am_view_template.inc"

#define TNAME float2
#define CTYPE glm::vec2
#define LUA_TYPE MT_am_vec2
#define GET_CTYPE(L, idx) (am_get_userdata(L, am_vec2, idx)->v)
#define PUSH_CTYPE(L, x) am_new_userdata(L, am_vec2)->v = x
#define VEC_SZ 2
#define GET_VEC_COMPONENT(L, idx) ((float)(lua_tonumber(L, idx)))
#include "am_view_template.inc"

#define TNAME float3
#define CTYPE glm::vec3
#define LUA_TYPE MT_am_vec3
#define GET_CTYPE(L, idx) (am_get_userdata(L, am_vec3, idx)->v)
#define PUSH_CTYPE(L, x) am_new_userdata(L, am_vec3)->v = x
#define VEC_SZ 3
#define GET_VEC_COMPONENT(L, idx) ((float)(lua_tonumber(L, idx)))
#include "am_view_template.inc"

#define TNAME float4
#define CTYPE glm::vec4
#define LUA_TYPE MT_am_vec4
#define GET_CTYPE(L, idx) (am_get_userdata(L, am_vec4, idx)->v)
#define PUSH_CTYPE(L, x) am_new_userdata(L, am_vec4)->v = x
#define VEC_SZ 4
#define GET_VEC_COMPONENT(L, idx) ((float)(lua_tonumber(L, idx)))
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

void am_open_buffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"buffer", create_buffer},
        {"read_buffer", read_buffer},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);

    am_enum_value view_type_enum[] = {
        {"float",           AM_VIEW_TYPE_FLOAT},
        {"vec2",            AM_VIEW_TYPE_FLOAT2},
        {"vec3",            AM_VIEW_TYPE_FLOAT3},
        {"vec4",            AM_VIEW_TYPE_FLOAT4},
        {"ubyte",           AM_VIEW_TYPE_UBYTE},
        {"byte",            AM_VIEW_TYPE_BYTE},
        {"ubyte_norm",      AM_VIEW_TYPE_UBYTE_NORM},
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

    register_buffer_mt(L);
    register_view_mt(L);
    register_float_view_mt(L);
    register_float2_view_mt(L);
    register_float3_view_mt(L);
    register_float4_view_mt(L);
    register_ubyte_view_mt(L);
    register_byte_view_mt(L);
    register_ubyte_norm_view_mt(L);
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
