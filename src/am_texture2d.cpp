#include "amulet.h"

static int create_texture2d(lua_State *L) {
    if (!am_gl_is_initialized()) {
        return luaL_error(L, "you need to create a window before creating a texture");
    }
    am_check_nargs(L, 1);
    if (!lua_istable(L, 1)) return luaL_error(L, "expecting a table in position 1");
    int width = 0;
    int height = 0;
    am_texture_min_filter min_filter = AM_MIN_FILTER_NEAREST;
    am_texture_mag_filter mag_filter = AM_MAG_FILTER_NEAREST;
    am_texture_wrap s_wrap = AM_TEXTURE_WRAP_CLAMP_TO_EDGE;
    am_texture_wrap t_wrap = AM_TEXTURE_WRAP_CLAMP_TO_EDGE;
    am_texture_format format = AM_TEXTURE_FORMAT_RGBA;
    am_texture_type type = AM_PIXEL_TYPE_UBYTE;
    am_buffer *buffer = NULL;
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        const char *key = luaL_checkstring(L, -2);
        if (strcmp(key, "width") == 0) {
            width = luaL_checkinteger(L, -1);
            if (width <= 0) return luaL_error(L, "width must be positive");
        } else if (strcmp(key, "height") == 0) {
            height = luaL_checkinteger(L, -1);
            if (height <= 0) return luaL_error(L, "height must be positive");
        } else if (strcmp(key, "minfilter") == 0) {
            min_filter = am_get_enum(L, am_texture_min_filter, -1);
        } else if (strcmp(key, "magfilter") == 0) {
            mag_filter = am_get_enum(L, am_texture_mag_filter, -1);
        } else if (strcmp(key, "swrap") == 0) {
            s_wrap = am_get_enum(L, am_texture_wrap, -1);
        } else if (strcmp(key, "twrap") == 0) {
            t_wrap = am_get_enum(L, am_texture_wrap, -1);
        } else if (strcmp(key, "format") == 0) {
            format = am_get_enum(L, am_texture_format, -1);
        } else if (strcmp(key, "type") == 0) {
            type = am_get_enum(L, am_texture_type, -1);
        } else if (strcmp(key, "buffer") == 0) {
            buffer = am_get_userdata(L, am_buffer, -1);
        } else if (strcmp(key, "image") == 0) {
            am_image *img = am_get_userdata(L, am_image, -1);
            buffer = img->buffer;
            width = img->width;
            height = img->height;
        } else {
            return luaL_error(L, "unrecognised texture setting: '%s'", key);
        }
        lua_pop(L, 1); // pop value
    }
    if (width == 0) {
        return luaL_error(L, "width missing");
    }
    if (height == 0) {
        return luaL_error(L, "height missing");
    }

    int pixel_size = am_compute_pixel_size(format, type);
    int required_size = pixel_size * width * height;
    if (buffer != NULL) {
        if (buffer->texture2d != NULL) {
            return luaL_error(L, "buffer already backing another texture");
        }
        if (required_size != buffer->size) {
            return luaL_error(L, "buffer has wrong size (%d, expecting %d)", buffer->size, required_size);
        }
    }
    if (s_wrap == AM_TEXTURE_WRAP_REPEAT && !am_is_power_of_two(width)) {
        return luaL_error(L, "texture width must be power of two when using repeat wrapping (in fact %d)", width);
    }
    if (s_wrap == AM_TEXTURE_WRAP_MIRRORED_REPEAT && !am_is_power_of_two(width)) {
        return luaL_error(L, "texture width must be power of two when using mirrored repeat wrapping (in fact %d)", width);
    }
    if (t_wrap == AM_TEXTURE_WRAP_REPEAT && !am_is_power_of_two(height)) {
        return luaL_error(L, "texture height must be power of two when using repeat wrapping (in fact %d)", height);
    }
    if (t_wrap == AM_TEXTURE_WRAP_MIRRORED_REPEAT && !am_is_power_of_two(height)) {
        return luaL_error(L, "texture height must be power of two when using mirrored repeat wrapping (in fact %d)", height);
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
    am_texture2d *texture = am_new_userdata(L, am_texture2d);
    texture->texture_id = am_create_texture();
    texture->width = width;
    texture->height = height;
    texture->format = format;
    texture->type = type;
    texture->pixel_size = pixel_size;
    texture->has_mipmap = needs_mipmap;
    texture->last_video_capture_frame = 0;
    texture->minfilter = min_filter;
    texture->magfilter = mag_filter;
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, texture->texture_id);
    am_set_texture_min_filter(AM_TEXTURE_BIND_TARGET_2D, min_filter);
    am_set_texture_mag_filter(AM_TEXTURE_BIND_TARGET_2D, mag_filter);
    am_set_texture_wrap(AM_TEXTURE_BIND_TARGET_2D, s_wrap, t_wrap);
    if (buffer != NULL) {
        am_set_texture_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, format, width, height, type, buffer->data);

        texture->buffer = buffer;
        buffer->push(L);
        texture->buffer_ref = texture->ref(L, -1);
        lua_pop(L, 1); // buffer

        buffer->update_if_dirty();
        buffer->texture2d = texture;
        buffer->texture2d_ref = buffer->ref(L, -1);
        buffer->track_dirty = true;
    } else {
        void *data = malloc(required_size);
        memset(data, 0, required_size);
        am_set_texture_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, format, width, height, type, data);
        free(data);
        texture->buffer = NULL;
        texture->buffer_ref = LUA_NOREF;
    }

    if (needs_mipmap) am_generate_mipmap(AM_TEXTURE_BIND_TARGET_2D);

    return 1;
}

