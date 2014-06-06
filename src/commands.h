struct am_command {
    virtual void execute(am_render_state *state) = 0;
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

struct am_draw_command : am_command {
    virtual void execute(am_render_state *state);
};
