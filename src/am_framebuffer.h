struct am_framebuffer : am_nonatomic_userdata {
    am_framebuffer_id       framebuffer_id;
    int                     width;
    int                     height;
    am_texture2d            *color_attachment0;
    int                     color_attachment0_ref;
    am_renderbuffer_id      depth_renderbuffer_id;
    am_renderbuffer_id      stencil_renderbuffer_id;
    am_renderbuffer_id      depthstencil_renderbuffer_id;
    bool                    has_depth_buf;
    bool                    has_stencil_buf;
    glm::dvec4              clear_color;
    int                     stencil_clear_value;
    glm::dmat4              projection;
    bool                    user_projection; // was the projection set by the user?

    void init(lua_State *L, am_texture2d *tex, int tex_idx, 
        bool depth_buf, bool stencil_buf, glm::dvec4 clear_color, int stencil_clear_value);
    void clear(bool clear_colorbuf, bool clear_depth, bool clear_stencil);
    void destroy(lua_State *L);
};

struct am_viewport_node : am_scene_node {
    int                     x;
    int                     y;
    int                     w;
    int                     h;

    virtual void render(am_render_state *rstate);
};

struct am_color_mask_node : am_scene_node {
    bool                    r;
    bool                    g;
    bool                    b;
    bool                    a;

    virtual void render(am_render_state *rstate);
};

void am_open_framebuffer_module(lua_State *L);
