#include "amulet.h"

am_tag AM_TAG_GROUP;
am_tag AM_TAG_BIND;
am_tag AM_TAG_USE_PROGRAM;
am_tag AM_TAG_TRANSLATE;
am_tag AM_TAG_ROTATE;
am_tag AM_TAG_SCALE;
am_tag AM_TAG_TRANSFORM;
am_tag AM_TAG_BILLBOARD;
am_tag AM_TAG_LOOKAT;
am_tag AM_TAG_BLEND;
am_tag AM_TAG_DRAW;
am_tag AM_TAG_VIEWPORT;
am_tag AM_TAG_COLOR_MASK;
am_tag AM_TAG_CULL_FACE;
am_tag AM_TAG_DEPTH_TEST;
am_tag AM_TAG_STENCIL_TEST;
am_tag AM_TAG_CULL_SPHERE;
am_tag AM_TAG_CULL_BOX;
am_tag AM_TAG_READ_UNIFORM;

static am_tag lookup_tag(lua_State *L, int name_idx);
static am_scene_node *find_tag(am_scene_node *node, am_tag tag, am_scene_node **parent);

am_scene_node::am_scene_node() {
    children.owner = this;
    tags.owner = this;
    recursion_limit = am_conf_default_recursion_limit;
    flags = 0;
    actions_ref = LUA_NOREF;
    action_seq = 0;
}

void am_scene_node::render_children(am_render_state *rstate) {
    if (recursion_limit < 0) return;
    recursion_limit--;
    for (int i = 0; i < children.size; i++) {
        am_scene_node *child = children.arr[i].child;
        if (!am_node_hidden(child)) {
            child->render(rstate);
        }
    }
    recursion_limit++;
}

void am_scene_node::render(am_render_state *rstate) {
    render_children(rstate);
}

int am_scene_node_index(lua_State *L) {
    return am_default_index_func(L);
}

int am_scene_node_newindex(lua_State *L) {
    return am_default_newindex_func(L);
}

static int append_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *child = am_get_userdata(L, am_scene_node, 2);
    am_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = parent->ref(L, 2); // ref from parent to child
    parent->children.push_back(L, child_slot);
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
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *child;
    if (lua_type(L, 2) == LUA_TSTRING) {
        am_tag tag = lookup_tag(L, 2);
        child = find_tag(parent, tag, &parent);
        if (child == NULL || parent == NULL) {
            goto end;
        }
    } else {
        child = am_get_userdata(L, am_scene_node, 2);
    }
    for (int i = 0; i < parent->children.size; i++) {
        if (parent->children.arr[i].child == child) {
            parent->unref(L, parent->children.arr[i].ref);
            parent->children.remove(i);
            break;
        }
    }
end:
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int replace(lua_State *L) {
    am_check_nargs(L, 3);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *old_child;
    am_scene_node *new_child;
    if (lua_type(L, 2) == LUA_TSTRING) {
        am_tag tag = lookup_tag(L, 2);
        old_child = find_tag(parent, tag, &parent);
        if (old_child == NULL || parent == NULL) {
            goto end;
        }
    } else {
        old_child = am_get_userdata(L, am_scene_node, 2);
    }
    new_child = am_get_userdata(L, am_scene_node, 3);
    for (int i = 0; i < parent->children.size; i++) {
        if (parent->children.arr[i].child == old_child) {
            parent->unref(L, parent->children.arr[i].ref);
            parent->children.remove(i);
            am_node_child slot;
            slot.child = new_child;
            slot.ref = parent->ref(L, 3);
            parent->children.insert(L, i, slot);
            break;
        }
    }
end:
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_all(lua_State *L) {
    am_check_nargs(L, 1);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    for (int i = parent->children.size-1; i >= 0; i--) {
        parent->unref(L, parent->children.arr[i].ref);
        parent->children.remove(i);
    }
    assert(parent->children.size == 0);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int create_group_node(lua_State *L) {
    int nargs = am_check_nargs(L, 0);
    am_scene_node *node = am_new_userdata(L, am_scene_node);
    node->tags.push_back(L, AM_TAG_GROUP);
    if (nargs >= 1 && lua_istable(L, 1)) {
        int i = 1;
        do {
            lua_rawgeti(L, 1, i);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1); // nil
                break;
            }
            am_scene_node *child = am_get_userdata(L, am_scene_node, -1);
            am_node_child child_slot;
            child_slot.child = child;
            child_slot.ref = node->ref(L, -1); // ref from node to child
            node->children.push_back(L, child_slot);
            lua_pop(L, 1); // child
            i++;
        } while (true);
    } else {
        for (int i = 0; i < nargs; i++) {
            am_scene_node *child = am_get_userdata(L, am_scene_node, i+1);
            am_node_child child_slot;
            child_slot.child = child;
            child_slot.ref = node->ref(L, i+1); // ref from node to child
            node->children.push_back(L, child_slot);
        }
    }
    return 1;
}

