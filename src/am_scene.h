struct am_scene_node;

struct am_node_child {
    int ref;
    am_scene_node *child;
};

struct am_scene_node : am_nonatomic_userdata {
    am_action *action_list;
    am_lua_array<am_scene_node*> live_parents;
    am_lua_array<am_node_child> children;
    int recursion_limit;
    uint32_t flags;
    int root_count;

    am_scene_node();
    virtual void render(am_render_state *rstate);
    void render_children(am_render_state *rstate);
    void activate();
    void deactivate();
    void activate_root(lua_State *L);
    void deactivate_root();
};

struct am_program_node : am_scene_node {
    am_program *program;
    int program_ref;

    virtual void render(am_render_state *rstate);
};

struct am_draw_arrays_node : am_scene_node {
    int first;
    int count;

    virtual void render(am_render_state *rstate);
};

struct am_bind_array_node : am_scene_node {
    am_param_name_id name;
    am_buffer_view *arr;
    int arr_ref;

    virtual void render(am_render_state *rstate);
};

#define AM_BIND_MAT_NODE_DECL(D)                                        \
struct am_bind_mat##D##_node : am_scene_node {                          \
    am_param_name_id name;                                              \
    glm::mat##D m;                                                      \
    virtual void render(am_render_state *rstate);                       \
};

AM_BIND_MAT_NODE_DECL(2)
AM_BIND_MAT_NODE_DECL(3)
AM_BIND_MAT_NODE_DECL(4)

void am_open_scene_module(lua_State *L);
