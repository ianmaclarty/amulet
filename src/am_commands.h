struct am_node;

struct am_command {
    virtual void execute(am_render_state *state) = 0;
};

struct am_set_float_param_command : am_command {
    am_param_name_id name;
    float value;

    am_set_float_param_command(lua_State *L, int nargs, am_node *node);
    virtual void execute(am_render_state *state);
};

struct am_mul_float_param_command : am_command {
    am_param_name_id name;
    float value;

    virtual void execute(am_render_state *state);
};

struct am_add_float_param_command : am_command {
    am_param_name_id name;
    float value;

    virtual void execute(am_render_state *state);
};

struct am_vertex_buffer {
    am_buffer_id buffer_id;
    int size;
};

struct am_set_float_array_command : am_command {
    am_param_name_id name;
    am_vertex_buffer *vbo;
    am_attribute_client_type type;
    bool normalized;
    int stride;
    int offset;

    virtual void execute(am_render_state *state);
};

struct am_draw_command : am_command {
    virtual void execute(am_render_state *state);
};