// Chaining (^ operator)

static void add_chain_children(lua_State *L, am_scene_node *parent) {
    if (lua_istable(L, 2)) {
        int i = 1;
        do {
            lua_rawgeti(L, 2, i);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1); // nil
                break;
            }
            am_scene_node *child = am_get_userdata(L, am_scene_node, -1);
            am_node_child child_slot;
            child_slot.child = child;
            child_slot.ref = parent->ref(L, -1); // ref from parent to child
            parent->children.push_back(L, child_slot);
            lua_pop(L, 1); // child
            i++;
        } while (true);
    } else {
        am_scene_node *child = am_get_userdata(L, am_scene_node, 2);
        am_node_child child_slot;
        child_slot.child = child;
        child_slot.ref = parent->ref(L, 2); // ref from parent to child
        parent->children.push_back(L, child_slot);
    }
}

static void chain_leaves(lua_State *L, am_scene_node *node) {
    if (am_node_marked(node)) return;
    am_mark_node(node);
    if (node->children.size == 0) {
        add_chain_children(L, node);
    } else {
        for (int i = 0; i < node->children.size; i++) {
            chain_leaves(L, node->children.arr[i].child);
        }
    }
}

static void unmark_all(am_scene_node *node) {
    if (!am_node_marked(node)) return;
    am_unmark_node(node);
    for (int i = 0; i < node->children.size; i++) {
        unmark_all(node->children.arr[i].child);
    }
}

static void check_chain_arg_2(lua_State *L) {
    if (lua_istable(L, 2)) {
        int i = 1;
        do {
            lua_rawgeti(L, 2, i);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1); // nil
                break;
            }
            void *ud = am_check_metatable_id_no_err(L, MT_am_scene_node, -1);
            if (ud == NULL) {
                luaL_error(L, "expecting a scene node at index %d", i);
            }
            lua_pop(L, 1); // child
            i++;
        } while (true);
    } else {
        if (am_check_metatable_id_no_err(L, MT_am_scene_node, 2) == NULL) {
            luaL_error(L, "expecting a scene node in position 2");
        }
    }
}

static int chain(lua_State *L) {
    am_check_nargs(L, 2);
    check_chain_arg_2(L);
    if (lua_istable(L, 1)) {
        int i = 1;
        do {
            lua_rawgeti(L, 1, i);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1); // nil
                break;
            }
            am_scene_node *parent = am_get_userdata(L, am_scene_node, -1);
            chain_leaves(L, parent);
            lua_pop(L, 1); // parent
            i++;
        } while (true);
        do {
            lua_rawgeti(L, 1, i);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1); // nil
                break;
            }
            am_scene_node *parent = am_get_userdata(L, am_scene_node, -1);
            unmark_all(parent);
            lua_pop(L, 1); // parent
            i++;
        } while (true);
    } else {
        am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
        chain_leaves(L, parent);
        unmark_all(parent);
    }
    lua_pushvalue(L, 1);
    return 1;
}

// Wrap nodes

void am_wrap_node::render(am_render_state *rstate) {
    if (inside) {
        inside = false;
        render_children(rstate);
        inside = true;
    } else {
        inside = true;
        wrapped->render(rstate);
        inside = false;
    }
}

static int create_wrap_node(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    if (nargs > 1) {
        // we depend on there only being one argument for
        // chain_leaves call below.
        luaL_error(L, "too many arguments (only 1 expected)");
    }
    am_wrap_node *node = am_new_userdata(L, am_wrap_node);
    node->wrapped = am_get_userdata(L, am_scene_node, 1);
    node->wrapped_ref = node->ref(L, 1);
    node->inside = false;

    chain_leaves(L, node->wrapped);
    unmark_all(node->wrapped);

    return 1;
}

static void register_wrap_node_mt(lua_State *L) {
    lua_newtable(L);
    am_register_metatable(L, "wrap_node", MT_am_wrap_node, MT_am_scene_node);
}

static int tag_search_result_newindex(lua_State *L) {
    int i = 1;
    do {
        lua_rawgeti(L, 1, i);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1); // nil
            break;
        }
        lua_pushvalue(L, 2);
        lua_pushvalue(L, 3);
        lua_settable(L, -3);
        i++;
    } while (true);
    return 0;
}

static void register_tag_search_result_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, tag_search_result_newindex, 0);
    lua_setfield(L, -2, "__newindex");
    am_register_metatable(L, "tag_search_result", MT_tag_search_result, 0);
}

