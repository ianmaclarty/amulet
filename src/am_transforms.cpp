#include "amulet.h"

static void log_ignored_transform(am_render_state *rstate, am_param_name_id name, const char *transform) {
    am_program_param_name_slot *slot = &rstate->param_name_map[name];
    am_log1("WARNING: ignoring %s on %s '%s' (expecting a mat4)",
        transform,
        am_program_param_client_type_name(slot),
        slot->name);
}

static void maybe_insert_default_mv(lua_State *L) {
    if ((lua_gettop(L) >= 1 && lua_type(L, 1) != LUA_TSTRING) || lua_gettop(L) == 0) {
        lua_pushstring(L, am_conf_default_modelview_matrix_name);
        lua_insert(L, 1);
    }
}

/* Translate */

void am_translate_node::render(am_render_state *rstate) {
    am_program_param_value *param = &rstate->param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::dmat4 *m = (glm::dmat4*)&param->value.m4[0];
        glm::dvec4 old_column = (*m)[3];
        (*m)[3] = (*m)[0] * v[0] + (*m)[1] * v[1] + (*m)[2] * v[2] + (*m)[3];
        render_children(rstate);
        (*m)[3] = old_column;
    } else {
        log_ignored_transform(rstate, name, "translate");
        render_children(rstate);
    }
}

static int create_translate_node(lua_State *L) {
    maybe_insert_default_mv(L);
    int nargs = am_check_nargs(L, 2);
    am_translate_node *node = am_new_userdata(L, am_translate_node);
    node->tags.push_back(L, AM_TAG_TRANSLATE);
    node->name = am_lookup_param_name(L, 1);
    switch (am_get_type(L, 2)) {
        case MT_am_vec2: {
            glm::dvec2 v2 = am_get_userdata(L, am_vec2, 2)->v;
            node->v = glm::dvec3(v2.x, v2.y, 0.0);
            break;
        }
        case MT_am_vec3: {
            node->v = am_get_userdata(L, am_vec3, 2)->v;
            break;
        }
        case LUA_TNUMBER: {
            if (nargs < 3) {
                return luaL_error(L, "too few arguments");
            } else if (nargs == 3) {
                node->v = glm::dvec3((double)luaL_checknumber(L, 2), (double)luaL_checknumber(L, 3), 0.0);
            } else if (nargs == 4) {
                node->v = glm::dvec3((double)luaL_checknumber(L, 2), (double)luaL_checknumber(L, 3), (double)luaL_checknumber(L, 4));
            } else {
                return luaL_error(L, "too many arguments");
            }
            break;
        }
        default:
            return luaL_error(L, "expecting a vec2 or vec3 argument");
    }
    return 1;
}

static void get_position(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    am_new_userdata(L, am_vec3)->v = node->v;
}

static void set_position(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    node->v = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property position_property = {get_position, set_position};

static void get_position2d(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    am_new_userdata(L, am_vec2)->v = glm::dvec2(node->v.x, node->v.y);
}

static void set_position2d(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    node->v = glm::dvec3(am_get_userdata(L, am_vec2, 3)->v, 0.0);
}

static am_property position2d_property = {get_position2d, set_position2d};

static void get_position_x(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    lua_pushnumber(L, node->v.x);
}

static void set_position_x(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    node->v.x = luaL_checknumber(L, 3);
}

static am_property position_x_property = {get_position_x, set_position_x};

static void get_position_y(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    lua_pushnumber(L, node->v.y);
}

static void set_position_y(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    node->v.y = luaL_checknumber(L, 3);
}

static am_property position_y_property = {get_position_y, set_position_y};

static void get_position_z(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    lua_pushnumber(L, node->v.z);
}

static void set_position_z(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    node->v.z = luaL_checknumber(L, 3);
}

static am_property position_z_property = {get_position_z, set_position_z};

static void register_translate_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "position", &position_property);
    am_register_property(L, "position2d", &position2d_property);
    am_register_property(L, "x", &position_x_property);
    am_register_property(L, "y", &position_y_property);
    am_register_property(L, "z", &position_z_property);

    am_register_metatable(L, "translate", MT_am_translate_node, MT_am_scene_node);
}

/* Scale */

void am_scale_node::render(am_render_state *rstate) {
    am_program_param_value *param = &rstate->param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::dmat4 *m = (glm::dmat4*)&param->value.m4[0];
        glm::dmat4 old = *m;
        *m = glm::scale(*m, v);
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(rstate, name, "scale");
        render_children(rstate);
    }
}

