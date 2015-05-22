#include "amulet.h"

static am_blend_state blend_mode_map[] = {
    // AM_BLEND_MODE_OFF:
    {false,
        AM_BLEND_EQUATION_ADD, AM_BLEND_EQUATION_ADD,
        AM_BLEND_SFACTOR_ZERO, AM_BLEND_DFACTOR_ZERO,
        AM_BLEND_SFACTOR_ZERO, AM_BLEND_DFACTOR_ZERO,
        1.0f, 1.0f, 1.0f, 1.0f},
    // AM_BLEND_MODE_NORMAL:
    {true,
        AM_BLEND_EQUATION_ADD, AM_BLEND_EQUATION_ADD,
        AM_BLEND_SFACTOR_SRC_ALPHA, AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA,
        AM_BLEND_SFACTOR_SRC_ALPHA, AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA,
        1.0f, 1.0f, 1.0f, 1.0f},
    // AM_BLEND_MODE_ADD:
    {true,
        AM_BLEND_EQUATION_ADD, AM_BLEND_EQUATION_ADD,
        AM_BLEND_SFACTOR_ONE, AM_BLEND_DFACTOR_ONE,
        AM_BLEND_SFACTOR_ONE, AM_BLEND_DFACTOR_ONE,
        1.0f, 1.0f, 1.0f, 1.0f},
};

ct_check_array_size(blend_mode_map, AM_NUM_BLEND_MODES);

void am_blend_node::render(am_render_state *rstate) {
    am_blend_state old = rstate->active_blend_state;    
    rstate->active_blend_state = blend_mode_map[mode];
    render_children(rstate);
    rstate->active_blend_state = old;
}

int am_create_blend_node(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_blend_node *node = am_new_userdata(L, am_blend_node);
    am_set_scene_node_child(L, node);
    node->mode = am_get_enum(L, am_blend_mode, 2);
    return 1;
}

static void get_blend_mode(lua_State *L, void *obj) {
    am_blend_node *node = (am_blend_node*)obj;
    am_push_enum(L, am_blend_mode, node->mode);
}

static void set_blend_mode(lua_State *L, void *obj) {
    am_blend_node *node = (am_blend_node*)obj;
    node->mode = am_get_enum(L, am_blend_mode, 3);
}

static am_property blend_mode_property = {get_blend_mode, set_blend_mode};

static void register_blend_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "mode", &blend_mode_property);

    am_register_metatable(L, "blend", MT_am_blend_node, MT_am_scene_node);
}

// Module init

void am_open_blending_module(lua_State *L) {
    am_enum_value blend_mode_enum[] = {
        {"off", AM_BLEND_MODE_OFF},
        {"normal", AM_BLEND_MODE_NORMAL},
        {"add", AM_BLEND_MODE_ADD},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_blend_mode, blend_mode_enum);
    register_blend_node_mt(L);
}
