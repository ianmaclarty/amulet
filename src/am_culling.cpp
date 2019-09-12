#include "amulet.h"

// Cull face

void am_cull_face_node::render(am_render_state *rstate) {
    am_cull_face_state old_state = rstate->active_cull_face_state;
    switch (mode) {
        case AM_CULL_FACE_MODE_FRONT:
            rstate->active_cull_face_state.set(true, AM_CULL_FACE_FRONT);
            break;
        case AM_CULL_FACE_MODE_BACK:
            rstate->active_cull_face_state.set(true, AM_CULL_FACE_BACK);
            break;
        case AM_CULL_FACE_MODE_NONE:
            rstate->active_cull_face_state.set(false, AM_CULL_FACE_BACK);
            break;
    }
    render_children(rstate);
    rstate->active_cull_face_state.restore(&old_state);
}

static int create_cull_face_node(lua_State *L) {
    am_check_nargs(L, 1);
    am_cull_face_node *node = am_new_userdata(L, am_cull_face_node);
    node->tags.push_back(L, AM_TAG_CULL_FACE);
    node->mode = am_get_enum(L, am_cull_face_mode, 1);
    return 1;
}

static void get_face(lua_State *L, void *obj) {
    am_cull_face_node *node = (am_cull_face_node*)obj;
    am_push_enum(L, am_cull_face_mode, node->mode);
}

static void set_face(lua_State *L, void *obj) {
    am_cull_face_node *node = (am_cull_face_node*)obj;
    node->mode = am_get_enum(L, am_cull_face_mode, 3);
}

static am_property face_property = {get_face, set_face};

static void register_cull_face_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "face", &face_property);

    am_register_metatable(L, "cull_face", MT_am_cull_face_node, MT_am_scene_node);
}

// Cull sphere

void am_cull_sphere_node::render(am_render_state *rstate) {
    glm::dmat4 matrix = glm::dmat4(1.0);
    for (int i = 0; i < num_names; i++) {
        am_program_param_name_slot *slot = &rstate->param_name_map[names[i]];
        am_program_param_value *param = &slot->value;
        if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
            glm::dmat4 *m = (glm::dmat4*)&param->value.m4[0];
            matrix = matrix * *m;
        } else {
            am_log1("WARNING: matrix '%s' is not a mat4 in cull_sphere node (node will be culled)", slot->name);
            return;
        }
    }
    if (am_sphere_visible(matrix, center, radius)) {
        render_children(rstate);
    }
}

static int create_cull_sphere_node(lua_State *L) {
    if (lua_gettop(L) >= 1 && lua_type(L, 1) != LUA_TSTRING) {
        lua_pushstring(L, am_conf_default_modelview_matrix_name);
        lua_insert(L, 1);
        lua_pushstring(L, am_conf_default_projection_matrix_name);
        lua_insert(L, 1);
    }
    int nargs = am_check_nargs(L, 2);
    if (lua_type(L, 1) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 1");
    am_cull_sphere_node *node = am_new_userdata(L, am_cull_sphere_node);
    node->tags.push_back(L, AM_TAG_CULL_SPHERE);
    node->names[0] = am_lookup_param_name(L, 1);
    node->num_names = 1;
    int i = 2;
    while (i < nargs && lua_type(L, i) == LUA_TSTRING && node->num_names < AM_MAX_CULL_SPHERE_NAMES) {
        node->names[node->num_names] = am_lookup_param_name(L, i);
        node->num_names++;
        i++;
    }
    if (i <= nargs) {
        node->radius = luaL_checknumber(L, i);
        i++;
    } else {
        return luaL_error(L, "expecting radius in position %d", i);
    }
    if (i <= nargs) {
        node->center = am_get_userdata(L, am_vec3, i)->v;
        i++;
    } else {
        node->center = glm::dvec3(0.0);
    }
    return 1;
}

static void get_cull_sphere_radius(lua_State *L, void *obj) {
    am_cull_sphere_node *node = (am_cull_sphere_node*)obj;
    lua_pushnumber(L, node->radius);
}

static void set_cull_sphere_radius(lua_State *L, void *obj) {
    am_cull_sphere_node *node = (am_cull_sphere_node*)obj;
    node->radius = luaL_checknumber(L, 3);
}

static am_property cull_sphere_radius_property = {get_cull_sphere_radius, set_cull_sphere_radius};

static void get_cull_sphere_center(lua_State *L, void *obj) {
    am_cull_sphere_node *node = (am_cull_sphere_node*)obj;
    am_vec3 *center = am_new_userdata(L, am_vec3);
    center->v = node->center;
}

static void set_cull_sphere_center(lua_State *L, void *obj) {
    am_cull_sphere_node *node = (am_cull_sphere_node*)obj;
    node->center = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property cull_sphere_center_property = {get_cull_sphere_center, set_cull_sphere_center};

static void register_cull_sphere_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "radius", &cull_sphere_radius_property);
    am_register_property(L, "center", &cull_sphere_center_property);

    am_register_metatable(L, "cull_sphere", MT_am_cull_sphere_node, MT_am_scene_node);
}

