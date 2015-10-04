struct am_framebuffer : am_nonatomic_userdata {
    am_framebuffer_id       framebuffer_id;
    int                     width;
    int                     height;
    am_texture2d            *color_attachment0;
    int                     color_attachment0_ref;
    am_renderbuffer_id      depth_renderbuffer_id;
    glm::vec4               clear_color;
    glm::mat4               projection;
    bool                    user_projection; // was the projection set by the user?

    void init(lua_State *L, am_texture2d *color_at, bool depth_buf, glm::vec4 clear_color);
    void destroy(lua_State *L);
};

void am_open_framebuffer_module(lua_State *L);