// Tags

static int next_tag = 0;

static void init_tag_table(lua_State *L) {
    next_tag = 1;
    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_TAG_TABLE);
}

static am_tag lookup_tag(lua_State *L, int name_idx) {
    name_idx = am_absindex(L, name_idx);
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_TAG_TABLE);
    int tt_idx = lua_gettop(L); // tag table
    lua_pushvalue(L, name_idx);
    lua_rawget(L, tt_idx);
    if (lua_isnil(L, -1)) {
        // tag not seen before, register it.
        lua_pop(L, 1); // nil
        if (next_tag > AM_MAX_TAG) {
            luaL_error(L, "too many tags (max %d)", UINT16_MAX);
        }
        am_tag tag = (am_tag)next_tag++;
        lua_pushvalue(L, name_idx);
        lua_pushinteger(L, tag);
        lua_rawset(L, tt_idx);
        lua_pop(L, 1); // tag table
        return tag;
    } else {
        am_tag tag = (am_tag)lua_tointeger(L, -1);
        lua_pop(L, 2); // tag, tag table
        return tag;
    }
}

static int tag(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 1);
    if (lua_type(L, 2) != LUA_TSTRING) {
        return luaL_error(L, "expecting a string in position 2");
    }
    node->tags.push_back(L, lookup_tag(L, 2));
    lua_pushvalue(L, 1);
    return 1;
}

static int untag(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 1);
    if (lua_type(L, 2) != LUA_TSTRING) {
        return luaL_error(L, "expecting a string in position 2");
    }
    am_tag tag = lookup_tag(L, 2);
    node->tags.remove_all(tag);
    lua_pushvalue(L, 1);
    return 1;
}

static bool node_has_tag(am_scene_node *node, am_tag tag) {
    for (int i = 0; i < node->tags.size; i++) {
        if (node->tags.arr[i] == tag) {
            return true;
        }
    }
    return false;
}

static am_scene_node *find_tag(am_scene_node *node, am_tag tag, am_scene_node **parent) {
    if (am_node_marked(node)) return NULL;
    am_mark_node(node);
    am_scene_node *found = NULL;
    if (node_has_tag(node, tag)) {
        found = node;
        *parent = NULL;
    } else {
        for (int i = 0; i < node->children.size; i++) {
            found = find_tag(node->children.arr[i].child, tag, parent);
            if (found != NULL) {
                if (*parent == NULL) *parent = node;
                break;
            }
        }
    }
    am_unmark_node(node);
    return found;
}

static int search_tag(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 1);
    if (lua_type(L, 2) != LUA_TSTRING) {
        return luaL_error(L, "expecting a string in position 2");
    }
    am_tag tag = lookup_tag(L, 2);
    am_scene_node *parent;
    am_scene_node *found = find_tag(node, tag, &parent);
    if (found == NULL) {
        lua_pushnil(L);
        lua_pushnil(L);
    } else {
        found->push(L);
        if (parent != NULL) {
            parent->push(L);
        } else {
            lua_pushnil(L);
        }
    }
    return 2;
}

static void find_all_tags(lua_State *L, am_scene_node *node, am_tag tag, int *i, int t, bool recurse) {
    if (am_node_marked(node)) return;
    am_mark_node(node);
    bool found_this = false;
    if (node_has_tag(node, tag)) {
        node->push(L);
        lua_rawseti(L, t, *i);
        (*i)++;
        found_this = true;
    }
    if (recurse || !found_this) {
        for (int j = 0; j < node->children.size; j++) {
            find_all_tags(L, node->children.arr[j].child, tag, i, t, recurse);
        }
    }
    am_unmark_node(node);
}

static int search_all_tags(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 1);
    if (lua_type(L, 2) != LUA_TSTRING) {
        return luaL_error(L, "expecting a string in position 2");
    }
    bool recurse = false;
    if (nargs > 2) {
        recurse = lua_toboolean(L, 3);
    }
    am_tag tag = lookup_tag(L, 2);
    lua_newtable(L);
    am_push_metatable(L, MT_tag_search_result);
    lua_setmetatable(L, -2);
    int i = 1;
    find_all_tags(L, node, tag, &i, am_absindex(L, -1), recurse);
    return 1;
}

