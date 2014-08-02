#include "amulet.h"

am_node::am_node() {
    recursion_limit = am_conf_default_recursion_limit;
    flags = 0;
    root_count = 0;
    action_list = NULL;
}

void am_node::render(am_render_state *rstate) {
    if (recursion_limit < 0) return;
    recursion_limit--;
    int ticket = rstate->trail.get_ticket();
    for (int i = 0; i < command_list.size; i++) {
        am_command *cmd = command_list.arr[i];
        cmd->execute(this, rstate);
    }
    rstate->trail.rewind_to(ticket);
    recursion_limit++;
}

void am_node::activate() {
    am_action *action = action_list;
    while (action != NULL) {
        am_schedule_action(action);
        action = action->nnext;
    }
}

void am_node::deactivate() {
    am_action *action = action_list;
    while (action != NULL) {
        am_deschedule_action(action);
        action = action->nnext;
    }
}

static bool update_liveness_ancestors(am_node *node) {
    assert(node->flags & AM_NODE_FLAG_LIVE);
    if (node->root_count > 0) {
        return true; // root found
    }
    if (node->flags & AM_NODE_FLAG_MARK) {
        return false; // cycle detected
    }
    node->flags |= AM_NODE_FLAG_MARK;
    bool found_root = false;
    for (int i = 0; i < node->parents.size; i++) {
        am_node *parent = node->parents.arr[i].parent;
        if ((parent->flags & AM_NODE_FLAG_LIVE) && update_liveness_ancestors(parent)) {
            found_root = true;
            break;
        }
    }
    node->flags &= ~AM_NODE_FLAG_MARK;
    if (!found_root) {
        // root not reachable, so node not live
        node->flags &= ~AM_NODE_FLAG_LIVE;
        node->deactivate();
    }
    return found_root;
}

static void update_liveness_after_removal(am_node *node) {
    if (!(node->flags & AM_NODE_FLAG_LIVE)) {
        // node already not live, removing it doesn't change anything
        return;
    }
    if (update_liveness_ancestors(node)) {
        // node is still live, nothing more to do
        return;
    }
    // node not live - update liveness of children
    for (int i = 0; i < node->children.size; i++) {
        am_node *child = node->children.arr[i].child;
        // if child is an ancestor of node (i.e. a cycle) then
        // child could have been marked not live by the call
        // to update_liveness_ancestors above
        if (child->flags & AM_NODE_FLAG_LIVE) {
            update_liveness_after_removal(child);
        }
    }
}

static void update_liveness_descendents(am_node *node) {
    if (!(node->flags & AM_NODE_FLAG_LIVE)) {
        node->flags |= AM_NODE_FLAG_LIVE;
        node->activate();
        for (int i = 0; i < node->children.size; i++) {
            am_node *child = node->children.arr[i].child;
            update_liveness_descendents(child);
        }
    }
}

static void update_liveness_after_insertion(am_node *node, am_node *parent) {
    if (parent->flags & AM_NODE_FLAG_LIVE) {
        // parent live, so this node and all its descendents are live too
        update_liveness_descendents(node);
    }
}

void am_node::activate_root() {
    root_count++;
    update_liveness_descendents(this);
}

void am_node::deactivate_root() {
    assert(root_count > 0);
    assert(flags & AM_NODE_FLAG_LIVE);
    root_count--;
    if (root_count == 0) {
        update_liveness_after_removal(this);
    }
}

static int create_node(lua_State *L) {
    new (am_new_nonatomic_userdata(L, sizeof(am_node))) am_node();
    am_set_metatable(L, AM_MT_NODE, -1);
    return 1;
}

