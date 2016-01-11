enum am_pixel_format {
    AM_PIXEL_FORMAT_RGBA8,
};

struct am_image_buffer : am_nonatomic_userdata {
    int width;
    int height;
    am_pixel_format format;
    am_buffer *buffer;
    int buffer_ref;

    am_image_buffer();
};

void am_pixel_to_texture_format(am_pixel_format pxl_fmt, am_texture_format *txt_fmt, am_texture_type *txt_type);

int am_load_image(lua_State *L);

void am_open_image_module(lua_State *L);
