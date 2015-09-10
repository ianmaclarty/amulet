#define AM_NODE_FLAG_MARK        ((uint32_t)1)
#define AM_NODE_FLAG_HIDDEN      ((uint32_t)2)

#define am_node_marked(node)        (node->flags & AM_NODE_FLAG_MARK)
#define am_mark_node(node)          node->flags |= AM_NODE_FLAG_MARK
#define am_unmark_node(node)        node->flags &= ~AM_NODE_FLAG_MARK
#define am_node_hidden(node)        (node->flags & AM_NODE_FLAG_HIDDEN)
#define am_set_node_hidden(node, hidden) \
    {if (hidden) (node)->flags |= AM_NODE_FLAG_HIDDEN; else (node)->flags &= ~AM_NODE_FLAG_HIDDEN; }

struct am_scene_node;

struct am_node_child {
    int ref;
    am_scene_node *child;
};

struct am_render_state;

#define AM_MAX_TAG UINT16_MAX
typedef uint16_t am_tag;

extern am_tag AM_TAG_GROUP;
extern am_tag AM_TAG_BIND;
extern am_tag AM_TAG_USE_PROGRAM;
extern am_tag AM_TAG_TRANSLATE;
extern am_tag AM_TAG_ROTATE;
extern am_tag AM_TAG_SCALE;
extern am_tag AM_TAG_BILLBOARD;
extern am_tag AM_TAG_LOOKAT;
extern am_tag AM_TAG_BLEND;

struct am_scene_node : am_nonatomic_userdata {
    am_lua_array<am_node_child> children;
    am_lua_array<am_tag> tags;
    int recursion_limit;
    uint32_t flags;
    int actions_ref;
    unsigned int action_seq; // used to avoid duplicating action in lua action list

    am_scene_node();
    virtual void render(am_render_state *rstate);
    void render_children(am_render_state *rstate);
};

int am_scene_node_index(lua_State *L);
int am_scene_node_newindex(lua_State *L);

void am_open_scene_module(lua_State *L);
