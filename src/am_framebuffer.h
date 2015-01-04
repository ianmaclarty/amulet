struct am_framebuffer : am_nonatomic_userdata {
    am_framebuffer_id       framebuffer_id;
    int                     width;
    int                     height;
    am_texture2d            *color_attachment0;
    int                     color_attachment0_ref;
    am_renderbuffer_id      depth_renderbuffer_id;
};

void am_open_framebuffer_module(lua_State *L);
