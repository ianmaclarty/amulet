struct am_framebuffer : am_nonatomic_userdata {
    am_framebuffer_id       fbo;
    int                     width;
    int                     height;
    int                     color_attachment0_ref;
};

void am_open_framebuffer_module(lua_State *L);
