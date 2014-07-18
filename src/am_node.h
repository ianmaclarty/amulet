#define AM_NODE_FLAG_LIVE        0x00000001
#define AM_NODE_FLAG_MARK        0x00000002

struct am_action;

struct am_child_slot {
    int ref;
    am_node *child;
};

struct am_parent_slot {
    int ref;
    am_node *parent;
};

struct am_node {
    am_lua_vector<am_command*> command_list;
    am_action *action_list;
    am_lua_vector<am_child_slot> children;
    am_lua_vector<am_parent_slot> parents;
    int recursion_limit;
    uint32_t flags;

    am_node();
    void render(am_render_state *rstate);
};

void am_open_node_module(lua_State *L);

extern am_node am_root_node;
