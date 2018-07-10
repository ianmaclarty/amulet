#include "amulet.h"

void am_stencil_test_node::render(am_render_state *rstate) {
    am_stencil_test_state old_state = rstate->active_stencil_test_state;
    rstate->active_stencil_test_state.set(
        enabled,
        func_back,
        ref_back,
        read_mask_back,
        write_mask_back,
        func_front,
        ref_front,
        read_mask_front,
        write_mask_front,
        op_fail_front,
        op_zfail_front,
        op_zpass_front,
        op_fail_back,
        op_zfail_back,
        op_zpass_back);
    render_children(rstate);
    rstate->active_stencil_test_state.restore(&old_state);
}

static int create_stencil_test_node(lua_State *L) {
    am_check_nargs(L, 1);
    am_stencil_test_node *node = am_new_userdata(L, am_stencil_test_node);
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "expecting a table in position 1");
    }
    node->enabled = false;
    node->func_back = AM_STENCIL_FUNC_ALWAYS;
    node->ref_back = 0;
    node->read_mask_back = 255;
    node->write_mask_back = 255;
    node->func_front = AM_STENCIL_FUNC_ALWAYS;
    node->ref_front = 0;
    node->read_mask_front = 255;
    node->write_mask_front = 255;
    node->op_fail_front = AM_STENCIL_OP_KEEP;
    node->op_zfail_front = AM_STENCIL_OP_KEEP;
    node->op_zpass_front = AM_STENCIL_OP_KEEP;
    node->op_fail_back = AM_STENCIL_OP_KEEP;
    node->op_zfail_back = AM_STENCIL_OP_KEEP;
    node->op_zpass_back = AM_STENCIL_OP_KEEP;

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        const char *key = luaL_checkstring(L, -2);
        if (strcmp(key, "enabled") == 0) {
            node->enabled = lua_toboolean(L, -1);
        } else if (strcmp(key, "func_back") == 0) {
            node->func_back = am_get_enum(L, am_stencil_func, -1);
        } else if (strcmp(key, "ref_back") == 0) {
            node->ref_back = luaL_checkinteger(L, -1);
            if (node->ref_back < 0 || node->ref_back > 255) {
                return luaL_error(L, "invalid ref_back value %d", node->ref_back);
            }
        } else if (strcmp(key, "read_mask_back") == 0) {
            node->read_mask_back = (uint32_t)luaL_checkinteger(L, -1);
        } else if (strcmp(key, "write_mask_back") == 0) {
            node->write_mask_back = (uint32_t)luaL_checkinteger(L, -1);
        } else if (strcmp(key, "func_front") == 0) {
            node->func_front = am_get_enum(L, am_stencil_func, -1);
        } else if (strcmp(key, "ref_front") == 0) {
            node->ref_front = luaL_checkinteger(L, -1);
            if (node->ref_front < 0 || node->ref_front > 255) {
                return luaL_error(L, "invalid ref_front value %d", node->ref_front);
            }
        } else if (strcmp(key, "read_mask_front") == 0) {
            node->read_mask_front = (uint32_t)luaL_checkinteger(L, -1);
        } else if (strcmp(key, "write_mask_front") == 0) {
            node->write_mask_front = (uint32_t)luaL_checkinteger(L, -1);
        } else if (strcmp(key, "op_fail_front") == 0) {
            node->op_fail_front = am_get_enum(L, am_stencil_op, -1);
        } else if (strcmp(key, "op_zfail_front") == 0) {
            node->op_zfail_front = am_get_enum(L, am_stencil_op, -1);
        } else if (strcmp(key, "op_zpass_front") == 0) {
            node->op_zpass_front = am_get_enum(L, am_stencil_op, -1);
        } else if (strcmp(key, "op_fail_back") == 0) {
            node->op_fail_back = am_get_enum(L, am_stencil_op, -1);
        } else if (strcmp(key, "op_zfail_back") == 0) {
            node->op_zfail_back = am_get_enum(L, am_stencil_op, -1);
        } else if (strcmp(key, "op_zpass_back") == 0) {
            node->op_zpass_back = am_get_enum(L, am_stencil_op, -1);
        } else {
            return luaL_error(L, "unrecognised stencil setting: '%s'", key);
        }
        lua_pop(L, 1); // pop value
    }
    return 1;
}

static void get_enabled(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    lua_pushboolean(L, node->enabled);
}

static void set_enabled(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->enabled = lua_toboolean(L, 3);
}

static am_property enabled_property = {get_enabled, set_enabled};

static void get_func_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    am_push_enum(L, am_stencil_func, node->func_back);
}

static void set_func_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->func_back = am_get_enum(L, am_stencil_func, 3);
}

static am_property func_back_property = {get_func_back, set_func_back};

static void get_ref_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    lua_pushinteger(L, node->ref_back);
}

static void set_ref_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->ref_back = lua_tointeger(L, 3);
}

static am_property ref_back_property = {get_ref_back, set_ref_back};

static void get_read_mask_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    lua_pushinteger(L, node->read_mask_back);
}

static void set_read_mask_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->read_mask_back = lua_tointeger(L, 3);
}

static am_property read_mask_back_property = {get_read_mask_back, set_read_mask_back};

static void get_write_mask_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    lua_pushinteger(L, node->write_mask_back);
}

static void set_write_mask_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->write_mask_back = lua_tointeger(L, 3);
}

