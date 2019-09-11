#include "amulet.h"

static double total_texture_memory = 0.0;

static int create_texture2d(lua_State *L) {
    if (!am_gl_is_initialized()) {
        return luaL_error(L, "you need to create a window before creating a texture");
    }
    int width = 0;
    int height = 0;
    am_texture_min_filter minfilter = AM_MIN_FILTER_NEAREST;
    am_texture_mag_filter magfilter = AM_MAG_FILTER_NEAREST;
    am_texture_wrap swrap = AM_TEXTURE_WRAP_CLAMP_TO_EDGE;
    am_texture_wrap twrap = AM_TEXTURE_WRAP_CLAMP_TO_EDGE;
    am_texture_format format = AM_TEXTURE_FORMAT_RGBA;
    am_texture_type type = AM_TEXTURE_TYPE_UBYTE;
    am_image_buffer *image_buffer = NULL;
    uint8_t *raw_img_data = NULL;
    int nargs = am_check_nargs(L, 1);
    int arg1_type = am_get_type(L, 1);
    switch (arg1_type) {
        case MT_am_image_buffer:
            image_buffer = am_get_userdata(L, am_image_buffer, 1);
            width = image_buffer->width;
            height = image_buffer->height;
            am_pixel_to_texture_format(image_buffer->format, &format, &type);
            break;
        case LUA_TNUMBER:
            width = lua_tointeger(L, 1);
            if (nargs > 1) {
                height = lua_tointeger(L, 2);
            } else {
                height = width;
            }
            if (width <= 0) {
                return luaL_error(L, "width must be positive");
            }
            if (height <= 0) {
                return luaL_error(L, "height must be positive");
            }
            break;
        case LUA_TSTRING: {
            char *errmsg;
            const char *filename = lua_tostring(L, 1);
            if (!am_load_image(filename, &raw_img_data, &width, &height, &errmsg)) {
                lua_pushfstring(L, "unable to load texture %s: %s", filename, errmsg);
                free(errmsg);
                return lua_error(L);
            }
            break;
        }
        default:
            return luaL_argerror(L, 1, "expecting an image_buffer, string or number");
    }
            
    int pixel_size = am_compute_pixel_size(format, type);
    int required_size = pixel_size * width * height;
    if (image_buffer != NULL) {
        if (image_buffer->buffer->texture2d != NULL) {
            return luaL_error(L, "buffer already backing another texture");
        }
        if (required_size != image_buffer->buffer->size) {
            return luaL_error(L, "buffer has wrong size (%d, expecting %d)", image_buffer->buffer->size, required_size);
        }
    }
    am_texture2d *texture = am_new_userdata(L, am_texture2d);
    texture->texture_id = am_create_texture();
    texture->width = width;
    texture->height = height;
    texture->format = format;
    texture->type = type;
    texture->pixel_size = pixel_size;
    texture->has_mipmap = false;
    texture->last_video_capture_frame = 0;
    texture->minfilter = minfilter;
    texture->magfilter = magfilter;
    texture->swrap = swrap;
    texture->twrap = twrap;
    texture->image_buffer = NULL;
    texture->image_buffer_ref = LUA_NOREF;
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, texture->texture_id);
    am_set_texture_min_filter(AM_TEXTURE_BIND_TARGET_2D, minfilter);
    am_set_texture_mag_filter(AM_TEXTURE_BIND_TARGET_2D, magfilter);
    am_set_texture_wrap(AM_TEXTURE_BIND_TARGET_2D, swrap, twrap);
    if (image_buffer != NULL) {
        am_set_texture_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, format, width, height, type, image_buffer->buffer->data);

        texture->image_buffer = image_buffer;
        image_buffer->push(L);
        texture->image_buffer_ref = texture->ref(L, -1);
        lua_pop(L, 1); // image_buffer

        image_buffer->buffer->update_if_dirty();
        image_buffer->buffer->texture2d = texture;
        image_buffer->buffer->ref(L, -1);
    } else if (raw_img_data != NULL) {
        am_set_texture_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, format, width, height, type, raw_img_data);
        free(raw_img_data);
    } else {
        void *data = malloc(required_size);
        memset(data, 0, required_size);
        am_set_texture_image_2d(AM_TEXTURE_COPY_TARGET_2D, 0, format, width, height, type, data);
        free(data);
    }
    
    total_texture_memory += required_size;

    return 1;
}

void am_texture2d::update_dirty() {
    if (image_buffer == NULL) return;
    am_buffer *buffer = image_buffer->buffer;
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
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, 0);
    am_delete_texture(texture->texture_id);
    total_texture_memory -= texture->pixel_size * texture->width * texture->height;
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

static void get_texture_image_buffer(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    if (tex->image_buffer == NULL) {
        lua_pushnil(L);
    } else {
        assert(tex->image_buffer_ref != LUA_NOREF);
        tex->pushref(L, tex->image_buffer_ref);
    }
}

static am_property texture_width_property = {get_texture_width, NULL};
static am_property texture_height_property = {get_texture_height, NULL};
static am_property texture_image_buffer_property = {get_texture_image_buffer, NULL};

static void get_texture_minfilter(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    am_push_enum(L, am_texture_min_filter, tex->minfilter);
}

