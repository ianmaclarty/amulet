#include "amulet.h"

am_buffer::am_buffer() {
    size = 0;
    data = NULL;
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
        am_bind_buffer(AM_ARRAY_BUFFER, 0);
        am_delete_buffer(arraybuf_id);
        arraybuf_id = 0;
    }
    if (elembuf_id != 0) {
        am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, 0);
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
            texture2d->update_from_image_buffer();
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

static int load_buffer(lua_State *L) {
    am_check_nargs(L, 1);
    const char *filename = luaL_checkstring(L, 1);
    int len;
    char *errmsg;
    void *data = am_read_resource(filename, &len, &errmsg);
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

static int load_string(lua_State *L) {
    am_check_nargs(L, 1);
    const char *filename = luaL_checkstring(L, 1);
    int len;
    char *errmsg;
    void *data = am_read_resource(filename, &len, &errmsg);
    if (data == NULL) {
        if (errmsg != NULL) {
            lua_pushfstring(L, "error reading file '%s': %s", filename, errmsg);
            free(errmsg);
            return lua_error(L);
        } else {
            return luaL_error(L, "unknown error reading file '%s'", filename);
        }
    }
    lua_pushlstring(L, (const char*)data, len);
    free(data);
    return 1;
}

static int load_script(lua_State *L) {
    am_check_nargs(L, 1);
    const char *filename = luaL_checkstring(L, 1);
    int len;
    char *errmsg;
    void *data = am_read_resource(filename, &len, &errmsg);
    if (data == NULL) {
        if (errmsg != NULL) {
            lua_pushfstring(L, "error reading file '%s': %s", filename, errmsg);
            free(errmsg);
            return lua_error(L);
        } else {
            return luaL_error(L, "unknown error reading file '%s'", filename);
        }
    }
    luaL_loadbuffer(L, (const char*)data, len, filename);
    free(data);
    return 1;
}

static const char *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_encode(lua_State *L) {
    am_check_nargs(L, 1);
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    uint8_t *data = buf->data;
    int buf_sz = buf->size;
    int b64_sz = (buf_sz + 2) / 3 * 4;
    char *b64_str = (char*)malloc(b64_sz);
    int i = 0;
    int j = 0;
    int extra = 0;
    int b1, b2, b3;
    while (i < buf_sz) {
        b1 = data[i++];
        if (i < buf_sz) {
            b2 = data[i++];
        } else {
            b2 = 0;
            extra++;
        }
        if (i < buf_sz) {
            b3 = data[i++];
        } else {
            b3 = 0;
            extra++;
        }
        int triple = b1 << 16 | b2 << 8 | b3;
        b64_str[j++] = base64[triple >> 18];
        b64_str[j++] = base64[triple >> 12 & 63];
        b64_str[j++] = base64[triple >> 6 & 63];
        b64_str[j++] = base64[triple & 63];
    }
    assert(extra <= 2);
    switch (extra) {
        case 2:
            b64_str[b64_sz-2] = '=';
        case 1:
            b64_str[b64_sz-1] = '=';
    }
    lua_pushlstring(L, b64_str, b64_sz);
    free(b64_str);
    return 1;
}

static int base64_decode(lua_State *L) {
    am_check_nargs(L, 1);
    size_t b64_sz;
    const char *b64_str = (const char*)lua_tolstring(L, 1, &b64_sz);
    if (b64_str == NULL) return luaL_error(L, "expecting a string in position 1");
    if (b64_sz % 4 != 0) return luaL_error(L, "string length should be divisble by 4");
    int buf_sz = b64_sz / 4 * 3;
    if (buf_sz == 0) {
        am_new_userdata(L, am_buffer, 0);
        return 1;
    }
    if (b64_str[b64_sz-1] == '=') buf_sz--;
    if (b64_str[b64_sz-2] == '=') buf_sz--;
    am_buffer *buf = am_new_userdata(L, am_buffer, buf_sz);
    uint8_t *data = buf->data;
    int i = 0;
    int j = 0;
    while (j < (int)b64_sz) {
        int triple = 0;
        for (int k = 0; k < 4; k++) {
            triple <<= 6;
            int c = b64_str[j++];
            switch (c) {
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                case 'F':
                case 'G':
                case 'H':
                case 'I':
                case 'J':
                case 'K':
                case 'L':
                case 'M':
                case 'N':
                case 'O':
                case 'P':
                case 'Q':
                case 'R':
                case 'S':
                case 'T':
                case 'U':
                case 'V':
                case 'W':
                case 'X':
                case 'Y':
                case 'Z':
                    triple |= c - 'A';
                    break;
                case 'a':
                case 'b':
                case 'c':
                case 'd':
                case 'e':
                case 'f':
                case 'g':
                case 'h':
                case 'i':
                case 'j':
                case 'k':
                case 'l':
                case 'm':
                case 'n':
                case 'o':
                case 'p':
                case 'q':
                case 'r':
                case 's':
                case 't':
                case 'u':
                case 'v':
                case 'w':
                case 'x':
                case 'y':
                case 'z':
                    triple |= c - 'a' + 26;
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    triple |= c - '0' + 52;
                    break;
                case '+':
                    triple |= 62;
                    break;
                case '/':
                    triple |= 63;
                    break;
                case '=':
                    break;
                default:
                    return luaL_error(L, "unexpected character in base64 string: %c", (char)c);
            }
        }
        assert(i < buf_sz);
        data[i++] = triple >> 16;
        if (i < buf_sz) {
            data[i++] = triple >> 8 & 255;
        }
        if (i < buf_sz) {
            data[i++] = triple & 255;
        }
    }
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

static int release_vbo(lua_State *L) {
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    if (buf->arraybuf_id != 0) {
        am_delete_buffer(buf->arraybuf_id);
        buf->arraybuf_id = 0;
    }
    return 0;
}

static int mark_buffer_dirty(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    int start = 0;
    int end = buf->size;
    if (nargs > 1) {
        start = lua_tointeger(L, 2);
        if (start < 0 || start >= buf->size) {
            return luaL_error(L, "start must be in the range 0 to %d", buf->size - 1);
        }
    }
    if (nargs > 2) {
        end = lua_tointeger(L, 3);
        if (end <= 0 || end > buf->size) {
            return luaL_error(L, "end be in the range 1 to %d", buf->size);
        }
    }
    buf->mark_dirty(start, end);
    return 0;
}

am_buffer_view* am_new_buffer_view(lua_State *L, am_buffer_view_type type) {
    int mtid = (int)MT_am_buffer_view + (int)type + 1;
    assert(mtid < (int)MT_VIEW_TYPE_END_MARKER);
    // use the element-type specific metatable
    am_buffer_view *view = (am_buffer_view*)
        am_set_metatable(L,
            new (lua_newuserdata(L, sizeof(am_buffer_view))) am_buffer_view(),
            mtid);
    return view;
}

static int create_buffer_view(lua_State *L) {
    int nargs = am_check_nargs(L, 2);

    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    am_buffer_view_type type = am_get_enum(L, am_buffer_view_type, 2);

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
        size = luaL_checkinteger(L, 3);
        if (size < 0 || size > (view->size - start + 1)) {
            return luaL_error(L, "slice size must be in the range [0, %d] (in fact %d)",
                view->size - start + 1, size);
        }
    } else {
        size = view->size - start + 1;
    }
    am_buffer_view *slice = am_new_buffer_view(L, view->type);
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

static void get_buffer_dataptr(lua_State *L, void *obj) {
    am_buffer *buf = (am_buffer*)obj;
    lua_pushlightuserdata(L, (void*)buf->data);
}

static am_property buffer_dataptr_property = {get_buffer_dataptr, NULL};

static void register_buffer_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, buffer_len, 0);
    lua_setfield(L, -2, "__len");
    lua_pushcclosure(L, buffer_gc, 0);
    lua_setfield(L, -2, "__gc");

    am_register_property(L, "dataptr", &buffer_dataptr_property);

    lua_pushcclosure(L, create_buffer_view, 0);
    lua_setfield(L, -2, "view");
    lua_pushcclosure(L, release_vbo, 0);
    lua_setfield(L, -2, "release_vbo");
    lua_pushcclosure(L, mark_buffer_dirty, 0);
    lua_setfield(L, -2, "mark_dirty");

    am_register_metatable(L, "buffer", MT_am_buffer, 0);
}

static void get_view_buffer(lua_State *L, void *obj) {
    am_buffer_view *view = (am_buffer_view*)obj;
    view->pushref(L, view->buffer_ref);
}

static am_property view_buffer_property = {get_view_buffer, NULL};

static int view_len(lua_State *L) {
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 1);
    lua_pushinteger(L, view->size);
    return 1;
}

static void register_view_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, view_len, 0);
    lua_setfield(L, -2, "__len");

    am_register_property(L, "buffer", &view_buffer_property);

    lua_pushcclosure(L, view_slice, 0);
    lua_setfield(L, -2, "slice");

    am_register_metatable(L, "view", MT_am_buffer_view, 0);
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
    "vec2",
    "vec3",
    "vec4",
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
        {"load_buffer", load_buffer},
        {"load_string", load_string},
        {"load_script", load_script},
        {"base64_encode", base64_encode},
        {"base64_decode", base64_decode},
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
    register_vec2_view_mt(L);
    register_vec3_view_mt(L);
    register_vec4_view_mt(L);
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
