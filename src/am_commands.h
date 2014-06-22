struct am_node;

struct am_command {
    virtual void execute(am_render_state *state) = 0;
};

struct am_use_program_command : am_command {
    am_program *program;
    int program_ref;
    virtual void execute(am_render_state *state);
};

struct am_draw_arrays_command : am_command {
    int first;
    int count;
    am_draw_arrays_command(int first, int count);
    virtual void execute(am_render_state *state);
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

struct am_set_float_array_command : am_command {
    am_param_name_id name;
    am_vertex_buffer *vbo;
    int vbo_ref;
    am_attribute_client_type type;
    int size;
    bool normalized;
    int stride;
    int offset;

    am_set_float_array_command(lua_State *L, int nargs, am_node *node);
    virtual void execute(am_render_state *state);
};
