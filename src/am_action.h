struct am_action {
    am_action *gnext;
    am_action *gprev;
    am_action *nnext;
    am_node *node;
    int tag_ref;
    int func_ref;
    int action_ref;
    int priority;
    uint64_t seq;
    bool paused;

    am_action();
};

void am_schedule_action(am_action *action);
void am_deschedule_action(am_action *action);
void am_execute_actions(lua_State *L);
void am_init_actions();
