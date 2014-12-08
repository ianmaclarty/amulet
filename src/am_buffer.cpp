#include "amulet.h"

am_vbo_info::am_vbo_info() {
    buffer_id = 0;
    dirty_start = 0;
    dirty_end = 0;
}

am_buffer::am_buffer(int sz) {
    size = sz;
    data = (uint8_t*)malloc(sz);
    memset(data, 0, size);
}

void am_buffer::destroy() {
    free(data);
    if (vbo.buffer_id != 0) {
        am_delete_buffer(vbo.buffer_id);
    }
}

void am_buffer::create_vbo() {
    assert(vbo.buffer_id == 0);
    vbo.buffer_id = am_create_buffer();
    vbo.dirty_start = INT_MAX;
    vbo.dirty_end = 0;
    am_bind_buffer(AM_ARRAY_BUFFER, vbo.buffer_id);
    am_set_buffer_data(AM_ARRAY_BUFFER, size, &data[0], AM_BUFFER_USAGE_STATIC_DRAW);
}

static int create_buffer(lua_State *L) {
    am_check_nargs(L, 1);
    int isnum;
    int size = lua_tointegerx(L, 1, &isnum);
    if (!isnum) return luaL_error(L, "expecting size (an integer) at position 1");
    if (size <= 0) return luaL_error(L, "size should be greater than 0");
    am_new_userdata(L, am_buffer, size);
    return 1;
}

static int buffer_gc(lua_State *L) {
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    buf->destroy();
    return 0;
}

static int create_buffer_view(lua_State *L) {
    int nargs = am_check_nargs(L, 4);

    am_buffer *buf = am_get_userdata(L, am_buffer, 1);

    const char *typestr = luaL_checkstring(L, 2);
    am_buffer_view_type type;
    int type_size;
    if (strcmp(typestr, "float") == 0) {
        type = AM_BUF_ELEM_TYPE_FLOAT;
        type_size = sizeof(float);
    } else if (strcmp(typestr, "float2") == 0) {
        type = AM_BUF_ELEM_TYPE_FLOAT2;
        type_size = sizeof(float) * 2;
    } else if (strcmp(typestr, "float3") == 0) {
        type = AM_BUF_ELEM_TYPE_FLOAT3;
        type_size = sizeof(float) * 3;
    } else if (strcmp(typestr, "float4") == 0) {
        type = AM_BUF_ELEM_TYPE_FLOAT4;
        type_size = sizeof(float) * 4;
    } else if (strcmp(typestr, "ubyte") == 0) {
        type = AM_BUF_ELEM_TYPE_UBYTE;
        type_size = sizeof(uint8_t);
    } else {
        return luaL_error(L, "unknown buffer view type: '%s'", typestr);
    }

    int offset = luaL_checkinteger(L, 3);
    int stride = luaL_checkinteger(L, 4);

    int size = INT_MAX;
    if (nargs > 4) size = luaL_checkinteger(L, 5);
    int available_bytes = buf->size - offset - type_size;
    int max_size = 0;
    if (available_bytes > 0) {
        max_size = available_bytes / stride + 1;
    }
    if (size > max_size) {
        size = max_size;
    }

    bool normalized = false;
    if (nargs > 5) {
        normalized = lua_toboolean(L, 6);
    }

    am_buffer_view *view = am_new_userdata(L, am_buffer_view);
    view->buffer = buf;
    view->buffer_ref = view->ref(L, 1);
    view->offset = offset;
    view->stride = stride;
    view->size = size;
    view->type = type;
    view->type_size = type_size;
    view->normalized = normalized;

    return 1;
}

static int buffer_view_index(lua_State *L) {
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 1);
    int index = lua_tointeger(L, 2);
    if (index < 1 || index > view->size) {
        lua_pushnil(L);
        return 1;
    }
    switch (view->type) {
        case AM_BUF_ELEM_TYPE_FLOAT: {
            lua_pushnumber(L, view->get_float(index-1));
            break;
        }
        case AM_BUF_ELEM_TYPE_FLOAT2: {
            view->get_float2(index-1, am_new_userdata(L, am_vec2));
            break;
        }
        case AM_BUF_ELEM_TYPE_FLOAT3: {
            view->get_float3(index-1, am_new_userdata(L, am_vec3));
            break;
        }
        case AM_BUF_ELEM_TYPE_FLOAT4: {
            view->get_float4(index-1, am_new_userdata(L, am_vec4));
            break;
        }
        case AM_BUF_ELEM_TYPE_UBYTE: {
            if (view->normalized) {
                lua_pushnumber(L, ((lua_Number)view->get_ubyte(index-1)) / 255.0);
            } else {
                lua_pushinteger(L, view->get_ubyte(index-1));
            }
            break;
        }
    }
    return 1;
}

