struct am_scene_node;

void am_pre_frame(lua_State *L, double dt);
void am_post_frame(lua_State *L);
bool am_execute_node_actions(lua_State *L, am_scene_node *node);
void am_open_actions_module(lua_State *L);
