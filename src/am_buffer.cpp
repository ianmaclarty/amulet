#include "amulet.h"

am_buffer::am_buffer(int sz) {
    size = sz;
    data = (uint8_t*)malloc(sz);
    memset(data, 0, size);
    vbo_id = 0;
    texture2d = NULL;
    texture2d_ref = LUA_NOREF;
    dirty_start = INT_MAX;
    dirty_end = 0;
}

void am_buffer::destroy() {
    free(data);
    if (vbo_id != 0) {
        am_delete_buffer(vbo_id);
        vbo_id = 0;
    }
}

void am_buffer::update_if_dirty() {
    if (dirty_start < dirty_end) {
        if (vbo_id != 0) {
            am_bind_buffer(AM_ARRAY_BUFFER, vbo_id);
            am_set_buffer_sub_data(AM_ARRAY_BUFFER, dirty_start, dirty_end - dirty_start, data + dirty_start);
        } 
        if (texture2d != NULL) {
            texture2d->update_from_buffer();
        }
        dirty_start = INT_MAX;
        dirty_end = 0;
    }
}

void am_buffer::create_vbo() {
    assert(vbo_id == 0);
    update_if_dirty();
    vbo_id = am_create_buffer_object();
    am_bind_buffer(AM_ARRAY_BUFFER, vbo_id);
    am_set_buffer_data(AM_ARRAY_BUFFER, size, &data[0], AM_BUFFER_USAGE_STATIC_DRAW);
}

