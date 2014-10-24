#include "amulet.h"

#define AM_NODE_FLAG_LIVE        ((uint32_t)1)
#define AM_NODE_FLAG_MARK        ((uint32_t)2)

#define node_live(node)          (node->flags & AM_NODE_FLAG_LIVE)
#define node_dead(node)          (!node_live(node))
#define set_live(node)           node->flags |= AM_NODE_FLAG_LIVE
#define set_dead(node)           node->flags &= ~AM_NODE_FLAG_LIVE

#define node_marked(node)        (node->flags & AM_NODE_FLAG_MARK)
#define mark_node(node)          node->flags |= AM_NODE_FLAG_MARK
#define unmark_node(node)        node->flags &= ~AM_NODE_FLAG_MARK

am_scene_node::am_scene_node() {
    live_parents.owner = this;
    children.owner = this;
    recursion_limit = am_conf_default_recursion_limit;
    flags = 0;
    root_count = 0;
    action_list = NULL;
}

void am_scene_node::render_children(am_render_state *rstate) {
    if (recursion_limit < 0) return;
    recursion_limit--;
    for (int i = 0; i < children.size; i++) {
        children.arr[i].child->render(rstate);
    }
    recursion_limit++;
}

void am_scene_node::render(am_render_state *rstate) {
    render_children(rstate);
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

// Check whether a node is reachable from a root node.
// This function assumes that nodes that have been marked dead
// are unreachable, but makes no assumption about nodes
// that are marked live.
// (we call this after removing a node, before liveness flags have been
// updated).
// Any nodes found to be dead will be marked as such.
static bool check_liveness(am_scene_node *node) {
    if (node_dead(node)) {
        // node already marked dead
        return false;
    }
    if (node->root_count > 0) {
        return true; // root found
    }
    if (node_marked(node)) {
        return false; // cycle detected
    }
    mark_node(node);
    bool found_root = false;
    for (int i = 0; i < node->live_parents.size; i++) {
        am_scene_node *parent = node->live_parents.arr[i];
        if (check_liveness(parent)) {
            found_root = true;
            break;
        }
    }
    node->flags &= ~AM_NODE_FLAG_MARK;
    if (!found_root) {
        set_dead(node);
        node->deactivate();
    }
    return found_root;
}

// Mark dead nodes that are reachable from the given node.
// The given node should have already been marked dead.
static void mark_dead_reachable(am_scene_node *node) {
    assert(node_dead(node));
    for (int i = 0; i < node->children.size; i++) {
        am_scene_node *child = node->children.arr[i].child;
        if (!check_liveness(child)) {
            // check_liveness would have marked child dead
            mark_dead_reachable(child);
        }
    }
}

// Remove dead nodes from the node's parent list
static void remove_dead_parents(am_scene_node *node) {
    am_scene_node **arr = node->live_parents.arr;
    int i = 0;
    int j = 0;
    int n = node->live_parents.size;
    while (j < n) {
        if (i < j) {
            arr[i] = arr[j];
        }
        if (node_live(arr[i])) {
            i++;
        }
        j++;
    }
    node->live_parents.size = i;
}

// Remove dead nodes from the parent list of this
// node and any dead descendents.
static void remove_dead_parents_reachable(am_scene_node *node) {
    remove_dead_parents(node);
    if (node_dead(node)) {
        for (int i = 0; i < node->children.size; i++) {
            am_scene_node *child = node->children.arr[i].child;
            remove_dead_parents_reachable(child);
        }
    }
}

static void update_liveness_after_removal(am_scene_node *child, am_scene_node *parent) {
    if (node_dead(child)) {
        assert(parent == NULL || node_dead(parent));
        assert(child->live_parents.size == 0);
        // child already dead, removing it doesn't change anything
        return;
    }
    // node is live, so parent should be live too (unless node is root)
    if (parent != NULL) {
        assert(node_live(parent));
        // remove the child->live_parent link (it should exist)
        bool found_parent = child->live_parents.remove_first(parent);
        assert(found_parent);
    }
    if (check_liveness(child)) {
        // child is still live, nothing more to do
        return;
    }
    // child became dead (check_liveness would have marked it dead)
    mark_dead_reachable(child);
    remove_dead_parents_reachable(child);
}

static void mark_live_reachable(lua_State *L, am_scene_node *node) {
    if (node_dead(node)) {
        set_live(node);
        node->activate();
        for (int i = 0; i < node->children.size; i++) {
            am_scene_node *child = node->children.arr[i].child;
            child->live_parents.push_back(L, node);
            mark_live_reachable(L, child);
        }
    }
}

static void update_liveness_after_insertion(lua_State *L, am_scene_node *child, am_scene_node *parent) {
    if (node_live(parent)) {
        child->live_parents.push_back(L, parent);
        mark_live_reachable(L, child);
    }
}

void am_scene_node::activate_root(lua_State *L) {
    root_count++;
    mark_live_reachable(L, this);
}

void am_scene_node::deactivate_root() {
    assert(root_count > 0);
    assert(node_live(this));
    root_count--;
    if (root_count == 0) {
        update_liveness_after_removal(this, NULL);
    }
}

static int create_action(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 1);
    if (!lua_isfunction(L, 2)) {
        return luaL_error(L, "expecting a function at position 2");
    }
    am_action *action = new (lua_newuserdata(L, sizeof(am_action))) am_action();
    action->node = node;
    action->action_ref = node->ref(L, -1); // ref from node to action
    action->func_ref = node->ref(L, 2); // ref from node to function
    if (nargs > 2) {
        if (!lua_isnil(L, 3)) {
            action->tag_ref = node->ref(L, 3);
        }
    }
    if (nargs > 3) {
        action->priority = luaL_checkinteger(L, 4);
    }
    lua_pop(L, 1); // action

    action->nnext = node->action_list;
    node->action_list = action;

    if (node_live(node)) {
        am_schedule_action(action);
    }

    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int append_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *child = am_get_userdata(L, am_scene_node, 2);
    am_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = parent->ref(L, 2); // ref from parent to child
    parent->children.push_back(L, child_slot);
    update_liveness_after_insertion(L, child, parent);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int prepend_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *child = am_get_userdata(L, am_scene_node, 2);
    am_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = parent->ref(L, 2); // ref from parent to child
    parent->children.push_front(L, child_slot);
    update_liveness_after_insertion(L, child, parent);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *child = am_get_userdata(L, am_scene_node, 2);
    for (int i = 0; i < parent->children.size; i++) {
        if (parent->children.arr[i].child == child) {
            parent->unref(L, parent->children.arr[i].ref);
            parent->children.remove(i);
            update_liveness_after_removal(child, parent);
            break;
        }
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int replace_child(lua_State *L) {
    am_check_nargs(L, 3);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *old_child = am_get_userdata(L, am_scene_node, 2);
    am_scene_node *new_child = am_get_userdata(L, am_scene_node, 3);
    for (int i = 0; i < parent->children.size; i++) {
        if (parent->children.arr[i].child == old_child) {
            parent->unref(L, parent->children.arr[i].ref);
            parent->children.remove(i);
            update_liveness_after_removal(old_child, parent);
            am_node_child slot;
            slot.child = new_child;
            slot.ref = parent->ref(L, 3);
            parent->children.insert(L, i, slot);
            update_liveness_after_insertion(L, new_child, parent);
            break;
        }
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_all_children(lua_State *L) {
    am_check_nargs(L, 1);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    for (int i = parent->children.size-1; i >= 0; i--) {
        am_scene_node *child = parent->children.arr[i].child;
        parent->unref(L, parent->children.arr[i].ref);
        parent->children.remove(i);
        update_liveness_after_removal(child, parent);
    }
    assert(parent->children.size == 0);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

/*
static int node_index(lua_State *L) {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    lua_remove(L, -2); // metatable
    return 1;
}
*/

void am_program_node::render(am_render_state *rstate) {
    am_program* old_program = rstate->active_program;
    rstate->active_program = program;
    render_children(rstate);
    rstate->active_program = old_program;
}

static void set_child(lua_State *L, am_scene_node *parent) {
    if (lua_isnil(L, 1)) {
        return;
    }
    am_scene_node *child = am_get_userdata(L, am_scene_node, 1);
    am_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = parent->ref(L, 1); // ref from parent to child
    parent->children.push_back(L, child_slot);
    update_liveness_after_insertion(L, child, parent);
}

static int create_program_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_program *prog = am_get_userdata(L, am_program, 2);
    am_program_node *node = am_new_userdata(L, am_program_node);
    set_child(L, node);
    node->program = prog;
    node->program_ref = node->ref(L, 2);
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
    am_draw_arrays_node *node = am_new_userdata(L, am_draw_arrays_node);
    node->first = first;
    node->count = count;
    return 1;
}

void am_bind_array_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    am_program_param_value old_val = *param;
    param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY;
    param->value.arr = arr;
    render_children(rstate);
    *param = old_val;
}

static int create_bind_array_node(lua_State *L) {
    am_check_nargs(L, 3);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 3);
    am_bind_array_node *node = am_new_userdata(L, am_bind_array_node);

    set_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->arr = view;
    node->arr_ref = node->ref(L, 3); // ref from node to view

    return 1;
}

#define AM_BIND_MAT_NODE_IMPL(D)                                        \
void am_bind_mat##D##_node::render(am_render_state *rstate) {           \
    am_program_param_value *param = &am_param_name_map[name].value;     \
    am_program_param_value old_val = *param;                            \
    param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_MAT##D;                  \
    memcpy(&param->value.m##D[0], glm::value_ptr(m), D * D * sizeof(float)); \
    render_children(rstate);                                            \
    *param = old_val;                                                   \
}                                                                       \
static int create_bind_mat##D##_node(lua_State *L) {                    \
    am_check_nargs(L, 3);                                               \
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2"); \
    am_mat##D *m = am_get_userdata(L, am_mat##D, 3);                    \
    am_bind_mat##D##_node *node = am_new_userdata(L, am_bind_mat##D##_node); \
    set_child(L, node);                                                 \
    node->name = am_lookup_param_name(L, 2);                            \
    node->m = m->m;                                                     \
    return 1;                                                           \
}                                                                       \
static void register_bind_mat##D##_node_mt(lua_State *L) {              \
    lua_newtable(L);                                                    \
    lua_pushvalue(L, -1);                                               \
    lua_setfield(L, -2, "__index");                                     \
                                                                        \
    lua_pushstring(L, "bind_mat" #D);                                   \
    lua_setfield(L, -2, "tname");                                       \
                                                                        \
    am_register_metatable(L, MT_am_bind_mat##D##_node, MT_am_scene_node);\
}

AM_BIND_MAT_NODE_IMPL(2)
AM_BIND_MAT_NODE_IMPL(3)
AM_BIND_MAT_NODE_IMPL(4)

static int create_empty_node(lua_State *L) {
    int nargs = am_check_nargs(L, 0);
    am_scene_node *node = am_new_userdata(L, am_scene_node);
    if (nargs > 0) {
        set_child(L, node);
    }
    return 1;
}

static void register_scene_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
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
    
    lua_pushcclosure(L, create_bind_mat2_node, 0);
    lua_setfield(L, -2, "bind_mat2");
    lua_pushcclosure(L, create_bind_mat3_node, 0);
    lua_setfield(L, -2, "bind_mat3");
    lua_pushcclosure(L, create_bind_mat4_node, 0);
    lua_setfield(L, -2, "bind_mat4");
    lua_pushcclosure(L, create_bind_array_node, 0);
    lua_setfield(L, -2, "bind_array");
    lua_pushcclosure(L, create_program_node, 0);
    lua_setfield(L, -2, "program");
    lua_pushcclosure(L, create_empty_node, 0);
    lua_setfield(L, -2, "empty");

    lua_pushcclosure(L, create_action, 0);
    lua_setfield(L, -2, "action");

    lua_pushstring(L, "node");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_scene_node, 0);
}

static void register_program_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "program_node");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_program_node, MT_am_scene_node);
}

static void register_bind_array_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "bind_array");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_bind_array_node, MT_am_scene_node);
}

static void register_draw_arrays_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "draw_arrays");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_draw_arrays_node, MT_am_scene_node);
}

void am_open_scene_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"draw_arrays", create_draw_arrays_node},
        {"empty", create_empty_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_scene_node_mt(L);
    register_program_node_mt(L);
    register_bind_array_node_mt(L);
    register_bind_mat2_node_mt(L);
    register_bind_mat3_node_mt(L);
    register_bind_mat4_node_mt(L);
    register_draw_arrays_node_mt(L);
}