static int create_scale_node(lua_State *L) {
    maybe_insert_default_mv(L);
    int nargs = am_check_nargs(L, 2);
    am_scale_node *node = am_new_userdata(L, am_scale_node);
    node->tags.push_back(L, AM_TAG_SCALE);
    node->name = am_lookup_param_name(L, 1);
    switch (am_get_type(L, 2)) {
        case MT_am_vec2: {
            glm::dvec2 v2 = am_get_userdata(L, am_vec2, 2)->v;
            node->v = glm::dvec3(v2.x, v2.y, 1.0);
            break;
        }
        case MT_am_vec3: {
            node->v = am_get_userdata(L, am_vec3, 2)->v;
            break;
        }
        case LUA_TNUMBER: {
            if (nargs == 2) {
                double s = (double)luaL_checknumber(L, 2);
                node->v = glm::dvec3(s, s, 1.0);
            } else if (nargs == 3) {
                node->v = glm::dvec3((double)luaL_checknumber(L, 2), (double)luaL_checknumber(L, 3), 1.0);
            } else if (nargs == 4) {
                node->v = glm::dvec3((double)luaL_checknumber(L, 2), (double)luaL_checknumber(L, 3), (double)luaL_checknumber(L, 4));
            } else {
                return luaL_error(L, "too many arguments");
            }
            break;
        }
        default:
            return luaL_error(L, "expecting a vec2 or vec3 argument");
    }
    return 1;
}

static void get_scale(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    am_new_userdata(L, am_vec3)->v = node->v;
}

static void set_scale(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    node->v = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property scale_property = {get_scale, set_scale};

static void get_scale2d(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    am_new_userdata(L, am_vec2)->v = glm::dvec2(node->v.x, node->v.y);
}

static void set_scale2d(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    node->v = glm::dvec3(am_get_userdata(L, am_vec2, 3)->v, 1.0);
}

static am_property scale2d_property = {get_scale2d, set_scale2d};

static void get_scale_x(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    lua_pushnumber(L, node->v.x);
}

static void set_scale_x(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    node->v.x = luaL_checknumber(L, 3);
}

static am_property scale_x_property = {get_scale_x, set_scale_x};

static void get_scale_y(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    lua_pushnumber(L, node->v.y);
}

static void set_scale_y(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    node->v.y = luaL_checknumber(L, 3);
}

static am_property scale_y_property = {get_scale_y, set_scale_y};

static void get_scale_z(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    lua_pushnumber(L, node->v.z);
}

static void set_scale_z(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    node->v.z = luaL_checknumber(L, 3);
}

static am_property scale_z_property = {get_scale_z, set_scale_z};

static void register_scale_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "scale", &scale_property);
    am_register_property(L, "scale2d", &scale2d_property);
    am_register_property(L, "x", &scale_x_property);
    am_register_property(L, "y", &scale_y_property);
    am_register_property(L, "z", &scale_z_property);

    am_register_metatable(L, "scale", MT_am_scale_node, MT_am_scene_node);
}

/* Rotate */

void am_rotate_node::render(am_render_state *rstate) {
    am_program_param_value *param = &rstate->param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::dmat4 *m = (glm::dmat4*)&param->value.m4[0];
        glm::dmat4 old = *m;
        *m *= glm::mat4_cast(rotation);
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(rstate, name, "rotate");
        render_children(rstate);
    }
}

static int create_rotate_node(lua_State *L) {
    maybe_insert_default_mv(L);
    int nargs = am_check_nargs(L, 2);
    am_rotate_node *node = am_new_userdata(L, am_rotate_node);
    node->tags.push_back(L, AM_TAG_ROTATE);
    node->name = am_lookup_param_name(L, 1);
    switch (am_get_type(L, 2)) {
        case MT_am_quat:
            node->rotation = am_get_userdata(L, am_quat, 2)->q;
            node->angle = glm::angle(node->rotation);
            node->axis = glm::axis(node->rotation);
            break;
        default:
            node->angle = luaL_checknumber(L, 2);
            if (nargs > 2) {
                node->axis = am_get_userdata(L, am_vec3, 3)->v;
            } else {
                node->axis = glm::dvec3(0.0, 0.0, 1.0);
            }
            node->rotation = glm::angleAxis(node->angle, node->axis);
            break;
    }
    return 1;
}

static void get_rotation(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    am_new_userdata(L, am_quat)->q = node->rotation;
}

static void set_rotation(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    node->rotation = am_get_userdata(L, am_quat, 3)->q;
}

static am_property rotation_property = {get_rotation, set_rotation};

static void get_angle(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    lua_pushnumber(L, node->angle);
}

static void set_angle(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    node->angle = luaL_checknumber(L, 3);
    node->rotation = glm::angleAxis(node->angle, node->axis);
}

static am_property angle_property = {get_angle, set_angle};

static void get_axis(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    am_new_userdata(L, am_vec3)->v = node->axis;
}

static void set_axis(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    node->axis = am_get_userdata(L, am_vec3, 3)->v;
    node->rotation = glm::angleAxis(node->angle, node->axis);
}

static am_property axis_property = {get_axis, set_axis};

static void register_rotate_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "rotation", &rotation_property);
    am_register_property(L, "angle", &angle_property);
    am_register_property(L, "axis", &axis_property);

    am_register_metatable(L, "rotate", MT_am_rotate_node, MT_am_scene_node);
}

/* Transform */

void am_transform_node::render(am_render_state *rstate) {
    am_program_param_value *param = &rstate->param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::dmat4 *m = (glm::dmat4*)&param->value.m4[0];
        glm::dmat4 old = *m;
        *m = (*m) * mat;
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(rstate, name, "transform");
        render_children(rstate);
    }
}

