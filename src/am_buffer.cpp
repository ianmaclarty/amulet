#include "amulet.h"

static int create_buffer(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    int isnum;
    int size = lua_tointegerx(L, 1, &isnum);
    if (!isnum) return luaL_error(L, "expecting size (an integer) at position 1");
    am_buffer *buf = (am_buffer*)lua_newuserdata(L, sizeof(am_buffer) + size);
    buf->size = size;
    if (nargs == 1 || (nargs > 1 && lua_toboolean(L, 2))) {
        memset(&buf->data[0], 0, size);
    }
    am_set_metatable(L, AM_MT_BUFFER, -1);
    return 1;
}

#define create_buffer_view_func(TYPE, type_size)                                                                \
static int create_buffer_view_##TYPE(lua_State *L) {                                                            \
    int nargs = am_check_nargs(L, 3);                                                                           \
                                                                                                                \
    am_buffer *buf = (am_buffer*)am_check_metatable_id(L, AM_MT_BUFFER, 1);                                     \
                                                                                                                \
    int offset = luaL_checkinteger(L, 2);                                                                       \
    int stride = luaL_checkinteger(L, 3);                                                                       \
    bool normalized = false;                                                                                    \
    if (nargs > 3) {                                                                                            \
        normalized = lua_toboolean(L, 4);                                                                       \
    }                                                                                                           \
                                                                                                                \
    int size = INT_MAX;                                                                                         \
    if (nargs > 4) size = luaL_checkinteger(L, 5);                                                              \
    int available_bytes = buf->size - offset - type_size;                                                       \
    int max_size = 0;                                                                                           \
    if (available_bytes > 0) {                                                                                  \
        max_size = available_bytes / stride + 1;                                                                \
    }                                                                                                           \
    if (size > max_size) {                                                                                      \
        size = max_size;                                                                                        \
    }                                                                                                           \
                                                                                                                \
    am_buffer_view *view = new (am_new_nonatomic_userdata(L, sizeof(am_buffer_view))) am_buffer_view();         \
    view->buffer = buf;                                                                                         \
    am_new_ref(L, -1, 1);                                                                                       \
    view->offset = offset;                                                                                      \
    view->stride = stride;                                                                                      \
    view->size = size;                                                                                          \
    view->type = AM_BUF_ELEM_TYPE_##TYPE;                                                                       \
    view->normalized = normalized;                                                                              \
                                                                                                                \
    am_set_metatable(L, AM_MT_BUFFER_VIEW_##TYPE, -1);                                                          \
                                                                                                                \
    return 1;                                                                                                   \
}

create_buffer_view_func(FLOAT, 4)
create_buffer_view_func(FLOAT_VEC2, 8)
create_buffer_view_func(FLOAT_VEC3, 12)
create_buffer_view_func(FLOAT_VEC4, 16)
create_buffer_view_func(BYTE, 1)
create_buffer_view_func(BYTE_VEC2, 2)
create_buffer_view_func(BYTE_VEC3, 3)
create_buffer_view_func(BYTE_VEC4, 4)
create_buffer_view_func(UNSIGNED_BYTE, 1)
create_buffer_view_func(UNSIGNED_BYTE_VEC2, 2)
create_buffer_view_func(UNSIGNED_BYTE_VEC3, 3)
create_buffer_view_func(UNSIGNED_BYTE_VEC4, 4)
create_buffer_view_func(SHORT, 2)
create_buffer_view_func(SHORT_VEC2, 4)
create_buffer_view_func(SHORT_VEC3, 6)
create_buffer_view_func(SHORT_VEC4, 8)
create_buffer_view_func(UNSIGNED_SHORT, 2)
create_buffer_view_func(UNSIGNED_SHORT_VEC2, 4)
create_buffer_view_func(UNSIGNED_SHORT_VEC3, 6)
create_buffer_view_func(UNSIGNED_SHORT_VEC4, 8)

#define buffer_view_index_func(TYPE)                    \
static int buffer_view_index_##TYPE(lua_State *L) {     \
    return luaL_error(L, "NYI");                        \
}

static int buffer_view_index_FLOAT(lua_State *L) {
    am_buffer_view *view = (am_buffer_view*)lua_touserdata(L, 1);
    int index = lua_tointeger(L, 2);
    if (index < 1 || index > view->size) {
        lua_pushnil(L);
        return 1;
    }
    float val = *((float*)&view->buffer->data[view->offset + view->stride * (index-1)]);
    lua_pushnumber(L, val);
    return 1;
}

