#include "amulet.h"

static int pixel_format_size(am_pixel_format fmt) {
    switch (fmt) {
        case AM_PIXEL_FORMAT_RGBA8: return 4;
        case AM_PIXEL_FORMAT_LUM8: return 1;
    }
    return 0;
}

/******************************************************************************/

static int create_image(lua_State *L) {
    am_check_nargs(L, 2);
    am_image *img = am_new_userdata(L, am_image);
    img->width = luaL_checkinteger(L, 1);
    if (img->width <= 0) {
        return luaL_error(L, "expecting a positive integer in position 1");
    }
    img->height = luaL_checkinteger(L, 2);
    if (img->height <= 0) {
        return luaL_error(L, "expecting a position integer in position 2");
    }
    img->format = AM_PIXEL_FORMAT_RGBA8;
    img->buffer = am_new_userdata(L, am_buffer, img->width * img->height * 4);
    img->buffer_ref = img->ref(L, -1);
    lua_pop(L, 1); // pop buffer;
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
    am_image *img = am_new_userdata(L, am_image);
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
    am_image *img = am_new_userdata(L, am_image);
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
    am_image *img = am_get_userdata(L, am_image, 1);
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
    am_image *img = am_get_userdata(L, am_image, 1);
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
    am_image *img = am_new_userdata(L, am_image);
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

static void get_image_width(lua_State *L, void *obj) {
    am_image *img = (am_image*)obj;
    lua_pushinteger(L, img->width);
}

static void get_image_height(lua_State *L, void *obj) {
    am_image *img = (am_image*)obj;
    lua_pushinteger(L, img->height);
}

static void get_image_buffer(lua_State *L, void *obj) {
    am_image *img = (am_image*)obj;
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

    am_register_metatable(L, "image", MT_am_image, 0);
}

void am_open_image_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"create_image", create_image},
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
