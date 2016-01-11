#include "amulet.h"

static int pixel_format_size(am_pixel_format fmt) {
    switch (fmt) {
        case AM_PIXEL_FORMAT_RGBA8: return 4;
    }
    return 0;
}

/******************************************************************************/

am_image_buffer::am_image_buffer() {
    width = 0;
    height = 0;
    format = AM_PIXEL_FORMAT_RGBA8;
    buffer = NULL;
    buffer_ref = LUA_NOREF;
}

static int create_image_buffer(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_image_buffer *img = am_new_userdata(L, am_image_buffer);
    int arg = 1;
    if (am_get_type(L, arg) == MT_am_buffer) {
        img->buffer = am_get_userdata(L, am_buffer, arg);
        img->buffer_ref = img->ref(L, arg);
        arg++;
    }
    if (nargs < arg) {
        return luaL_error(L, "not enough arguments");
    }
    img->width = luaL_checkinteger(L, arg++);
    if (img->width <= 0) {
        return luaL_error(L, "width must be positive");
    }
    if (nargs >= arg) {
        img->height = luaL_checkinteger(L, arg++);
    } else {
        img->height = img->width;
    }
    if (img->height <= 0) {
        return luaL_error(L, "height must be positive");
    }
    img->format = AM_PIXEL_FORMAT_RGBA8;
    int required_size = img->width * img->height * pixel_format_size(img->format);
    if (img->buffer == NULL) {
        // create new buffer
        img->buffer = am_new_userdata(L, am_buffer, required_size);
        img->buffer_ref = img->ref(L, -1);
        lua_pop(L, 1); // pop buffer;
    } else {
        // check supplied buffer has correct size.
        if (required_size != img->buffer->size) {
            return luaL_error(L, "buffer has wrong size (%d, expecting %d)", img->buffer->size, required_size);
        }
    }
    return 1;
}

static int load_image(lua_State *L) {
    am_check_nargs(L, 1);
    char *errmsg;
    int len;
    const char *filename = luaL_checkstring(L, 1);
    void *data = am_read_resource(filename, &len, &errmsg);
    if (data == NULL) {
        lua_pushstring(L, errmsg);
        free(errmsg);
        return lua_error(L);
    }
    int width, height;
    int components = 4;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc *img_data =
        stbi_load_from_memory((stbi_uc const *)data, len, &width, &height, &components, 4);
    free(data);
    if (img_data == NULL) {
        return luaL_error(L, "error loading image %s: %s", filename, stbi_failure_reason());
    }
    am_image_buffer *img = am_new_userdata(L, am_image_buffer);
    img->width = width;
    img->height = height;
    img->format = AM_PIXEL_FORMAT_RGBA8;
    int sz = width * height * pixel_format_size(img->format);
    am_buffer *imgbuf = am_new_userdata(L, am_buffer);
    imgbuf->size = sz;
    imgbuf->data = img_data;
    img->buffer = imgbuf;
    img->buffer_ref = img->ref(L, -1);
    lua_pop(L, 1); // pop imgbuf
    return 1;
}

int am_load_image(lua_State *L) {
    return load_image(L);
}

static int load_embedded_image(lua_State *L) {
    am_check_nargs(L, 1);
    const char *filename = luaL_checkstring(L, 1);
    am_embedded_file_record *rec = am_get_embedded_file(filename);
    if (rec == NULL) {
        return luaL_error(L, "embedded file not found: %s", filename);
    }
    int width, height;
    int components = 4;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc *img_data =
        stbi_load_from_memory((stbi_uc const *)rec->data, rec->len, &width, &height, &components, 4);
    if (img_data == NULL) {
        return luaL_error(L, "error loading image %s: %s", filename, stbi_failure_reason());
    }
    am_image_buffer *img = am_new_userdata(L, am_image_buffer);
    img->width = width;
    img->height = height;
    img->format = AM_PIXEL_FORMAT_RGBA8;
    int sz = width * height * pixel_format_size(img->format);
    am_buffer *imgbuf = am_new_userdata(L, am_buffer);
    imgbuf->size = sz;
    imgbuf->data = img_data;
    img->buffer = imgbuf;
    img->buffer_ref = img->ref(L, -1);
    lua_pop(L, 1); // pop imgbuf
    return 1;
}