static int create_action(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_action *action = new (lua_newuserdata(L, sizeof(am_action))) am_action();
    action->node = node;
    action->action_ref = am_new_ref(L, 1, -1); // ref from node to action
    if (!lua_isfunction(L, 2)) {
        return luaL_error(L, "expecting a function at position 2");
    }
    action->func_ref = am_new_ref(L, 1, 2);
    if (nargs > 2) {
        if (!lua_isnil(L, 3)) {
            action->tag_ref = am_new_ref(L, 1, 3);
        }
    }
    if (nargs > 3) {
        action->priority = luaL_checkinteger(L, 4);
    }
    lua_pop(L, 1); // action

    action->nnext = node->action_list;
    node->action_list = action;

    if (node->flags & AM_NODE_FLAG_LIVE) {
        am_schedule_action(action);
    }

    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static inline void add_parent(lua_State *L, am_node *parent, int parent_idx, am_node *child, int child_idx) {
    am_parent_slot parent_slot;
    parent_slot.parent = parent;
    parent_slot.ref = am_new_ref(L, child_idx, parent_idx); // ref from child to parent
    child->parents.push_back(L, child_idx, parent_slot);
}

static inline void remove_parent(lua_State *L, am_node *parent, am_node *child, int child_idx) {
    for (int i = 0; i < child->parents.size; i++) {
        if (child->parents.arr[i].parent == parent) {
            am_delete_ref(L, child_idx, child->parents.arr[i].ref);
            child->parents.remove(i);
            return;
        }
    }
    assert(false); // child->parent ref not found
}

static int append_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_node *parent = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_node *child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    am_child_slot child_slot;
    child_slot.child = child;
    child_slot.ref = am_new_ref(L, 1, 2); // ref from parent to child
    parent->children.push_back(L, 1, child_slot);
    add_parent(L, parent, 1, child, 2);
    update_liveness_after_insertion(child, parent);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int prepend_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_node *parent = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_node *child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    am_child_slot child_slot;
    child_slot.child = child;
    child_slot.ref = am_new_ref(L, 1, 2);
    parent->children.push_front(L, 1, child_slot); // ref from parent to child
    add_parent(L, parent, 1, child, 2);
    update_liveness_after_insertion(child, parent);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_node *parent = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_node *child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    for (int i = 0; i < parent->children.size; i++) {
        if (parent->children.arr[i].child == child) {
            am_delete_ref(L, 1, parent->children.arr[i].ref);
            parent->children.remove(i);
            remove_parent(L, parent, child, 2);
            update_liveness_after_removal(child);
            break;
        }
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int replace_child(lua_State *L) {
    am_check_nargs(L, 3);
    am_node *parent = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_node *old_child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 2);
    am_node *new_child = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 3);
    for (int i = 0; i < parent->children.size; i++) {
        if (parent->children.arr[i].child == old_child) {
            am_replace_ref(L, 1, parent->children.arr[i].ref, 3);
            parent->children.arr[i].child = new_child;
            remove_parent(L, parent, old_child, 2);
            update_liveness_after_removal(old_child);
            add_parent(L, parent, 1, new_child, 3);
            update_liveness_after_insertion(new_child, parent);
            break;
        }
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_all_children(lua_State *L) {
    am_check_nargs(L, 1);
    am_node *parent = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    for (int i = 0; i < parent->children.size; i++) {
        am_push_ref(L, 1, parent->children.arr[i].ref); // push child
        am_node *child = parent->children.arr[i].child;
        remove_parent(L, parent, child, -1);
        update_liveness_after_removal(child);
        lua_pop(L, 1); // child
        am_delete_ref(L, 1, parent->children.arr[i].ref);
    }
    parent->children.clear();
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static void append_command(lua_State *L, am_node *node, int node_idx, am_command *cmd, int cmd_idx) {
    node->command_list.push_back(L, node_idx, cmd);
    am_new_ref(L, node_idx, cmd_idx);
}

static int node_index(lua_State *L) {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_remove(L, -2); // metatable
        return 1;
    }

    lua_pop(L, 2); // nil, metatable

    // look in uservalue table
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (lua_islightuserdata(L, -1)) {
        am_command *cmd = (am_command*)lua_touserdata(L, -1);
        lua_pop(L, 2); // cmd, uservalue
        return cmd->alias_index(L);
    } else {
        lua_pop(L, 2); // cmd, uservalue
        lua_pushnil(L);
        return 1;
    }
}

static int node_newindex(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (lua_islightuserdata(L, -1)) {
        am_command *cmd = (am_command*)lua_touserdata(L, -1);
        lua_pop(L, 2); // cmd, uservalue
        return cmd->alias_newindex(L);
    } else {
        lua_pop(L, 2); // cmd, uservalue
        const char *field = lua_tostring(L, 2);
        if (field == NULL) field = "<unknown>";
        return luaL_error(L, "no such field: '%s'");
    }
}

static int create_command_alias(lua_State *L) {
    am_check_nargs(L, 2);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    if (node->command_list.size == 0) {
        return luaL_error(L, "add a command before creating an alias");
    }
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2); // key
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pop(L, 2); // existing value, uservalue
        const char *alias = lua_tostring(L, 2);
        if (alias == NULL) alias = "<unknown>";
        return luaL_error(L, "alias '%s' already used", alias);
    }
    lua_pop(L, 1); // nil
    lua_pushvalue(L, 2); // key
    lua_pushlightuserdata(L, node->command_list.arr[node->command_list.size-1]);
    lua_rawset(L, -3);
    lua_pop(L, 1); // uservalue table
    lua_pushvalue(L, 1); // for chaining
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

static int set_array_command(lua_State *L) {
    int nargs = am_check_nargs(L, 3);
    am_node *node = (am_node*)am_check_metatable_id(L, AM_MT_NODE, 1);
    am_set_array_command *cmd = new (lua_newuserdata(L, sizeof(am_set_array_command))) am_set_array_command(L, nargs, node);
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
    lua_pushcclosure(L, node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    lua_pushcclosure(L, draw_children_command, 0);
    lua_setfield(L, -2, "draw_children");
    lua_pushcclosure(L, set_float_command, 0);
    lua_setfield(L, -2, "set_float");
    lua_pushcclosure(L, set_array_command, 0);
    lua_setfield(L, -2, "set_array");
    lua_pushcclosure(L, use_program_command, 0);
    lua_setfield(L, -2, "use_program");
    lua_pushcclosure(L, draw_arrays_command, 0);
    lua_setfield(L, -2, "draw_arrays");

    lua_pushcclosure(L, create_command_alias, 0);
    lua_setfield(L, -2, "as");

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
    
    lua_pushcclosure(L, create_action, 0);
    lua_setfield(L, -2, "action");

    lua_pushstring(L, "node");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, AM_MT_NODE, 0);
}

void am_open_node_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"node", create_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_node_mt(L);
}