static am_property write_mask_back_property = {get_write_mask_back, set_write_mask_back};

static void get_op_fail_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    am_push_enum(L, am_stencil_op, node->op_fail_back);
}

static void set_op_fail_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->op_fail_back = am_get_enum(L, am_stencil_op, 3);
}

static am_property op_fail_back_property = {get_op_fail_back, set_op_fail_back};

static void get_op_zfail_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    am_push_enum(L, am_stencil_op, node->op_zfail_back);
}

static void set_op_zfail_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->op_zfail_back = am_get_enum(L, am_stencil_op, 3);
}

static am_property op_zfail_back_property = {get_op_zfail_back, set_op_zfail_back};

static void get_op_zpass_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    am_push_enum(L, am_stencil_op, node->op_zpass_back);
}

static void set_op_zpass_back(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->op_zpass_back = am_get_enum(L, am_stencil_op, 3);
}

static am_property op_zpass_back_property = {get_op_zpass_back, set_op_zpass_back};

static void get_func_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    am_push_enum(L, am_stencil_func, node->func_front);
}

static void set_func_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->func_front = am_get_enum(L, am_stencil_func, 3);
}

static am_property func_front_property = {get_func_front, set_func_front};

static void get_ref_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    lua_pushinteger(L, node->ref_front);
}

static void set_ref_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->ref_front = lua_tointeger(L, 3);
}

static am_property ref_front_property = {get_ref_front, set_ref_front};

static void get_read_mask_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    lua_pushinteger(L, node->read_mask_front);
}

static void set_read_mask_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->read_mask_front = lua_tointeger(L, 3);
}

static am_property read_mask_front_property = {get_read_mask_front, set_read_mask_front};

static void get_write_mask_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    lua_pushinteger(L, node->write_mask_front);
}

static void set_write_mask_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->write_mask_front = lua_tointeger(L, 3);
}

static am_property write_mask_front_property = {get_write_mask_front, set_write_mask_front};

static void get_op_fail_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    am_push_enum(L, am_stencil_op, node->op_fail_front);
}

static void set_op_fail_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->op_fail_front = am_get_enum(L, am_stencil_op, 3);
}

static am_property op_fail_front_property = {get_op_fail_front, set_op_fail_front};

static void get_op_zfail_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    am_push_enum(L, am_stencil_op, node->op_zfail_front);
}

static void set_op_zfail_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->op_zfail_front = am_get_enum(L, am_stencil_op, 3);
}

static am_property op_zfail_front_property = {get_op_zfail_front, set_op_zfail_front};

static void get_op_zpass_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    am_push_enum(L, am_stencil_op, node->op_zpass_front);
}

static void set_op_zpass_front(lua_State *L, void *obj) {
    am_stencil_test_node *node = (am_stencil_test_node*)obj;
    node->op_zpass_front = am_get_enum(L, am_stencil_op, 3);
}

static am_property op_zpass_front_property = {get_op_zpass_front, set_op_zpass_front};

static void register_stencil_test_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "enabled", &enabled_property);
    am_register_property(L, "func_back", &func_back_property);
    am_register_property(L, "ref_back", &ref_back_property);
    am_register_property(L, "read_mask_back", &read_mask_back_property);
    am_register_property(L, "write_mask_back", &write_mask_back_property);
    am_register_property(L, "func_front", &func_front_property);
    am_register_property(L, "ref_front", &ref_front_property);
    am_register_property(L, "read_mask_front", &read_mask_front_property);
    am_register_property(L, "write_mask_front", &write_mask_front_property);
    am_register_property(L, "op_fail_front", &op_fail_front_property);
    am_register_property(L, "op_zfail_front", &op_zfail_front_property);
    am_register_property(L, "op_zpass_front", &op_zpass_front_property);
    am_register_property(L, "op_fail_back", &op_fail_back_property);
    am_register_property(L, "op_zfail_back", &op_zfail_back_property);
    am_register_property(L, "op_zpass_back", &op_zpass_back_property);

    am_register_metatable(L, "stencil_test", MT_am_stencil_test_node, MT_am_scene_node);
}

void am_open_stencilbuffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"stencil_test", create_stencil_test_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);

    am_enum_value stencil_func_enum[] = {
        {"never", AM_STENCIL_FUNC_NEVER},
        {"always", AM_STENCIL_FUNC_ALWAYS},
        {"equal", AM_STENCIL_FUNC_EQUAL},
        {"notequal", AM_STENCIL_FUNC_NOTEQUAL},
        {"less", AM_STENCIL_FUNC_LESS},
        {"lequal", AM_STENCIL_FUNC_LEQUAL},
        {"greater",AM_STENCIL_FUNC_GREATER},
        {"gequal", AM_STENCIL_FUNC_GEQUAL},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_stencil_func, stencil_func_enum);

    am_enum_value stencil_op_enum[] = {
        {"keep", AM_STENCIL_OP_KEEP},
        {"zero", AM_STENCIL_OP_ZERO},
        {"replace", AM_STENCIL_OP_REPLACE},
        {"incr", AM_STENCIL_OP_INCR},
        {"decr", AM_STENCIL_OP_DECR},
        {"invert", AM_STENCIL_OP_INVERT},
        {"incr_wrap", AM_STENCIL_OP_INCR_WRAP},
        {"decr_wrap", AM_STENCIL_OP_DECR_WRAP},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_stencil_op, stencil_op_enum);

    register_stencil_test_node_mt(L);
}
