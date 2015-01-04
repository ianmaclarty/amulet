#include "amulet.h"

void am_depth_pass_node::render(am_render_state *rstate) {
    am_depth_test_state old_state = rstate->depth_test_state;
    rstate->depth_test_state.set(func != AM_DEPTH_FUNC_ALWAYS, func);
    render_children(rstate);
    rstate->depth_test_state.restore(&old_state);
}

int am_create_depth_pass_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_depth_pass_node *node = am_new_userdata(L, am_depth_pass_node);
    am_set_scene_node_child(L, node);
    node->func = am_get_enum(L, am_depth_func, 2);
    return 1;
}

static void register_depth_pass_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "depth_pass");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_depth_pass_node, MT_am_scene_node);
}

void am_open_depthbuffer_module(lua_State *L) {
    am_enum_value depth_func_enum[] = {
        {"never", AM_DEPTH_FUNC_NEVER},
        {"always", AM_DEPTH_FUNC_ALWAYS},
        {"equal", AM_DEPTH_FUNC_EQUAL},
        {"notequal", AM_DEPTH_FUNC_NOTEQUAL},
        {"less", AM_DEPTH_FUNC_LESS},
        {"lequal", AM_DEPTH_FUNC_LEQUAL},
        {"greater", AM_DEPTH_FUNC_GREATER},
        {"gequal", AM_DEPTH_FUNC_GEQUAL},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_depth_func, depth_func_enum);

    register_depth_pass_node_mt(L);
}