static int create_buffer(lua_State *L) {
    am_check_nargs(L, 1);
    int size = luaL_checkinteger(L, 1);
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

    am_buffer_view_type type = am_get_enum(L, am_buffer_view_type, 2);

    int offset = luaL_checkinteger(L, 3);
    int stride = luaL_checkinteger(L, 4);

    bool normalized = false;
    if (nargs > 5) {
        normalized = lua_toboolean(L, 6);
    }

    int type_size = 0;
    switch (type) {
        case AM_BUF_ELEM_TYPE_FLOAT:
            type_size = sizeof(float);
            break;
        case AM_BUF_ELEM_TYPE_FLOAT2:
            type_size = sizeof(float) * 2;
            break;
        case AM_BUF_ELEM_TYPE_FLOAT3:
            type_size = sizeof(float) * 3;
            break;
        case AM_BUF_ELEM_TYPE_FLOAT4:
            type_size = sizeof(float) * 4;
            break;
        case AM_BUF_ELEM_TYPE_UBYTE:
            type_size = sizeof(uint8_t);
            break;
        case AM_BUF_ELEM_TYPE_USHORT:
            type_size = sizeof(uint16_t);
            break;
        case AM_BUF_ELEM_TYPE_SHORT:
            type_size = sizeof(int16_t);
            break;
        case AM_BUF_ELEM_TYPE_UINT:
            type_size = sizeof(int32_t);
            break;
        case AM_BUF_ELEM_TYPE_INT:
            type_size = sizeof(int32_t);
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
            return luaL_error(L, "the buffer is too small for %d %ss with stride %d (max %d)",
                size, lua_tostring(L, 2), stride, max_size);
        }
    } else {
        size = max_size;
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
        return am_default_index_func(L);
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
        case AM_BUF_ELEM_TYPE_USHORT: {
            if (view->normalized) {
                lua_pushnumber(L, ((lua_Number)view->get_ushort(index-1)) / 65535.0);
            } else {
                lua_pushinteger(L, view->get_ushort(index-1));
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_SHORT: {
            if (view->normalized) {
                lua_pushnumber(L, ((lua_Number)view->get_short(index-1)) / 32767.0);
            } else {
                lua_pushinteger(L, view->get_short(index-1));
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_UINT: {
            if (view->normalized) {
                lua_pushnumber(L, ((lua_Number)view->get_int(index-1)) / (lua_Number)UINT32_MAX);
            } else {
                lua_pushnumber(L, view->get_uint(index-1));
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_INT: {
            if (view->normalized) {
                lua_pushnumber(L, ((lua_Number)view->get_int(index-1)) / 2147483647.0);
            } else {
                lua_pushinteger(L, view->get_int(index-1));
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
        case AM_BUF_ELEM_TYPE_USHORT: {
            if (view->normalized) {
                view->set_ushort(index-1, am_clamp(luaL_checknumber(L, 3), 0.0, 1.0) * 65535.0);
            } else {
                view->set_ushort(index-1, am_clamp(luaL_checkinteger(L, 3), 0, UINT16_MAX));
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_SHORT: {
            if (view->normalized) {
                view->set_short(index-1, am_clamp(luaL_checknumber(L, 3), -1.0, 1.0) * 32767.0);
            } else {
                view->set_short(index-1, am_clamp(luaL_checkinteger(L, 3), INT16_MIN, INT16_MAX));
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_UINT: {
            if (view->normalized) {
                view->set_uint(index-1, am_clamp(luaL_checknumber(L, 3), 0.0, 1.0) * (lua_Number)UINT32_MAX);
            } else {
                view->set_uint(index-1, am_clamp(luaL_checknumber(L, 3), 0.0, (lua_Number)UINT32_MAX));
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_INT: {
            if (view->normalized) {
                view->set_int(index-1, am_clamp(luaL_checknumber(L, 3), -1.0, 1.0) * 2147483647.0);
            } else {
                view->set_int(index-1, am_clamp(luaL_checkinteger(L, 3), INT32_MIN, INT32_MAX));
            }
            break;
        }
    }
    am_buffer *buf = view->buffer;
    if (buf->vbo_id != 0 || buf->texture2d != NULL) {
        int byte_start = view->offset + view->stride * (index-1);
        int byte_end = byte_start + view->type_size;
        if (byte_start < buf->dirty_start) {
            buf->dirty_start = byte_start;
        } 
        if (byte_end > buf->dirty_end) {
            buf->dirty_end = byte_end;
        }
    }
    return 0;
}

#define SET_ONE(T, GET)                                                         \
    lua_rawgeti(L, 2, i++);                                                     \
    switch (lua_type(L, -1)) {                                                  \
        case LUA_TNUMBER:                                                       \
            *((T*)ptr) = (T)(GET);                                              \
            break;                                                              \
        case LUA_TNIL:                                                          \
            more = false;                                                       \
            break;                                                              \
        default:                                                                \
            return luaL_error(L, "expecting a table containing only numbers");  \
    }                                                                           \
    lua_pop(L, 1);

static int buffer_view_set_num_table(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 1);
    if (!lua_istable(L, 2)) {
        return luaL_error(L, "expecting a table in position 2");
    }
    int offset = 0;
    if (nargs > 2) {
        offset = luaL_checkinteger(L, 3)-1;
    }
    if (offset < 0) {
        return luaL_error(L, "offset must be positive (instead %d)", offset+1);
    }
    if (offset >= view->size) {
        return luaL_error(L, "offset (%d) is past the end of the view (size %d)", offset+1, view->size);
    }
    int max_slots = view->size - offset;
    int max_nums = 0;
    switch (view->type) {
        case AM_BUF_ELEM_TYPE_FLOAT:
        case AM_BUF_ELEM_TYPE_UBYTE:
        case AM_BUF_ELEM_TYPE_USHORT:
        case AM_BUF_ELEM_TYPE_SHORT:
        case AM_BUF_ELEM_TYPE_UINT:
        case AM_BUF_ELEM_TYPE_INT:
            max_nums = max_slots;
            break;
        case AM_BUF_ELEM_TYPE_FLOAT2:
            max_nums = max_slots * 2;
            break;
        case AM_BUF_ELEM_TYPE_FLOAT3:
            max_nums = max_slots * 3;
            break;
        case AM_BUF_ELEM_TYPE_FLOAT4:
            max_nums = max_slots * 4;
            break;
    }
    if ((int)lua_objlen(L, 2) > max_nums) {
        return luaL_error(L, "table contains too many values (%d, but view only has space for %d at index %d)",
            lua_objlen(L, 2), max_nums, offset + 1);
    }
    int i = 1;
    uint8_t *ptr = view->buffer->data + view->offset + view->stride * offset;
    bool more = true;
    switch (view->type) {
        case AM_BUF_ELEM_TYPE_FLOAT: {
            while (more) {
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += view->stride;
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_FLOAT2: {
            while (more) {
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += 4;
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += view->stride - 4;
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_FLOAT3: {
            while (more) {
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += 4;
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += 4;
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += view->stride - 8;
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_FLOAT4: {
            while (more) {
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += 4;
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += 4;
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += 4;
                SET_ONE(float, lua_tonumber(L, -1))
                ptr += view->stride - 12;
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_UBYTE: {
            if (view->normalized) {
                while (more) {
                    SET_ONE(uint8_t, am_clamp(lua_tonumber(L, -1), 0.0, 1.0) * 255.0);
                    ptr += view->stride;
                }
            } else {
                while (more) {
                    SET_ONE(uint8_t, am_clamp(lua_tointeger(L, -1), 0, 255));
                    ptr += view->stride;
                }
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_USHORT: {
            if (view->normalized) {
                while (more) {
                    SET_ONE(uint16_t, am_clamp(lua_tonumber(L, -1), 0.0, 1.0) * 65535.0);
                    ptr += view->stride;
                }
            } else {
                while (more) {
                    SET_ONE(uint16_t, am_clamp(lua_tointeger(L, -1), 0, UINT16_MAX));
                    ptr += view->stride;
                }
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_SHORT: {
            if (view->normalized) {
                while (more) {
                    SET_ONE(int16_t, am_clamp(lua_tonumber(L, -1), -1.0, 1.0) * 32767.0);
                    ptr += view->stride;
                }
            } else {
                while (more) {
                    SET_ONE(int16_t, am_clamp(lua_tointeger(L, -1), INT16_MIN, INT16_MAX));
                    ptr += view->stride;
                }
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_UINT: {
            if (view->normalized) {
                while (more) {
                    SET_ONE(uint32_t, am_clamp(lua_tonumber(L, -1), 0.0, 1.0) * (lua_Number)UINT32_MAX);
                    ptr += view->stride;
                }
            } else {
                while (more) {
                    SET_ONE(uint32_t, am_clamp(lua_tonumber(L, -1), 0.0, (lua_Number)UINT32_MAX));
                    ptr += view->stride;
                }
            }
            break;
        }
        case AM_BUF_ELEM_TYPE_INT: {
            if (view->normalized) {
                while (more) {
                    SET_ONE(int32_t, am_clamp(lua_tonumber(L, -1), -1.0, 1.0) * 2147483647.0);
                    ptr += view->stride;
                }
            } else {
                while (more) {
                    SET_ONE(int32_t, am_clamp(lua_tointeger(L, -1), INT32_MIN, INT32_MAX));
                    ptr += view->stride;
                }
            }
            break;
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

    lua_pushcclosure(L, buffer_view_set_num_table, 0);
    lua_setfield(L, -2, "set_nums");

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

    am_enum_value view_type_enum[] = {
        {"float",           AM_BUF_ELEM_TYPE_FLOAT},
        {"vec2",            AM_BUF_ELEM_TYPE_FLOAT2},
        {"vec3",            AM_BUF_ELEM_TYPE_FLOAT3},
        {"vec4",            AM_BUF_ELEM_TYPE_FLOAT4},
        {"ubyte",           AM_BUF_ELEM_TYPE_UBYTE},
        {"ushort",          AM_BUF_ELEM_TYPE_USHORT},
        {"short",           AM_BUF_ELEM_TYPE_SHORT},
        {"uint",            AM_BUF_ELEM_TYPE_UINT},
        {"int",             AM_BUF_ELEM_TYPE_INT},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_buffer_view_type, view_type_enum);

    register_buffer_mt(L);
    register_buffer_view_mt(L);
}