buffer_view_index_func(FLOAT_VEC2)
buffer_view_index_func(FLOAT_VEC3)
buffer_view_index_func(FLOAT_VEC4)
buffer_view_index_func(BYTE)
buffer_view_index_func(BYTE_VEC2)
buffer_view_index_func(BYTE_VEC3)
buffer_view_index_func(BYTE_VEC4)
buffer_view_index_func(UNSIGNED_BYTE)
buffer_view_index_func(UNSIGNED_BYTE_VEC2)
buffer_view_index_func(UNSIGNED_BYTE_VEC3)
buffer_view_index_func(UNSIGNED_BYTE_VEC4)
buffer_view_index_func(SHORT)
buffer_view_index_func(SHORT_VEC2)
buffer_view_index_func(SHORT_VEC3)
buffer_view_index_func(SHORT_VEC4)
buffer_view_index_func(UNSIGNED_SHORT)
buffer_view_index_func(UNSIGNED_SHORT_VEC2)
buffer_view_index_func(UNSIGNED_SHORT_VEC3)
buffer_view_index_func(UNSIGNED_SHORT_VEC4)

#define buffer_view_newindex_func(TYPE)                 \
static int buffer_view_newindex_##TYPE(lua_State *L) {  \
    return luaL_error(L, "NYI");                        \
}

static int buffer_view_newindex_FLOAT(lua_State *L) {
    am_buffer_view *view = (am_buffer_view*)lua_touserdata(L, 1);
    int index = lua_tointeger(L, 2);
    if (index < 1 || index > view->size) {
        return luaL_error(L, "view index %d out of range", index);
    }
    float val = luaL_checknumber(L, 3);
    *((float*)&view->buffer->data[view->offset + view->stride * (index-1)]) = val;
    return 0;
}

buffer_view_newindex_func(FLOAT_VEC2)
buffer_view_newindex_func(FLOAT_VEC3)
buffer_view_newindex_func(FLOAT_VEC4)
buffer_view_newindex_func(BYTE)
buffer_view_newindex_func(BYTE_VEC2)
buffer_view_newindex_func(BYTE_VEC3)
buffer_view_newindex_func(BYTE_VEC4)
buffer_view_newindex_func(UNSIGNED_BYTE)
buffer_view_newindex_func(UNSIGNED_BYTE_VEC2)
buffer_view_newindex_func(UNSIGNED_BYTE_VEC3)
buffer_view_newindex_func(UNSIGNED_BYTE_VEC4)
buffer_view_newindex_func(SHORT)
buffer_view_newindex_func(SHORT_VEC2)
buffer_view_newindex_func(SHORT_VEC3)
buffer_view_newindex_func(SHORT_VEC4)
buffer_view_newindex_func(UNSIGNED_SHORT)
buffer_view_newindex_func(UNSIGNED_SHORT_VEC2)
buffer_view_newindex_func(UNSIGNED_SHORT_VEC3)
buffer_view_newindex_func(UNSIGNED_SHORT_VEC4)

