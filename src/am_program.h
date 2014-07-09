typedef int am_param_name_id;

struct am_program_param_name_info {
    am_program_param *param;
    const char *name;
};

extern am_program_param_name_info *am_param_name_map;

void am_init_param_name_map(lua_State *L);
am_param_name_id am_lookup_param_name(lua_State *L, int name_idx);

struct am_program_param {
    am_param_name_id name;

    am_program_param(am_param_name_id name);

    virtual void bind(am_render_state *rstate) = 0;

    virtual void trailed_set_float(am_render_state *rstate, float val);
    virtual void trailed_set_array(am_render_state *rstate,
        am_vertex_buffer *vbo, am_attribute_client_type type, int dimensions,
        bool normalized, int stride, int offset, int max_draw_elements);
};

struct am_program {
    am_program_id program_id;
    int num_params;
    am_program_param **params;
};

void am_open_program_module(lua_State *L);