void am_texture2d::update_from_buffer() {
    if (buffer == NULL) return;
    if (buffer->dirty_start >= buffer->dirty_end) return;

    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, texture_id);
    int dirty_pixel_start = buffer->dirty_start / pixel_size;
    int dirty_pixel_end = (buffer->dirty_end - 1) / pixel_size + 1;
    int start_row = dirty_pixel_start / width;
    int end_row = (dirty_pixel_end - 1) / width;
    if (start_row == end_row) {
        int x = dirty_pixel_start % width;
        int data_offset = dirty_pixel_start * pixel_size;
        am_set_texture_sub_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, x, start_row,
            dirty_pixel_end - dirty_pixel_start, 1,
            format, type, buffer->data + data_offset);
    } else {
        int data_offset = start_row * width * pixel_size;
        am_set_texture_sub_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, 0, start_row, width,
            end_row - start_row + 1, format, type, buffer->data + data_offset);
    }
    if (has_mipmap) am_generate_mipmap(AM_TEXTURE_BIND_TARGET_2D);
}

static int capture_video(lua_State *L) {
    am_texture2d *texture = am_get_userdata(L, am_texture2d, 1);
    int next_frame = am_next_video_capture_frame();
    if (next_frame != texture->last_video_capture_frame) {
        am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, texture->texture_id);
        am_copy_video_frame_to_texture();
        texture->last_video_capture_frame = next_frame;
        //am_debug("%s", "updated texture");
    }
    return 0;
}

static int texture2d_gc(lua_State *L) {
    am_texture2d *texture = am_get_userdata(L, am_texture2d, 1);
    am_delete_texture(texture->texture_id);
    return 0;
}

static void get_texture_width(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    lua_pushinteger(L, tex->width);
}

static void get_texture_height(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    lua_pushinteger(L, tex->height);
}

static void get_texture_buffer(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    if (tex->buffer == NULL) {
        lua_pushnil(L);
    } else {
        assert(tex->buffer_ref != LUA_NOREF);
        tex->pushref(L, tex->buffer_ref);
    }
}

static am_property texture_width_property = {get_texture_width, NULL};
static am_property texture_height_property = {get_texture_height, NULL};
static am_property texture_buffer_property = {get_texture_buffer, NULL};

static void get_texture_minfilter(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    am_push_enum(L, am_texture_min_filter, tex->minfilter);
}

static void set_texture_minfilter(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    tex->minfilter = am_get_enum(L, am_texture_min_filter, 3);
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, tex->texture_id);
    am_set_texture_min_filter(AM_TEXTURE_BIND_TARGET_2D, tex->minfilter);
}

static void get_texture_magfilter(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    am_push_enum(L, am_texture_mag_filter, tex->magfilter);
}

static void set_texture_magfilter(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    tex->magfilter = am_get_enum(L, am_texture_mag_filter, 3);
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, tex->texture_id);
    am_set_texture_mag_filter(AM_TEXTURE_BIND_TARGET_2D, tex->magfilter);
}

static am_property texture_minfilter_property = {get_texture_minfilter, set_texture_minfilter};
static am_property texture_magfilter_property = {get_texture_magfilter, set_texture_magfilter};

static void register_texture2d_mt(lua_State *L) {
    lua_newtable(L);

    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, texture2d_gc, 0);
    lua_setfield(L, -2, "__gc");

    lua_pushcclosure(L, capture_video, 0);
    lua_setfield(L, -2, "capture_video");

    am_register_property(L, "width",  &texture_width_property);
    am_register_property(L, "height", &texture_height_property);
    am_register_property(L, "buffer", &texture_buffer_property);
    am_register_property(L, "minfilter", &texture_minfilter_property);
    am_register_property(L, "magfilter", &texture_magfilter_property);

    am_register_metatable(L, "texture2d", MT_am_texture2d, 0);
}

void am_open_texture2d_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"texture2d",    create_texture2d},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_texture2d_mt(L);

    am_enum_value texture_format_enum[] = {
        {"alpha",           AM_TEXTURE_FORMAT_ALPHA},
        {"luminance",       AM_TEXTURE_FORMAT_LUMINANCE},
        {"luminance_alpha", AM_TEXTURE_FORMAT_LUMINANCE_ALPHA},
        {"rgb",             AM_TEXTURE_FORMAT_RGB},
        {"rgba",            AM_TEXTURE_FORMAT_RGBA},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_texture_format, texture_format_enum);

    am_enum_value texture_type_enum[] = {
        {"8",               AM_PIXEL_TYPE_UBYTE},
        {"565",             AM_PIXEL_TYPE_USHORT_5_6_5},
        {"4444",            AM_PIXEL_TYPE_USHORT_4_4_4_4},
        {"5551",            AM_PIXEL_TYPE_USHORT_5_5_5_1},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_texture_type, texture_type_enum);

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
