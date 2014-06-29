#include "amulet.h"

am_node::am_node() {
    recursion_limit = am_conf_default_recursion_limit;
}

void am_node::render(am_render_state *rstate) {
    if (recursion_limit <= 0) return;
    recursion_limit--;
    int ticket = rstate->trail.get_ticket();
    for (int i = 0; i < command_list.size; i++) {
        am_command *cmd = command_list.arr[i];
        cmd->execute(this, rstate);
    }
    rstate->trail.rewind_to(ticket);
    recursion_limit++;
}

static int create_node(lua_State *L) {
    new (am_new_nonatomic_userdata(L, sizeof(am_node))) am_node();
    am_set_metatable(L, AM_MT_NODE, -1);
    return 1;
}

static int append_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_node *child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    am_child_slot slot;
    slot.child = child;
    slot.ref = am_new_ref(L, 1, 2);
    node->children.push_back(L, 1, slot);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int prepend_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_node *child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    am_child_slot slot;
    slot.child = child;
    slot.ref = am_new_ref(L, 1, 2);
    node->children.push_front(L, 1, slot);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_node *child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    for (int i = 0; i < node->children.size; i++) {
        if (node->children.arr[i].child == child) {
            am_delete_ref(L, 1, node->children.arr[i].ref);
            node->children.remove(i);
            break;
        }
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int replace_child(lua_State *L) {
    am_check_nargs(L, 3);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_node *old_child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    am_node *new_child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 3);
    for (int i = 0; i < node->children.size; i++) {
        if (node->children.arr[i].child == old_child) {
            am_replace_ref(L, 1, node->children.arr[i].ref, 3);
            node->children.arr[i].child = new_child;
            break;
        }
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_all_children(lua_State *L) {
    am_check_nargs(L, 1);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    for (int i = 0; i < node->children.size; i++) {
        am_delete_ref(L, 1, node->children.arr[i].ref);
    }
    node->children.clear();
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static void append_command(lua_State *L, am_node *node, int node_idx, am_command *cmd, int cmd_idx) {
    node->command_list.push_back(L, node_idx, cmd);
    am_new_ref(L, node_idx, cmd_idx);
}

static int fallback_index_func(lua_State *L) {
    lua_pushnil(L);
    return 1;
}

static int draw_children_command(lua_State *L) {
    am_check_nargs(L, 1);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_draw_children_command *cmd = new (lua_newuserdata(L, sizeof(am_draw_children_command))) am_draw_children_command();
    append_command(L, node, 1, cmd, -1);
    lua_pop(L, 1); // cmd
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int set_float_command(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_set_float_param_command *cmd = new (lua_newuserdata(L, sizeof(am_set_float_param_command))) am_set_float_param_command(L, nargs, node);
    append_command(L, node, 1, cmd, -1);
    lua_pop(L, 1); // cmd
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int set_float_array_command(lua_State *L) {
    int nargs = am_check_nargs(L, 3);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_set_float_array_command *cmd = new (lua_newuserdata(L, sizeof(am_set_float_array_command))) am_set_float_array_command(L, nargs, node);
    append_command(L, node, 1, cmd, -1);
    lua_pop(L, 1); // cmd
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int use_program_command(lua_State *L) {
    am_check_nargs(L, 2);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_program *prog = (am_program*)am_check_metatable_id(L, AM_MT_PROGRAM, 2);
    am_use_program_command *cmd = new (lua_newuserdata(L, sizeof(am_use_program_command))) am_use_program_command();
    cmd->program = prog;
    cmd->program_ref = am_new_ref(L, 1, 2); // add ref from node to prog.
    append_command(L, node, 1, cmd, -1);
    lua_pop(L, 1); // cmd
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int draw_arrays_command(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    int first = 0;
    int count = INT_MAX;
    if (nargs > 1) {
        first = luaL_checkinteger(L, 2);
    }
    if (nargs > 2) {
        count = luaL_checkinteger(L, 3);
    }
    am_draw_arrays_command *cmd = new (lua_newuserdata(L, sizeof(am_draw_arrays_command))) am_draw_arrays_command(first, count);
    append_command(L, node, 1, cmd, -1);
    lua_pop(L, 1); // cmd
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static void register_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushcclosure(L, draw_children_command, 0);
    lua_setfield(L, -2, "draw_children");
    lua_pushcclosure(L, set_float_command, 0);
    lua_setfield(L, -2, "set_float");
    lua_pushcclosure(L, set_float_array_command, 0);
    lua_setfield(L, -2, "set_float_array");
    lua_pushcclosure(L, use_program_command, 0);
    lua_setfield(L, -2, "use_program");
    lua_pushcclosure(L, draw_arrays_command, 0);
    lua_setfield(L, -2, "draw_arrays");

    lua_pushcclosure(L, append_child, 0);
    lua_setfield(L, -2, "append");
    lua_pushcclosure(L, prepend_child, 0);
    lua_setfield(L, -2, "prepend");
    lua_pushcclosure(L, remove_child, 0);
    lua_setfield(L, -2, "remove");
    lua_pushcclosure(L, replace_child, 0);
    lua_setfield(L, -2, "replace");
    lua_pushcclosure(L, remove_all_children, 0);
    lua_setfield(L, -2, "empty");
    
    lua_pushstring(L, "node");
    lua_setfield(L, -2, "tname");
    lua_newtable(L);
    lua_pushcclosure(L, fallback_index_func, 0);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
    am_register_metatable(L, AM_MT_NODE, 0);
}

void am_open_node_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"node", create_node},
        {NULL, NULL}
    };
    am_open_module(L, "amulet", funcs);
    register_node_mt(L);
}