static int buffer_view_newindex(lua_State *L) {
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 1);
    int index = lua_tointeger(L, 2);
    if (index < 1 || index > view->size) {
        if (lua_isnumber(L, 2)) {
            return luaL_error(L, "view index %d not in range [1, %d]", index, view->size);
        } else {
            return luaL_error(L, "view index must be an integer (got %s)", lua_typename(L, lua_type(L, 2)));
        }
    }
    switch (view->type) {
        case AM_BUF_ELEM_TYPE_FLOAT: {
            view->set_float(index-1, luaL_checknumber(L, 3));
            break;
        }
        case AM_BUF_ELEM_TYPE_FLOAT2: {
            view->set_float2(index-1, am_get_userdata(L, am_vec2, 3));
            break;
        }
        case AM_BUF_ELEM_TYPE_FLOAT3: {
            view->set_float3(index-1, am_get_userdata(L, am_vec3, 3));
            break;
        }
        case AM_BUF_ELEM_TYPE_FLOAT4: {
            view->set_float4(index-1, am_get_userdata(L, am_vec4, 3));
            break;
        }
        case AM_BUF_ELEM_TYPE_UBYTE: {
            if (view->normalized) {
                view->set_ubyte(index-1, am_clamp(luaL_checknumber(L, 3), 0.0, 1.0) * 255.0);
            } else {
                view->set_ubyte(index-1, am_clamp(luaL_checkinteger(L, 3), 0, 255));
            }
            break;
        }
    }
    am_buffer *buf = view->buffer;
    if (buf->vbo.buffer_id != 0) {
        int byte_start = view->offset + view->stride * (index-1);
        int byte_end = byte_start + view->type_size;
        if (byte_start < buf->vbo.dirty_start) {
            buf->vbo.dirty_start = byte_start;
        } 
        if (byte_end > buf->vbo.dirty_end) {
            buf->vbo.dirty_end = byte_end;
        }
    }
    return 0;
}

static void register_buffer_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, buffer_gc, 0);
    lua_setfield(L, -2, "__gc");

    lua_pushstring(L, "buffer");
    lua_setfield(L, -2, "tname");

    lua_pushcclosure(L, create_buffer_view, 0);
    lua_setfield(L, -2, "view");

    am_register_metatable(L, MT_am_buffer, 0);
}

static void register_buffer_view_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L, buffer_view_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, buffer_view_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    lua_pushstring(L, "buffer_view");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_buffer_view, 0);
}

void am_open_buffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"buffer", create_buffer},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_buffer_mt(L);
    register_buffer_view_mt(L);
}

void am_buf_view_type_to_attr_client_type_and_dimensions(am_buffer_view_type t, am_attribute_client_type *ctype, int *dims) {
    switch (t) {
        case AM_BUF_ELEM_TYPE_FLOAT:
            *ctype = AM_ATTRIBUTE_CLIENT_TYPE_FLOAT; *dims = 1;
            return;
        case AM_BUF_ELEM_TYPE_FLOAT2:
            *ctype = AM_ATTRIBUTE_CLIENT_TYPE_FLOAT; *dims = 2;
            return;
        case AM_BUF_ELEM_TYPE_FLOAT3:
            *ctype = AM_ATTRIBUTE_CLIENT_TYPE_FLOAT; *dims = 3;
            return;
        case AM_BUF_ELEM_TYPE_FLOAT4:
            *ctype = AM_ATTRIBUTE_CLIENT_TYPE_FLOAT; *dims = 4;
            return;
        case AM_BUF_ELEM_TYPE_UBYTE:
            *ctype = AM_ATTRIBUTE_CLIENT_TYPE_UBYTE; *dims = 1;
            return;
    }
}
