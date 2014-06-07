struct am_command {
    int next_offset;
    int prev_offset;

    virtual void execute(am_render_state *state) = 0;

    am_command *next();
    am_command *prev();
};

struct am_command_list {
    int capacity; // in bytes
    int last;
    int end;

    am_command *first();
    void *extend(int size);

    char base[];
};

struct am_set_float_param_command : am_command {
    am_param_name_id name;
    float value;

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