#define register_buffer_view_mt_func(TYPE, suffix)                  \
static void register_buffer_view_mt_##TYPE(lua_State *L) {          \
    lua_newtable(L);                                                \
    lua_pushcclosure(L, buffer_view_index_##TYPE, 0);               \
    lua_setfield(L, -2, "__index");                                 \
    lua_pushcclosure(L, buffer_view_newindex_##TYPE, 0);            \
    lua_setfield(L, -2, "__newindex");                              \
    lua_pushstring(L, "view_" #suffix);                             \
    lua_setfield(L, -2, "tname");                                   \
    am_register_metatable(L, AM_MT_BUFFER_VIEW_##TYPE);             \
}

register_buffer_view_mt_func(FLOAT, float)
register_buffer_view_mt_func(FLOAT_VEC2, float2)
register_buffer_view_mt_func(FLOAT_VEC3, float3)
register_buffer_view_mt_func(FLOAT_VEC4, float4)
register_buffer_view_mt_func(BYTE, byte)
register_buffer_view_mt_func(BYTE_VEC2, byte2)
register_buffer_view_mt_func(BYTE_VEC3, byte3)
register_buffer_view_mt_func(BYTE_VEC4, byte4)
register_buffer_view_mt_func(UNSIGNED_BYTE, ubyte)
register_buffer_view_mt_func(UNSIGNED_BYTE_VEC2, ubyte2)
register_buffer_view_mt_func(UNSIGNED_BYTE_VEC3, ubyte3)
register_buffer_view_mt_func(UNSIGNED_BYTE_VEC4, ubyte4)
register_buffer_view_mt_func(SHORT, short)
register_buffer_view_mt_func(SHORT_VEC2, short2)
register_buffer_view_mt_func(SHORT_VEC3, short3)
register_buffer_view_mt_func(SHORT_VEC4, short4)
register_buffer_view_mt_func(UNSIGNED_SHORT, ushort)
register_buffer_view_mt_func(UNSIGNED_SHORT_VEC2, ushort2)
register_buffer_view_mt_func(UNSIGNED_SHORT_VEC3, ushort3)
register_buffer_view_mt_func(UNSIGNED_SHORT_VEC4, ushort4)

static void register_buffer_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushstring(L, "buffer");
    lua_setfield(L, -2, "tname");
    lua_pushcclosure(L, create_buffer_view_FLOAT, 0);
    lua_setfield(L, -2, "float_view");
    lua_pushcclosure(L, create_buffer_view_FLOAT_VEC2, 0);
    lua_setfield(L, -2, "float2_view");
    lua_pushcclosure(L, create_buffer_view_FLOAT_VEC3, 0);
    lua_setfield(L, -2, "float3_view");
    lua_pushcclosure(L, create_buffer_view_FLOAT_VEC4, 0);
    lua_setfield(L, -2, "float4_view");
    lua_pushcclosure(L, create_buffer_view_BYTE, 0);
    lua_setfield(L, -2, "byte_view");
    lua_pushcclosure(L, create_buffer_view_BYTE_VEC2, 0);
    lua_setfield(L, -2, "byte2_view");
    lua_pushcclosure(L, create_buffer_view_BYTE_VEC3, 0);
    lua_setfield(L, -2, "byte3_view");
    lua_pushcclosure(L, create_buffer_view_BYTE_VEC4, 0);
    lua_setfield(L, -2, "byte4_view");
    lua_pushcclosure(L, create_buffer_view_UNSIGNED_BYTE, 0);
    lua_setfield(L, -2, "ubyte_view");
    lua_pushcclosure(L, create_buffer_view_UNSIGNED_BYTE_VEC2, 0);
    lua_setfield(L, -2, "ubyte2_view");
    lua_pushcclosure(L, create_buffer_view_UNSIGNED_BYTE_VEC3, 0);
    lua_setfield(L, -2, "ubyte3_view");
    lua_pushcclosure(L, create_buffer_view_UNSIGNED_BYTE_VEC4, 0);
    lua_setfield(L, -2, "ubyte4_view");
    lua_pushcclosure(L, create_buffer_view_SHORT, 0);
    lua_setfield(L, -2, "short_view");
    lua_pushcclosure(L, create_buffer_view_SHORT_VEC2, 0);
    lua_setfield(L, -2, "short2_view");
    lua_pushcclosure(L, create_buffer_view_SHORT_VEC3, 0);
    lua_setfield(L, -2, "short3_view");
    lua_pushcclosure(L, create_buffer_view_SHORT_VEC4, 0);
    lua_setfield(L, -2, "short4_view");
    lua_pushcclosure(L, create_buffer_view_UNSIGNED_SHORT, 0);
    lua_setfield(L, -2, "ushort_view");
    lua_pushcclosure(L, create_buffer_view_UNSIGNED_SHORT_VEC2, 0);
    lua_setfield(L, -2, "ushort2_view");
    lua_pushcclosure(L, create_buffer_view_UNSIGNED_SHORT_VEC3, 0);
    lua_setfield(L, -2, "ushort3_view");
    lua_pushcclosure(L, create_buffer_view_UNSIGNED_SHORT_VEC4, 0);
    lua_setfield(L, -2, "ushort4_view");
    am_register_metatable(L, AM_MT_BUFFER);
}

void am_open_buffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"buffer", create_buffer},
        {NULL, NULL}
    };
    am_open_module(L, "amulet", funcs);
    register_buffer_mt(L);
    register_buffer_view_mt_FLOAT(L);
    register_buffer_view_mt_FLOAT_VEC2(L);
    register_buffer_view_mt_FLOAT_VEC3(L);
    register_buffer_view_mt_FLOAT_VEC4(L);
    register_buffer_view_mt_BYTE(L);
    register_buffer_view_mt_BYTE_VEC2(L);
    register_buffer_view_mt_BYTE_VEC3(L);
    register_buffer_view_mt_BYTE_VEC4(L);
    register_buffer_view_mt_UNSIGNED_BYTE(L);
    register_buffer_view_mt_UNSIGNED_BYTE_VEC2(L);
    register_buffer_view_mt_UNSIGNED_BYTE_VEC3(L);
    register_buffer_view_mt_UNSIGNED_BYTE_VEC4(L);
    register_buffer_view_mt_SHORT(L);
    register_buffer_view_mt_SHORT_VEC2(L);
    register_buffer_view_mt_SHORT_VEC3(L);
    register_buffer_view_mt_SHORT_VEC4(L);
    register_buffer_view_mt_UNSIGNED_SHORT(L);
    register_buffer_view_mt_UNSIGNED_SHORT_VEC2(L);
    register_buffer_view_mt_UNSIGNED_SHORT_VEC3(L);
    register_buffer_view_mt_UNSIGNED_SHORT_VEC4(L);
}
