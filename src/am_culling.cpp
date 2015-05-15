#include "amulet.h"

void am_cull_face_node::render(am_render_state *rstate) {
    am_cull_face_state old_state = rstate->active_cull_face_state;
    switch (mode) {
        case AM_CULL_FACE_MODE_FRONT:
            rstate->active_cull_face_state.set(true, AM_FACE_WIND_CCW, AM_CULL_FACE_FRONT);
            break;
        case AM_CULL_FACE_MODE_BACK:
            rstate->active_cull_face_state.set(true, AM_FACE_WIND_CCW, AM_CULL_FACE_BACK);
            break;
        case AM_CULL_FACE_MODE_NONE:
            rstate->active_cull_face_state.set(false, AM_FACE_WIND_CCW, AM_CULL_FACE_BACK);
            break;
    }
    render_children(rstate);
    rstate->active_cull_face_state.restore(&old_state);
}

int am_create_cull_face_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_cull_face_node *node = am_new_userdata(L, am_cull_face_node);
    am_set_scene_node_child(L, node);
    node->mode = am_get_enum(L, am_cull_face_mode, 2);
    return 1;
}

static void register_cull_face_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_metatable(L, "cull_face", MT_am_cull_face_node, MT_am_scene_node);
}

void am_open_culling_module(lua_State *L) {
    am_enum_value cull_face_enum[] = {
        {"front", AM_CULL_FACE_MODE_FRONT},
        {"cw", AM_CULL_FACE_MODE_FRONT},
        {"back", AM_CULL_FACE_MODE_BACK},
        {"ccw", AM_CULL_FACE_MODE_BACK},
        {"none", AM_CULL_FACE_MODE_NONE},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_cull_face_mode, cull_face_enum);
    register_cull_face_node_mt(L);
}
