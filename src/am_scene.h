#define AM_NODE_FLAG_MARK        ((uint32_t)2)

#define am_node_marked(node)        (node->flags & AM_NODE_FLAG_MARK)
#define am_mark_node(node)          node->flags |= AM_NODE_FLAG_MARK
#define am_unmark_node(node)        node->flags &= ~AM_NODE_FLAG_MARK

struct am_scene_node;

struct am_node_child {
    int ref;
    am_scene_node *child;
};

struct am_render_state;

struct am_scene_node : am_nonatomic_userdata {
    am_lua_array<am_node_child> children;
    int recursion_limit;
    uint32_t flags;
    int actions_ref;
    uint32_t action_seq;

    am_scene_node();
    virtual void render(am_render_state *rstate);
    virtual int specialized_index(lua_State *L);
    virtual int specialized_newindex(lua_State *L); // return -1 for failure
    void render_children(am_render_state *rstate);
};

void am_set_scene_node_child(lua_State *L, am_scene_node *parent);

int am_scene_node_index(lua_State *L);
int am_scene_node_newindex(lua_State *L);

void am_open_scene_module(lua_State *L);
