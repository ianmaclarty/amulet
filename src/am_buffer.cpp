#include "amulet.h"

am_buffer::am_buffer(int sz) {
    size = sz;
    data = (uint8_t*)malloc(sz);
    memset(data, 0, size);
    vbo_id = 0;
    texture2d_info = NULL;
    dirty_start = INT_MAX;
    dirty_end = 0;
}

void am_buffer::destroy() {
    free(data);
    if (vbo_id != 0) {
        am_delete_buffer(vbo_id);
        vbo_id = 0;
    }
    if (texture2d_info != NULL) {
        am_delete_texture(texture2d_info->texture_id);
        free(texture2d_info);
        texture2d_info = NULL;
    }
}

void am_buffer::update_if_dirty() {
    if (dirty_start < dirty_end) {
        if (vbo_id != 0) {
            am_bind_buffer(AM_ARRAY_BUFFER, vbo_id);
            am_set_buffer_sub_data(AM_ARRAY_BUFFER, dirty_start, dirty_end - dirty_start, data + dirty_start);
        } 
        if (texture2d_info != NULL) {
            am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, texture2d_info->texture_id);
            int pixel_size = texture2d_info->pixel_size;
            int dirty_pixel_start = dirty_start / pixel_size;
            int dirty_pixel_end = (dirty_end - 1) / pixel_size + 1;
            int start_row = dirty_pixel_start / texture2d_info->width;
            int end_row = (dirty_pixel_end - 1) / texture2d_info->width;
            if (start_row == end_row) {
                int x = dirty_pixel_start % texture2d_info->width;
                int data_offset = dirty_pixel_start * pixel_size;
                am_set_texture_sub_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, x, start_row,
                    dirty_pixel_end - dirty_pixel_start, 1,
                    texture2d_info->format, texture2d_info->type, data + data_offset);
            } else {
                int data_offset = start_row * texture2d_info->width * pixel_size;
                am_set_texture_sub_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, 0, start_row, texture2d_info->width,
                    end_row - start_row + 1, texture2d_info->format, texture2d_info->type, data + data_offset);
            }
            if (texture2d_info->has_mipmap) am_generate_mipmap(AM_TEXTURE_BIND_TARGET_2D);
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

