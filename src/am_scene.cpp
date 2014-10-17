#include "amulet.h"

#define AM_NODE_FLAG_LIVE        0x00000001
#define AM_NODE_FLAG_MARK        0x00000002

am_scene_node::am_scene_node() {
    recursion_limit = am_conf_default_recursion_limit;
    flags = 0;
    root_count = 0;
    action_list = NULL;
}

void am_scene_node::render(am_render_state *rstate) {
    render_children(rstate);
}

void am_scene_node::render_children(am_render_state *rstate) {
    if (recursion_limit < 0) return;
    recursion_limit--;
    for (int i = 0; i < children.size; i++) {
        children.arr[i].child->render(rstate);
    }
    recursion_limit++;
}

void am_scene_node::activate() {
    am_action *action = action_list;
    while (action != NULL) {
        am_schedule_action(action);
        action = action->nnext;
    }
}

void am_scene_node::deactivate() {
    am_action *action = action_list;
    while (action != NULL) {
        am_deschedule_action(action);
        action = action->nnext;
    }
}

static bool update_liveness_ancestors(am_scene_node *node) {
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
        am_scene_node *parent = node->parents.arr[i];
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

static void update_liveness_after_removal(am_scene_node *node) {
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
        am_scene_node *child = node->children.arr[i].child;
        // if child is an ancestor of node (i.e. a cycle) then
        // child could have been marked not live by the call
        // to update_liveness_ancestors above
        if (child->flags & AM_NODE_FLAG_LIVE) {
            update_liveness_after_removal(child);
        }
    }
}

static void update_liveness_descendents(am_scene_node *node) {
    if (!(node->flags & AM_NODE_FLAG_LIVE)) {
        node->flags |= AM_NODE_FLAG_LIVE;
        node->activate();
        for (int i = 0; i < node->children.size; i++) {
            am_scene_node *child = node->children.arr[i].child;
            update_liveness_descendents(child);
        }
    }
}

static void update_liveness_after_insertion(am_scene_node *node, am_scene_node *parent) {
    if (parent->flags & AM_NODE_FLAG_LIVE) {
        // parent live, so this node and all its descendents are live too
        update_liveness_descendents(node);
    }
}

void am_scene_node::activate_root() {
    root_count++;
    update_liveness_descendents(this);
}

void am_scene_node::deactivate_root() {
    assert(root_count > 0);
    assert(flags & AM_NODE_FLAG_LIVE);
    root_count--;
    if (root_count == 0) {
        update_liveness_after_removal(this);
    }
}

static int create_scene_node(lua_State *L) {
    new (am_new_nonatomic_userdata(L, sizeof(am_scene_node))) am_scene_node();
    am_set_metatable(L, AM_MT_SCENE_NODE, -1);
    return 1;
}

static int create_action(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_scene_node *node = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 1);
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

static inline void add_parent(lua_State *L, am_scene_node *parent, int parent_idx, am_scene_node *child, int child_idx) {
    child->parents.push_back(L, child_idx, parent);
}

static inline void remove_parent(lua_State *L, am_scene_node *parent, am_scene_node *child, int child_idx) {
    for (int i = 0; i < child->parents.size; i++) {
        if (child->parents.arr[i] == parent) {
            child->parents.remove(i);
            return;
        }
    }
    assert(false); // child->parent ref not found
}

static int append_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *parent = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 1);
    am_scene_node *child = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 2);
    am_node_child child_slot;
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
    am_scene_node *parent = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 1);
    am_scene_node *child = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 2);
    am_node_child child_slot;
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
    am_scene_node *parent = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 1);
    am_scene_node *child = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 2);
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
    am_scene_node *parent = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 1);
    am_scene_node *old_child = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 2);
    am_scene_node *new_child = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 3);
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
    am_scene_node *parent = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 1);
    for (int i = 0; i < parent->children.size; i++) {
        am_push_ref(L, 1, parent->children.arr[i].ref); // push child
        am_scene_node *child = parent->children.arr[i].child;
        remove_parent(L, parent, child, -1);
        update_liveness_after_removal(child);
        lua_pop(L, 1); // child
        am_delete_ref(L, 1, parent->children.arr[i].ref);
    }
    parent->children.clear();
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int node_index(lua_State *L) {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    lua_remove(L, -2); // metatable
    return 1;
}

void am_program_node::render(am_render_state *rstate) {
    render_children(rstate);
    for (int i = 0; i < program->num_params; i++) {
        am_program_param *param = program->params[i];
        am_param_name_map[param->name].param = param;
    }
    am_program* old_program = rstate->active_program;
    rstate->active_program = program;
    render_children(rstate);
    rstate->active_program = old_program;
    if (old_program != NULL) {
        for (int i = 0; i < old_program->num_params; i++) {
            am_program_param *param = old_program->params[i];
            am_param_name_map[param->name].param = param;
        }
    }
}

