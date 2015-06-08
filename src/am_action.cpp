#include "amulet.h"

static int num_actions = 0;
static uint32_t action_seq = 1;

static void add_actions(lua_State *L, am_scene_node *node, int actions_tbl) {
    if (am_node_marked(node)) return;
    am_mark_node(node);
    if (node->action_seq != action_seq && node->actions_ref != LUA_NOREF) {
        node->action_seq = action_seq;
        node->pushref(L, node->actions_ref);
        assert(lua_istable(L, -1));
        int i = 1;
        while (true) {
            lua_rawgeti(L, -1, i);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                break;
            }
            assert(lua_istable(L, -1));
            num_actions++;
            lua_rawseti(L, actions_tbl, num_actions);
            i++;
        }
        lua_pop(L, 1); // action list
    }
    for (int i = 0; i < node->children.size; i++) {
        add_actions(L, node->children.arr[i].child, actions_tbl);
    }
    am_unmark_node(node);
}

void am_pre_execute_actions(lua_State *L, double dt) {
    am_update_times(L, dt);
    action_seq++;
    am_call_amulet(L, "_update_action_seq", 0, 0);
    num_actions = 0;
}

void am_post_execute_actions(lua_State *L) {
}

bool am_execute_node_actions(lua_State *L, am_scene_node *node) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_ACTION_TABLE);
    int actions_tbl = am_absindex(L, -1);
    int from = num_actions + 1;
    add_actions(L, node, actions_tbl);
    int to = num_actions;
    lua_pushinteger(L, from);
    lua_pushinteger(L, to);
    return am_call_amulet(L, "_execute_actions", 3, 0);
}

void am_init_actions(lua_State *L) {
    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_ACTION_TABLE);
}