static int setup_texture2d(lua_State *L) {
    int nargs = am_check_nargs(L, 3);
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    if (buf->texture2d_info != NULL) {
        return luaL_error(L, "2d texture already set up for this buffer");
    }
    int width = luaL_checkinteger(L, 2);
    int height = luaL_checkinteger(L, 3);
    am_texture_min_filter min_filter = AM_MIN_FILTER_NEAREST;
    am_texture_mag_filter mag_filter = AM_MAG_FILTER_NEAREST;
    am_texture_wrap s_wrap = AM_TEXTURE_WRAP_CLAMP_TO_EDGE;
    am_texture_wrap t_wrap = AM_TEXTURE_WRAP_CLAMP_TO_EDGE;
    am_texture_format format = AM_TEXTURE_FORMAT_RGBA;
    am_pixel_type type = AM_PIXEL_TYPE_UBYTE;
    if (nargs > 3) min_filter = am_get_enum(L, am_texture_min_filter, 4);
    if (nargs > 4) mag_filter = am_get_enum(L, am_texture_mag_filter, 5);
    if (nargs > 5) s_wrap = am_get_enum(L, am_texture_wrap, 6);
    if (nargs > 6) t_wrap = am_get_enum(L, am_texture_wrap, 7);
    if (nargs > 7) format = am_get_enum(L, am_texture_format, 8);
    if (nargs > 8) type = am_get_enum(L, am_pixel_type, 9);
    int pixel_size = am_compute_pixel_size(format, type);
    int required_size = pixel_size * width * height;
    if (required_size != buf->size) {
        return luaL_error(L, "buffer has wrong size (%d, expecting %d)", buf->size, required_size);
    }
    if (s_wrap == AM_TEXTURE_WRAP_REPEAT && !am_is_power_of_two(width)) {
        return luaL_error(L, "texture width must be power of two when using repeat (in fact %d)", width);
    }
    if (s_wrap == AM_TEXTURE_WRAP_MIRRORED_REPEAT && !am_is_power_of_two(width)) {
        return luaL_error(L, "texture width must be power of two when using mirrored_repeat (in fact %d)", width);
    }
    if (t_wrap == AM_TEXTURE_WRAP_REPEAT && !am_is_power_of_two(height)) {
        return luaL_error(L, "texture height must be power of two when using repeat (in fact %d)", height);
    }
    if (t_wrap == AM_TEXTURE_WRAP_MIRRORED_REPEAT && !am_is_power_of_two(height)) {
        return luaL_error(L, "texture height must be power of two when using mirrored_repeat (in fact %d)", height);
    }
    bool needs_mipmap = false;
    switch (min_filter) {
        case AM_MIN_FILTER_NEAREST:
        case AM_MIN_FILTER_LINEAR:
            needs_mipmap = false;
            break;
        case AM_MIN_FILTER_NEAREST_MIPMAP_NEAREST:
        case AM_MIN_FILTER_NEAREST_MIPMAP_LINEAR:
        case AM_MIN_FILTER_LINEAR_MIPMAP_NEAREST:
        case AM_MIN_FILTER_LINEAR_MIPMAP_LINEAR:
            needs_mipmap = true;
            break;
    }
    if (needs_mipmap && !am_is_power_of_two(width)) {
        return luaL_error(L, "texture width must be power of two when using mipmaps (in fact %d)",
            width);
    }
    if (needs_mipmap && !am_is_power_of_two(height)) {
        return luaL_error(L, "texture height must be power of two when using mipmaps (in fact %d)",
            height);
    }
    buf->update_if_dirty();
    buf->texture2d_info = (am_texture2d_info*)malloc(sizeof(am_texture2d_info));
    buf->texture2d_info->texture_id = am_create_texture();
    buf->texture2d_info->width = width;
    buf->texture2d_info->height = height;
    buf->texture2d_info->format = format;
    buf->texture2d_info->type = type;
    buf->texture2d_info->pixel_size = pixel_size;
    buf->texture2d_info->has_mipmap = needs_mipmap;
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, buf->texture2d_info->texture_id);
    am_set_texture_min_filter(AM_TEXTURE_BIND_TARGET_2D, min_filter);
    am_set_texture_mag_filter(AM_TEXTURE_BIND_TARGET_2D, mag_filter);
    am_set_texture_wrap(AM_TEXTURE_BIND_TARGET_2D, s_wrap, t_wrap);
    am_set_texture_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, format, width, height, type, buf->data);
    if (needs_mipmap) am_generate_mipmap(AM_TEXTURE_BIND_TARGET_2D);
    return 0;
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
        case AM_BUF_ELEM_TYPE_INT:
            type_size = sizeof(int32_t);
            break;
    }

    int offset = luaL_checkinteger(L, 3);
    int stride = luaL_checkinteger(L, 4);

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
    if (buf->vbo_id != 0 || buf->texture2d_info != NULL) {
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

static void register_buffer_mt(lua_State *L, bool worker) {
    lua_newtable(L);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, buffer_gc, 0);
    lua_setfield(L, -2, "__gc");

    lua_pushstring(L, "buffer");
    lua_setfield(L, -2, "tname");

    lua_pushcclosure(L, create_buffer_view, 0);
    lua_setfield(L, -2, "view");
    if (!worker) {
        lua_pushcclosure(L, setup_texture2d, 0);
        lua_setfield(L, -2, "setup_texture2d");
    }

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

void am_open_buffer_module(lua_State *L, bool worker) {
    luaL_Reg funcs[] = {
        {"buffer", create_buffer},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);

    am_enum_value view_type_enum[] = {
        {"float",           AM_BUF_ELEM_TYPE_FLOAT},
        {"float2",          AM_BUF_ELEM_TYPE_FLOAT2},
        {"float3",          AM_BUF_ELEM_TYPE_FLOAT3},
        {"float4",          AM_BUF_ELEM_TYPE_FLOAT4},
        {"ubyte",           AM_BUF_ELEM_TYPE_UBYTE},
        {"ushort",          AM_BUF_ELEM_TYPE_USHORT},
        {"short",           AM_BUF_ELEM_TYPE_SHORT},
        {"int",             AM_BUF_ELEM_TYPE_INT},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_buffer_view_type, view_type_enum);

    if (!worker) {
        am_enum_value texture_format_enum[] = {
            {"alpha",           AM_TEXTURE_FORMAT_ALPHA},
            {"luminance",       AM_TEXTURE_FORMAT_LUMINANCE},
            {"luminance_alpha", AM_TEXTURE_FORMAT_LUMINANCE_ALPHA},
            {"rgb",             AM_TEXTURE_FORMAT_RGB},
            {"rgba",            AM_TEXTURE_FORMAT_RGBA},
            {NULL, 0}
        };
        am_register_enum(L, ENUM_am_texture_format, texture_format_enum);

        am_enum_value pixel_type_enum[] = {
            {"8",               AM_PIXEL_TYPE_UBYTE},
            {"565",             AM_PIXEL_TYPE_USHORT_5_6_5},
            {"4444",            AM_PIXEL_TYPE_USHORT_4_4_4_4},
            {"5551",            AM_PIXEL_TYPE_USHORT_5_5_5_1},
            {NULL, 0}
        };
        am_register_enum(L, ENUM_am_pixel_type, pixel_type_enum);

        am_enum_value min_filter_enum[] = {
            {"nearest",                 AM_MIN_FILTER_NEAREST},
            {"linear",                  AM_MIN_FILTER_LINEAR},
            {"nearest_mipmap_nearest",  AM_MIN_FILTER_NEAREST_MIPMAP_NEAREST},
            {"linear_mipmap_nearest",   AM_MIN_FILTER_LINEAR_MIPMAP_NEAREST},
            {"nearest_mipmap_linear",   AM_MIN_FILTER_NEAREST_MIPMAP_LINEAR},
            {"linear_mipmap_linear",    AM_MIN_FILTER_LINEAR_MIPMAP_LINEAR},
            {NULL, 0}
        };
        am_register_enum(L, ENUM_am_texture_min_filter, min_filter_enum);

        am_enum_value mag_filter_enum[] = {
            {"nearest",         AM_MAG_FILTER_NEAREST},
            {"linear",          AM_MAG_FILTER_LINEAR},
            {NULL, 0}
        };
        am_register_enum(L, ENUM_am_texture_mag_filter, mag_filter_enum);

        am_enum_value texture_wrap_enum[] = {
            {"clamp_to_edge",   AM_TEXTURE_WRAP_CLAMP_TO_EDGE},
            {"mirrored_repeat", AM_TEXTURE_WRAP_MIRRORED_REPEAT},
            {"repeat",          AM_TEXTURE_WRAP_REPEAT},
            {NULL, 0}
        };
        am_register_enum(L, ENUM_am_texture_wrap, texture_wrap_enum);
    }

    register_buffer_mt(L, worker);
    register_buffer_view_mt(L);
}