static void set_child(lua_State *L, am_scene_node *parent, int parent_idx, am_scene_node *child, int child_idx) {
    am_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = am_new_ref(L, parent_idx, child_idx); // ref from parent to child
    parent->children.push_back(L, parent_idx, child_slot);
    add_parent(L, parent, parent_idx, child, child_idx);
    update_liveness_after_insertion(child, parent);
}

static int create_program_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *child = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 1);
    am_program *prog = (am_program*)am_check_metatable_id(L, AM_MT_PROGRAM, 2);
    am_program_node *node = new (am_new_nonatomic_userdata(L, sizeof(am_program_node))) am_program_node();
    am_set_metatable(L, AM_MT_PROGRAM_NODE, -1);
    node->program = prog;
    node->program_ref = am_new_ref(L, -1, 2); // add ref from node to prog.
    set_child(L, node, -1, child, 1);
    return 1;
}

void am_draw_arrays_node::render(am_render_state *rstate) {
    rstate->draw_arrays(first, count);
}

static int create_draw_arrays_node(lua_State *L) {
    int nargs = am_check_nargs(L, 0);
    int first = 0;
    int count = INT_MAX;
    if (nargs > 0) {
        first = luaL_checkinteger(L, 1);
    }
    if (nargs > 1) {
        count = luaL_checkinteger(L, 2);
    }
    am_draw_arrays_node *node = new (am_new_nonatomic_userdata(L, sizeof(am_draw_arrays_node))) am_draw_arrays_node();
    am_set_metatable(L, AM_MT_DRAW_ARRAYS_NODE, -1);
    node->first = first;
    node->count = count;
    return 1;
}

void am_array_param_node::render(am_render_state *rstate) {
    am_program_param *p = am_param_name_map[name].param;
    if (p == NULL) return;
    int available_bytes = vbo->size - offset - am_attribute_client_type_size(type);
    int max_draw_elements = 0;
    if (available_bytes > 0) {
        max_draw_elements = available_bytes / stride + 1;
    }
    XXX
    p->trailed_set_array(rstate, vbo, type, dimensions, normalized, stride, offset,
        max_draw_elements);
}

static int create_array_param_node(lua_State *L) {
    int nargs = am_check_nargs(L, 3);
    am_scene_node *child = (am_scene_node*)am_check_metatable_id(L, AM_MT_SCENE_NODE, 1);
    am_array_param_node *node = new (am_new_nonatomic_userdata(L, sizeof(am_array_param_node))) am_array_param_node();
    am_set_metatable(L, AM_MT_ARRAY_PARAM_NODE, -1);
    int node_idx = am_absindex(L, -1);
    if (!lua_isstring(L, 2)) {
        luaL_error(L, "expecting a string in position 2");
    }
    node->name = am_lookup_param_name(L, 2);
    am_buffer_view *view = (am_buffer_view*)am_check_metatable_id(L, AM_MT_BUFFER_VIEW, 3);

    am_push_ref(L, 3, view->buffer_ref); // push buffer
    if (view->buffer->vbo == NULL) {
        am_push_new_vertex_buffer(L, view->buffer, -1);  // push new vbo
    } else {
        am_push_ref(L, -1, view->buffer->vbo->vbo_ref);  // push existing vbo
    }
    int vbo_idx = am_absindex(L, -1);

    node->vbo = view->buffer->vbo;
    node->vbo_ref = am_new_ref(L, node_idx, vbo_idx); // create ref from node to vbo
    am_buf_view_type_to_attr_client_type_and_dimensions(view->type, &node->type, &node->dimensions);
    node->normalized = view->normalized;
    node->stride = view->stride;
    node->offset = view->offset;

    lua_pop(L, 2); // buffer, vbo
    return 1;
}

static void register_scene_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, node_index, 0);
    lua_setfield(L, -2, "__index");

    lua_pushcclosure(L, append_child, 0);
    lua_setfield(L, -2, "append");
    lua_pushcclosure(L, prepend_child, 0);
    lua_setfield(L, -2, "prepend");
    lua_pushcclosure(L, remove_child, 0);
    lua_setfield(L, -2, "remove");
    lua_pushcclosure(L, replace_child, 0);
    lua_setfield(L, -2, "replace");
    lua_pushcclosure(L, remove_all_children, 0);
    lua_setfield(L, -2, "remove_all");
    
    lua_pushcclosure(L, create_program_node, 0);
    lua_setfield(L, -2, "program");

    lua_pushcclosure(L, create_action, 0);
    lua_setfield(L, -2, "action");

    lua_pushstring(L, "node");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, AM_MT_SCENE_NODE, 0);
}

static void register_program_node_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushstring(L, "program_node");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, AM_MT_PROGRAM_NODE, AM_MT_SCENE_NODE);
}

static void register_draw_arrays_node_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushstring(L, "draw_arrays_node");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, AM_MT_DRAW_ARRAYS_NODE, AM_MT_SCENE_NODE);
}

void am_open_scene_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"empty", create_scene_node},
        {"draw_arrays", create_draw_arrays_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_scene_node_mt(L);
    register_program_node_mt(L);
    register_draw_arrays_node_mt(L);
}
