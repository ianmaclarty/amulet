#include "amulet.h"

static int pixel_format_size(am_pixel_format fmt) {
    switch (fmt) {
        case AM_PIXEL_FORMAT_RGBA8: return 4;
    }
    return 0;
}


/******************************* PNG decoding ********************************/

struct png_read_state {
    uint8_t *data;
    unsigned int bytes_left;
    unsigned int size;
};

static void png_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    png_read_state *state = (png_read_state*)png_get_io_ptr(png_ptr);
    am_log0("error while decoding png at byte %d", state->size - state->bytes_left);
    longjmp(png_jmpbuf(png_ptr), 1);
}

static void png_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
    am_log0("warning while decoding png: %s", warning_msg);
}

static void png_read_fn(png_structp png_ptr, png_bytep data, png_size_t length) {
    png_read_state *state = (png_read_state*)png_get_io_ptr(png_ptr);
    if (length > state->bytes_left) {
        // abort
        am_log0("attempt to read beyond end of buffer by %d bytes", (length - state->bytes_left));
        longjmp(png_jmpbuf(png_ptr), 1);
    }
    memcpy(data, state->data, length);
    state->data += length;
    state->bytes_left -= length;
}

static uint8_t* read_png(uint8_t *data, int sz, int *width, int *height) {
    if (sz < 8 || !png_check_sig(data, 8)) {
        am_log0("%s", "png has invalid signature");
        return NULL;
    }
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_infop end_ptr = png_create_info_struct(png_ptr);

    png_set_error_fn(png_ptr, NULL, png_error_fn, png_warning_fn);

    if (setjmp(png_jmpbuf(png_ptr))) {
        // we'll end up here if there was an error reading the png.
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
        return NULL;
    }

    png_read_state state;
    state.data = data+8;
    state.bytes_left = sz-8;
    state.size = sz;
    png_set_read_fn(png_ptr, &state, png_read_fn);
    png_set_sig_bytes(png_ptr, 8);

    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

    int png_transforms = 
        PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
        PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_EXPAND;
    png_read_png(png_ptr, info_ptr, png_transforms, NULL);

    png_uint_32 uwidth;
    png_uint_32 uheight;
    int bit_depth;
    int color_type;
    png_get_IHDR(png_ptr, info_ptr, &uwidth, &uheight, &bit_depth, &color_type,
        NULL, NULL, NULL);
    *width = (int)uwidth;
    *height = (int)uheight;
    if (bit_depth != 8) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
        return NULL;
    }

    png_byte **rows = png_get_rows(png_ptr, info_ptr);
    uint8_t *img_data = (uint8_t*)malloc(uwidth * uheight * 4);
    uint8_t *ptr = img_data;
    for (int row = *height - 1; row >= 0; row--) {
        memcpy(ptr, &rows[row][0], uwidth * 4);
        ptr += uwidth * 4;
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
    return img_data;
}

/******************************* JPEG decoding ********************************/

static uint8_t* read_jpg(uint8_t *data, int sz, int *width, int *height) {
    int subsamp;
    int colorspace;
    tjhandle tj = tjInitDecompress();
    if (tjDecompressHeader3(tj, data, sz, width, height, &subsamp, &colorspace) < 0) {
        tjDestroy(tj);
        am_debug("failed to read header: %s", tjGetErrorStr());
        return NULL;
    }
    int w = *width;
    int h = *height;
    uint8_t* img_data = (uint8_t*)malloc(w * h * 4);
    if (tjDecompress2(tj, data, sz, img_data, w, w * 4, h, TJPF_RGBA, TJFLAG_BOTTOMUP) < 0) {
        tjDestroy(tj);
        free(img_data);
        am_debug("failed to read body: %s", tjGetErrorStr());
        return NULL;
    }
    tjDestroy(tj);
    return img_data;
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

static int decode_png(lua_State *L) {
    am_check_nargs(L, 1);
    am_buffer *rawbuf = am_get_userdata(L, am_buffer, 1);
    int width, height;
    uint8_t *img_data = read_png(rawbuf->data, rawbuf->size, &width, &height);
    if (img_data == NULL) {
        return luaL_error(L, "error decoding png '%s'", rawbuf->origin);
    }
    am_image *img = am_new_userdata(L, am_image);
    img->width = width;
    img->height = height;
    img->format = AM_PIXEL_FORMAT_RGBA8;
    am_buffer *imgbuf = am_new_userdata(L, am_buffer);
    imgbuf->data = img_data;
    imgbuf->size = width * height * pixel_format_size(img->format);
    img->buffer = imgbuf;
    img->buffer_ref = img->ref(L, -1);
    lua_pop(L, 1); // pop imgbuf
    return 1;
}

static int decode_jpg(lua_State *L) {
    am_check_nargs(L, 1);
    am_buffer *rawbuf = am_get_userdata(L, am_buffer, 1);
    int width, height;
    uint8_t *img_data = read_jpg(rawbuf->data, rawbuf->size, &width, &height);
    if (img_data == NULL) {
        return luaL_error(L, "error decoding jpg '%s'", rawbuf->origin);
    }
    am_image *img = am_new_userdata(L, am_image);
    img->width = width;
    img->height = height;
    img->format = AM_PIXEL_FORMAT_RGBA8;
    am_buffer *imgbuf = am_new_userdata(L, am_buffer);
    imgbuf->data = img_data;
    imgbuf->size = width * height * pixel_format_size(img->format);
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
        {"decode_png", decode_png},
        {"decode_jpg", decode_jpg},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_image_mt(L);
}
