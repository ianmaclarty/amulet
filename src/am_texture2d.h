struct am_texture2d : am_nonatomic_userdata {
    am_texture_id           texture_id;
    int                     width;
    int                     height;
    am_texture_format       format;
    am_texture_type         type;
    int                     pixel_size;
    bool                    has_mipmap;
    am_image_buffer         *image_buffer; // can be NULL
    int                     image_buffer_ref;
    int                     last_video_capture_frame;
    am_texture_min_filter   minfilter;
    am_texture_mag_filter   magfilter;
    am_texture_wrap         swrap;
    am_texture_wrap         twrap;

    void update_dirty();
};

void am_open_texture2d_module(lua_State *L);