static int save_image_as_png(lua_State *L) {
    am_check_nargs(L, 2);
    am_image_buffer *img = am_get_userdata(L, am_image_buffer, 1);
    const char *filename = luaL_checkstring(L, 2);
    size_t len;
    void *png_data = tdefl_write_image_to_png_file_in_memory_ex(
        img->buffer->data, img->width, img->height, 4, &len, MZ_DEFAULT_LEVEL, 1);
    FILE *f = fopen(filename, "wb");
    fwrite(png_data, len, 1, f);
    fclose(f);
    free(png_data);
    return 0;
}

static int encode_png(lua_State *L) {
    am_check_nargs(L, 1);
    am_image_buffer *img = am_get_userdata(L, am_image_buffer, 1);
    size_t len;
    void *png_data = tdefl_write_image_to_png_file_in_memory_ex(
        img->buffer->data, img->width, img->height, 4, &len, MZ_DEFAULT_LEVEL, 1);
    am_buffer *buf = am_new_userdata(L, am_buffer);
    buf->size = len;
    buf->data = (uint8_t*)png_data;
    return 1;
}

static int decode_png(lua_State *L) {
    am_check_nargs(L, 1);
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    int width, height;
    int components = 4;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc *img_data =
        stbi_load_from_memory((stbi_uc const *)buf->data, buf->size, &width, &height, &components, 4);
    if (img_data == NULL) {
        return luaL_error(L, "error decoding image %s: %s", buf->origin, stbi_failure_reason());
    }
    am_image_buffer *img = am_new_userdata(L, am_image_buffer);
    img->width = width;
    img->height = height;
    img->format = AM_PIXEL_FORMAT_RGBA8;
    int sz = width * height * pixel_format_size(img->format);
    am_buffer *imgbuf = am_new_userdata(L, am_buffer);
    imgbuf->size = sz;
    imgbuf->data = img_data;
    img->buffer = imgbuf;
    img->buffer_ref = img->ref(L, -1);
    lua_pop(L, 1); // pop imgbuf
    return 1;
}

void am_pixel_to_texture_format(am_pixel_format pxl_fmt, am_texture_format *txt_fmt, am_texture_type *txt_type) {
    switch (pxl_fmt) {
        case AM_PIXEL_FORMAT_RGBA8:
            *txt_fmt = AM_TEXTURE_FORMAT_RGBA;
            *txt_type = AM_TEXTURE_TYPE_UBYTE;
            break;
    }
}

static void get_image_width(lua_State *L, void *obj) {
    am_image_buffer *img = (am_image_buffer*)obj;
    lua_pushinteger(L, img->width);
}

static void get_image_height(lua_State *L, void *obj) {
    am_image_buffer *img = (am_image_buffer*)obj;
    lua_pushinteger(L, img->height);
}

static void get_image_buffer(lua_State *L, void *obj) {
    am_image_buffer *img = (am_image_buffer*)obj;
    img->pushref(L, img->buffer_ref);
}

static am_property image_width_property = {get_image_width, NULL};
static am_property image_height_property = {get_image_height, NULL};
static am_property image_buffer_property = {get_image_buffer, NULL};

static void register_image_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    am_register_property(L, "width", &image_width_property);
    am_register_property(L, "height", &image_height_property);
    am_register_property(L, "buffer", &image_buffer_property);

    am_register_metatable(L, "image_buffer", MT_am_image_buffer, 0);
}

void am_open_image_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"image_buffer", create_image_buffer},
        {"load_image", load_image},
        {"_load_embedded_image", load_embedded_image},
        {"save_image_as_png", save_image_as_png},
        {"encode_png", encode_png},
        {"decode_png", decode_png},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_image_mt(L);
}