static int create_transform_node(lua_State *L) {
    maybe_insert_default_mv(L);
    am_check_nargs(L, 2);
    am_transform_node *node = am_new_userdata(L, am_transform_node);
    node->tags.push_back(L, AM_TAG_TRANSFORM);
    node->name = am_lookup_param_name(L, 1);
    node->mat = am_get_userdata(L, am_mat4, 2)->m;
    return 1;
}

static void get_transform_mat(lua_State *L, void *obj) {
    am_transform_node *node = (am_transform_node*)obj;
    am_new_userdata(L, am_mat4)->m = node->mat;
}

static void set_transform_mat(lua_State *L, void *obj) {
    am_transform_node *node = (am_transform_node*)obj;
    node->mat = am_get_userdata(L, am_mat4, 3)->m;
}

static am_property transform_mat_property = {get_transform_mat, set_transform_mat};

static void register_transform_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "mat", &transform_mat_property);

    am_register_metatable(L, "transform", MT_am_transform_node, MT_am_scene_node);
}

/* Lookat */

void am_lookat_node::render(am_render_state *rstate) {
    am_program_param_value *param = &rstate->param_name_map[name].value;
    am_program_param_value old_val = *param;
    param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4;
    memcpy(&param->value.m4[0], glm::value_ptr(glm::lookAt(eye, center, up)), 16 * sizeof(double));
    render_children(rstate);
    *param = old_val;
}

static int create_lookat_node(lua_State *L) {
    maybe_insert_default_mv(L);
    am_check_nargs(L, 4);
    am_lookat_node *node = am_new_userdata(L, am_lookat_node);
    node->tags.push_back(L, AM_TAG_LOOKAT);
    node->name = am_lookup_param_name(L, 1);
    node->eye = am_get_userdata(L, am_vec3, 2)->v;
    node->center = am_get_userdata(L, am_vec3, 3)->v;
    node->up = am_get_userdata(L, am_vec3, 4)->v;
    return 1;
}

static void get_lookat_eye(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    am_vec3 *eye = am_new_userdata(L, am_vec3);
    eye->v = node->eye;
}

static void set_lookat_eye(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    node->eye = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property lookat_eye_property = {get_lookat_eye, set_lookat_eye};

static void get_lookat_center(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    am_vec3 *center = am_new_userdata(L, am_vec3);
    center->v = node->center;
}

static void set_lookat_center(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    node->center = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property lookat_center_property = {get_lookat_center, set_lookat_center};

static void get_lookat_up(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    am_vec3 *up = am_new_userdata(L, am_vec3);
    up->v = node->up;
}

static void set_lookat_up(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    node->up = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property lookat_up_property = {get_lookat_up, set_lookat_up};

static void register_lookat_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "eye", &lookat_eye_property);
    am_register_property(L, "center", &lookat_center_property);
    am_register_property(L, "up", &lookat_up_property);

    am_register_metatable(L, "lookat", MT_am_lookat_node, MT_am_scene_node);
}

/* Billboard */

void am_billboard_node::render(am_render_state *rstate) {
    am_program_param_value *param = &rstate->param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::dmat4 *m = (glm::dmat4*)&param->value.m4[0];
        glm::dmat4 old = *m;
        double s = 1.0;
        if (preserve_uniform_scaling) {
            s = glm::length(glm::dvec3((*m)[0][0], (*m)[1][0], (*m)[2][0]));
        }
        (*m)[0][0] = s;
        (*m)[0][1] = 0.0;
        (*m)[0][2] = 0.0;
        (*m)[1][0] = 0.0;
        (*m)[1][1] = s;
        (*m)[1][2] = 0.0;
        (*m)[2][0] = 0.0;
        (*m)[2][1] = 0.0;
        (*m)[2][2] = s;
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(rstate, name, "billboard");
        render_children(rstate);
    }
}

static int create_billboard_node(lua_State *L) {
    maybe_insert_default_mv(L);
    int nargs = am_check_nargs(L, 1);
    am_billboard_node *node = am_new_userdata(L, am_billboard_node);
    node->tags.push_back(L, AM_TAG_BILLBOARD);
    node->name = am_lookup_param_name(L, 1);
    node->preserve_uniform_scaling = false;
    if (nargs > 1) {
        node->preserve_uniform_scaling = lua_toboolean(L, 2);
    }
    return 1;
}

static void register_billboard_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_metatable(L, "billboard", MT_am_billboard_node, MT_am_scene_node);
}

/* Module init */

void am_open_transforms_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"translate", create_translate_node},
        {"rotate", create_rotate_node},
        {"scale", create_scale_node},
        {"transform", create_transform_node},
        {"billboard", create_billboard_node},
        {"lookat", create_lookat_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_translate_node_mt(L);
    register_scale_node_mt(L);
    register_rotate_node_mt(L);
    register_transform_node_mt(L);
    register_lookat_node_mt(L);
    register_billboard_node_mt(L);
}