static void init_default_tags(lua_State *L) {
    lua_pushstring(L, "group");
    AM_TAG_GROUP = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "bind");
    AM_TAG_BIND = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "use_program");
    AM_TAG_USE_PROGRAM = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "translate");
    AM_TAG_TRANSLATE = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "rotate");
    AM_TAG_ROTATE = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "scale");
    AM_TAG_SCALE = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "transform");
    AM_TAG_TRANSFORM = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "billboard");
    AM_TAG_BILLBOARD = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "lookat");
    AM_TAG_LOOKAT = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "blend");
    AM_TAG_BLEND = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "draw");
    AM_TAG_DRAW = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "viewport");
    AM_TAG_VIEWPORT = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "color_mask");
    AM_TAG_COLOR_MASK = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "cull_face");
    AM_TAG_CULL_FACE = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "depth_test");
    AM_TAG_DEPTH_TEST = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "stencil_test");
    AM_TAG_STENCIL_TEST = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "cull_sphere");
    AM_TAG_CULL_SPHERE = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "cull_box");
    AM_TAG_CULL_BOX = lookup_tag(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "read_uniform");
    AM_TAG_READ_UNIFORM = lookup_tag(L, -1);
    lua_pop(L, 1);
}

// Other stuff

static int child_pair_next(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 1);
    int i = luaL_checkinteger(L, 2);
    if (i >= 0 && i < node->children.size) {
        lua_pushinteger(L, i+1);
        node->children.arr[i].child->push(L);
        return 2;
    } else {
        lua_pushnil(L);
        return 1;
    }
}

static int child_pairs(lua_State *L) {
    lua_pushcclosure(L, child_pair_next, 0);
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 0);
    return 3;
}

static int get_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 1);
    int i = luaL_checkinteger(L, 2);
    if (i >= 1 && i <= node->children.size) {
        node->children.arr[i-1].child->push(L);
        return 1;
    } else {
        return 0;
    }
}

static void get_hidden(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    lua_pushboolean(L, am_node_hidden(node));
}

static void set_hidden(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    am_set_node_hidden(node, lua_toboolean(L, 3));
}

static am_property hidden_property = {get_hidden, set_hidden};

static void get_paused(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    lua_pushboolean(L, am_node_paused(node));
}

static void set_paused(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    am_set_node_paused(node, lua_toboolean(L, 3));
}

static am_property paused_property = {get_paused, set_paused};

static void get_num_children(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    lua_pushinteger(L, node->children.size);
}

static am_property num_children_property = {get_num_children, NULL};

static void get_recursion_limit(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    lua_pushinteger(L, node->recursion_limit);
}

static void set_recursion_limit(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    node->recursion_limit = lua_tointeger(L, 3);
}

static am_property recursion_limit_property = {get_recursion_limit, set_recursion_limit};

static void get_actions(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    if (node->actions_ref == LUA_NOREF) {
        lua_pushnil(L);
    } else {
        node->pushref(L, node->actions_ref);
    }
}

static void set_actions(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    if (node->actions_ref == LUA_NOREF) {
        node->actions_ref = node->ref(L, 3);
    } else {
        node->reref(L, node->actions_ref, 3);
    }
}

static am_property actions_property = {get_actions, set_actions};

static void register_scene_node_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");
    lua_pushcclosure(L, chain, 0);
    lua_setfield(L, -2, "__pow");

    lua_pushcclosure(L, child_pairs, 0);
    lua_setfield(L, -2, "child_pairs");
    lua_pushcclosure(L, get_child, 0);
    lua_setfield(L, -2, "child");

    am_register_property(L, "hidden", &hidden_property);
    am_register_property(L, "paused", &paused_property);
    am_register_property(L, "num_children", &num_children_property);
    am_register_property(L, "recursion_limit", &recursion_limit_property);
    am_register_property(L, "_actions", &actions_property);

    lua_pushcclosure(L, append_child, 0);
    lua_setfield(L, -2, "append");
    lua_pushcclosure(L, prepend_child, 0);
    lua_setfield(L, -2, "prepend");
    lua_pushcclosure(L, remove, 0);
    lua_setfield(L, -2, "remove");
    lua_pushcclosure(L, replace, 0);
    lua_setfield(L, -2, "replace");
    lua_pushcclosure(L, remove_all, 0);
    lua_setfield(L, -2, "remove_all");

    lua_pushcclosure(L, search_tag, 0);
    lua_setfield(L, -2, "__call");
    lua_pushcclosure(L, search_all_tags, 0);
    lua_setfield(L, -2, "all");
    lua_pushcclosure(L, tag, 0);
    lua_setfield(L, -2, "tag");
    lua_pushcclosure(L, untag, 0);
    lua_setfield(L, -2, "untag");

    am_register_metatable(L, "scene_node", MT_am_scene_node, 0);
}

void am_open_scene_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"group", create_group_node},
        {"wrap", create_wrap_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_scene_node_mt(L);
    register_wrap_node_mt(L);
    register_tag_search_result_mt(L);
    init_tag_table(L);
    init_default_tags(L);
}