// Cull box

void am_cull_box_node::render(am_render_state *rstate) {
    glm::dmat4 matrix = glm::dmat4(1.0);
    for (int i = 0; i < num_names; i++) {
        am_program_param_name_slot *slot = &rstate->param_name_map[names[i]];
        am_program_param_value *param = &slot->value;
        if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
            glm::dmat4 *m = (glm::dmat4*)&param->value.m4[0];
            matrix = matrix * *m;
        } else {
            am_log1("WARNING: matrix '%s' is not a mat4 in cull_box node (node will be culled)", slot->name);
            return;
        }
    }
    if (am_box_visible(matrix, min, max)) {
        render_children(rstate);
    }
}

static int create_cull_box_node(lua_State *L) {
    if (lua_gettop(L) >= 1 && lua_type(L, 1) != LUA_TSTRING) {
        lua_pushstring(L, am_conf_default_modelview_matrix_name);
        lua_insert(L, 1);
        lua_pushstring(L, am_conf_default_projection_matrix_name);
        lua_insert(L, 1);
    }
    int nargs = am_check_nargs(L, 3);
    if (lua_type(L, 1) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 1");
    am_cull_box_node *node = am_new_userdata(L, am_cull_box_node);
    node->tags.push_back(L, AM_TAG_CULL_BOX);
    node->names[0] = am_lookup_param_name(L, 1);
    node->num_names = 1;
    int i = 2;
    while (i < nargs && lua_type(L, i) == LUA_TSTRING && node->num_names < AM_MAX_CULL_BOX_NAMES) {
        node->names[node->num_names] = am_lookup_param_name(L, i);
        node->num_names++;
        i++;
    }
    if (i <= nargs) {
        node->min = am_get_userdata(L, am_vec3, i)->v;
        i++;
    } else {
        return luaL_error(L, "expecting min (vec3) in position %d", i);
    }
    if (i <= nargs) {
        node->max = am_get_userdata(L, am_vec3, i)->v;
        i++;
    } else {
        return luaL_error(L, "expecting max (vec3) in position %d", i);
    }
    return 1;
}

static void get_cull_box_min(lua_State *L, void *obj) {
    am_cull_box_node *node = (am_cull_box_node*)obj;
    am_new_userdata(L, am_vec3)->v = node->min;
}

static void set_cull_box_min(lua_State *L, void *obj) {
    am_cull_box_node *node = (am_cull_box_node*)obj;
    node->min = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property cull_box_min_property = {get_cull_box_min, set_cull_box_min};

static void get_cull_box_max(lua_State *L, void *obj) {
    am_cull_box_node *node = (am_cull_box_node*)obj;
    am_new_userdata(L, am_vec3)->v = node->max;
}

static void set_cull_box_max(lua_State *L, void *obj) {
    am_cull_box_node *node = (am_cull_box_node*)obj;
    node->max = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property cull_box_max_property = {get_cull_box_max, set_cull_box_max};

static void register_cull_box_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "min", &cull_box_min_property);
    am_register_property(L, "max", &cull_box_max_property);

    am_register_metatable(L, "cull_box", MT_am_cull_box_node, MT_am_scene_node);
}

// Module init

void am_open_culling_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"cull_face", create_cull_face_node},
        {"cull_sphere", create_cull_sphere_node},
        {"cull_box", create_cull_box_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    am_enum_value cull_face_enum[] = {
        {"front", AM_CULL_FACE_MODE_FRONT},
        {"ccw", AM_CULL_FACE_MODE_FRONT},
        {"back", AM_CULL_FACE_MODE_BACK},
        {"cw", AM_CULL_FACE_MODE_BACK},
        {"none", AM_CULL_FACE_MODE_NONE},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_cull_face_mode, cull_face_enum);
    register_cull_face_node_mt(L);
    register_cull_sphere_node_mt(L);
    register_cull_box_node_mt(L);
}