static void set_texture_minfilter(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    am_texture_min_filter minfilter = am_get_enum(L, am_texture_min_filter, 3);
    bool needs_mipmap = false;
    switch (minfilter) {
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
    if (needs_mipmap && !am_is_power_of_two(tex->width)) {
        luaL_error(L, "texture width must be power of two when using mipmaps (width = %d)",
            tex->width);
    }
    if (needs_mipmap && !am_is_power_of_two(tex->height)) {
        luaL_error(L, "texture height must be power of two when using mipmaps (height = %d)",
            tex->height);
    }
    tex->minfilter = minfilter;
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, tex->texture_id);
    am_set_texture_min_filter(AM_TEXTURE_BIND_TARGET_2D, tex->minfilter);
    if (needs_mipmap && !tex->has_mipmap) {
        am_generate_mipmap(AM_TEXTURE_BIND_TARGET_2D);
    }
    tex->has_mipmap = needs_mipmap;
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

static void set_texture_filter(lua_State *L, void *obj) {
    set_texture_minfilter(L, obj);
    set_texture_magfilter(L, obj);
}

static am_property texture_minfilter_property = {get_texture_minfilter, set_texture_minfilter};
static am_property texture_magfilter_property = {get_texture_magfilter, set_texture_magfilter};
static am_property texture_filter_property = {get_texture_magfilter, set_texture_filter};

static void get_texture_swrap(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    am_push_enum(L, am_texture_wrap, tex->swrap);
}

static void set_texture_swrap(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    am_texture_wrap swrap = am_get_enum(L, am_texture_wrap, 3);
    if (swrap == AM_TEXTURE_WRAP_REPEAT && !am_is_power_of_two(tex->width)) {
        luaL_error(L, "texture width must be a power of 2 when using repeat wrapping (width = %d)", tex->width);
    }
    if (swrap == AM_TEXTURE_WRAP_MIRRORED_REPEAT && !am_is_power_of_two(tex->width)) {
        luaL_error(L, "texture width must be a power of 2 when using mirrored repeat wrapping (width = %d)", tex->width);
    }
    tex->swrap = swrap;
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, tex->texture_id);
    am_set_texture_wrap(AM_TEXTURE_BIND_TARGET_2D, tex->swrap, tex->twrap);
}

static void get_texture_twrap(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    am_push_enum(L, am_texture_wrap, tex->twrap);
}

static void set_texture_twrap(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    am_texture_wrap twrap = am_get_enum(L, am_texture_wrap, 3);
    if (twrap == AM_TEXTURE_WRAP_REPEAT && !am_is_power_of_two(tex->height)) {
        luaL_error(L, "texture height must be a power of 2 when using repeat wrapping (height = %d)", tex->height);
    }
    if (twrap == AM_TEXTURE_WRAP_MIRRORED_REPEAT && !am_is_power_of_two(tex->height)) {
        luaL_error(L, "texture height must be a power of 2 when using mirrored repeat wrapping (height = %d)", tex->height);
    }
    tex->twrap = twrap;
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, tex->texture_id);
    am_set_texture_wrap(AM_TEXTURE_BIND_TARGET_2D, tex->swrap, tex->twrap);
}

static void set_texture_wrap(lua_State *L, void *obj) {
    am_texture2d *tex = (am_texture2d*)obj;
    am_texture_wrap wrap = am_get_enum(L, am_texture_wrap, 3);
    if (wrap == AM_TEXTURE_WRAP_REPEAT && !am_is_power_of_two(tex->height)) {
        luaL_error(L, "texture size must be a power of 2 when using repeat wrapping (size = %dx%d)", tex->width, tex->height);
    }
    if (wrap == AM_TEXTURE_WRAP_MIRRORED_REPEAT && !am_is_power_of_two(tex->height)) {
        luaL_error(L, "texture size must be a power of 2 when using mirrored repeat wrapping (size = %dx%d)", tex->width, tex->height);
    }
    tex->swrap = wrap;
    tex->twrap = wrap;
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, tex->texture_id);
    am_set_texture_wrap(AM_TEXTURE_BIND_TARGET_2D, tex->swrap, tex->twrap);
}

static am_property texture_swrap_property = {get_texture_swrap, set_texture_swrap};
static am_property texture_twrap_property = {get_texture_twrap, set_texture_twrap};
static am_property texture_wrap_property = {get_texture_swrap, set_texture_wrap};

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
    am_register_property(L, "image_buffer", &texture_image_buffer_property);
    am_register_property(L, "minfilter", &texture_minfilter_property);
    am_register_property(L, "magfilter", &texture_magfilter_property);
    am_register_property(L, "filter", &texture_filter_property);
    am_register_property(L, "swrap", &texture_swrap_property);
    am_register_property(L, "twrap", &texture_twrap_property);
    am_register_property(L, "wrap", &texture_wrap_property);

    am_register_metatable(L, "texture2d", MT_am_texture2d, 0);
}

static int get_total_texture_mem(lua_State *L) {
    lua_pushnumber(L, total_texture_memory / 1024.0);
    return 1;
}

void am_open_texture2d_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"texture2d",    create_texture2d},
        {"_total_texture_mem",    get_total_texture_mem},
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
        {"8",               AM_TEXTURE_TYPE_UBYTE},
        {"565",             AM_TEXTURE_TYPE_USHORT_5_6_5},
        {"4444",            AM_TEXTURE_TYPE_USHORT_4_4_4_4},
        {"5551",            AM_TEXTURE_TYPE_USHORT_5_5_5_1},
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
