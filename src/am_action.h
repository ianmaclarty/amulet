struct am_scene_node;

struct am_action {
    am_action *gnext; // global list next
    am_action *gprev; // global list prev
    am_action *nnext; // scene node list next
    am_scene_node *node;
    int tag_ref;
    int func_ref;
    int action_ref; // ref from node to action
    int priority;
    uint64_t seq;
    bool paused;

    am_action();
};

void am_schedule_action(am_action *action);
void am_deschedule_action(am_action *action);
bool am_execute_actions(lua_State *L);
void am_init_actions();
