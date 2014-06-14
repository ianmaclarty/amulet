struct am_node {
    am_command **command_list;
    int command_list_capacity;
    int command_list_size;
    int command_list_ref;
    int recursion_limit;

    void render(am_render_state *rstate);
};

void am_open_node_module(lua_State *L);
