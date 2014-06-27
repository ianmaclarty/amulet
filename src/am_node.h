struct am_child_slot {
    int ref;
    am_node *child;
};

struct am_node {
    am_lua_vector<am_command*> command_list;
    am_lua_vector<am_child_slot> children;
    int recursion_limit;

    void render(am_render_state *rstate);
};

void am_open_node_module(lua_State *L);
