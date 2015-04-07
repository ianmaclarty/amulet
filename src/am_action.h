struct am_scene_node;

struct am_action {
    int actions_ref;
    am_scene_node *node;
    int id_ref;
    int func_ref;
    int action_ref; // ref from node to action
    bool paused;

    am_action();
};

void am_prepare_to_execute_actions(lua_State *L, double dt);
bool am_execute_node_actions(lua_State *L, am_scene_node *node);
void am_init_actions(lua_State *L);
