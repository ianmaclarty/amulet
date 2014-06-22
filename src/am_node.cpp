#include "amulet.h"

void am_node::render(am_render_state *rstate) {
    if (recursion_limit <= 0) return;
    recursion_limit--;
    int ticket = rstate->trail.get_ticket();
    for (int i = 0; i < command_list_size; i++) {
        am_command *cmd = command_list[i];
        cmd->execute(rstate);
    }
    rstate->trail.rewind_to(ticket);
    recursion_limit++;
}

static int create_node(lua_State *L) {
    am_node *node = new (am_new_nonatomic_userdata(L, sizeof(am_node))) am_node();
    node->recursion_limit = am_conf_default_recursion_limit;
    node->command_list = (am_command**)lua_newuserdata(L, sizeof(am_command*) * am_conf_command_list_initial_size);
    node->command_list_capacity = am_conf_command_list_initial_size;
    node->command_list_size = 0;
    node->command_list_ref = am_new_ref(L, -2, -1);
    lua_pop(L, 1); // command list
    am_set_metatable(L, AM_MT_NODE, -1);
    return 1;
}

static void append_command(lua_State *L, am_node *node, int node_idx, am_command *cmd, int cmd_idx) {
    node_idx = lua_absindex(L, node_idx);
    cmd_idx = lua_absindex(L, cmd_idx);
    if (node->command_list_capacity == node->command_list_size) {
        int new_capacity = node->command_list_capacity * 2;
        am_command **new_list = (am_command**)lua_newuserdata(L, sizeof(am_command*) * new_capacity);
        memcpy(new_list, node->command_list, sizeof(am_command*) * node->command_list_capacity);
        node->command_list = new_list;
        node->command_list_capacity = new_capacity;
        am_replace_ref(L, node_idx, node->command_list_ref, -1);
        lua_pop(L, 1); // command list userdata
    }
    node->command_list[node->command_list_size++] = cmd;
    am_new_ref(L, node_idx, cmd_idx);
}

static int fallback_index_func(lua_State *L) {
    lua_pushnil(L);
    return 1;
}

static int set_float_command(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_set_float_param_command *cmd = new (lua_newuserdata(L, sizeof(am_set_float_param_command))) am_set_float_param_command(L, nargs, node);
    append_command(L, node, 1, cmd, -1);
    lua_pop(L, 1); // cmd
    lua_pushvalue(L, 1); // return node for chaining
    return 1;
}

static int set_float_array_command(lua_State *L) {
    int nargs = am_check_nargs(L, 3);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_set_float_array_command *cmd = new (lua_newuserdata(L, sizeof(am_set_float_array_command))) am_set_float_array_command(L, nargs, node);
    append_command(L, node, 1, cmd, -1);
    lua_pop(L, 1); // cmd
    lua_pushvalue(L, 1); // return node for chaining
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
    lua_pushvalue(L, 1); // return node for chaining
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
    lua_pushvalue(L, 1); // return node for chaining
    return 1;
}

static void register_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, set_float_command, 0);
    lua_setfield(L, -2, "set_float");
    lua_pushcclosure(L, set_float_array_command, 0);
    lua_setfield(L, -2, "set_float_array");
    lua_pushcclosure(L, use_program_command, 0);
    lua_setfield(L, -2, "use_program");
    lua_pushcclosure(L, draw_arrays_command, 0);
    lua_setfield(L, -2, "draw_arrays");
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
