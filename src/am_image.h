enum am_pixel_format {
    AM_PIXEL_FORMAT_RGBA8,
};

struct am_image : am_nonatomic_userdata {
    int width;
    int height;
    am_pixel_format format;
    am_buffer *buffer;
    int buffer_ref;
};

void am_open_image_module(lua_State *L);
