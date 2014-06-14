extern am_program_param **am_param_name_map;

void am_init_param_name_map();
int am_lookup_param_name(lua_State *L, int name_idx);

struct am_program_param {
    virtual void bind(am_render_state *rstate) = 0;

    virtual void trailed_set_float(am_render_state *rstate, float val) {};
    virtual void trailed_set_float_array(am_render_state *rstate,
        am_buffer_id buffer_id, am_attribute_client_type type, bool normalized,
        int stride, int offset, int max_draw_elements) {};

    virtual void trailed_mul_float(am_render_state *rstate, float val) {};
    virtual void trailed_add_float(am_render_state *rstate, float val) {};
};
