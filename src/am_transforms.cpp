#include "amulet.h"

/* Translate */

static void log_ignored_transform(am_param_name_id name, const char *transform) {
    am_program_param_name_slot *slot = &am_param_name_map[name];
    am_log1("WARNING: ignoring %s on %s '%s' (expecting a mat4)",
        transform,
        am_program_param_client_type_name(slot),
        slot->name);
}

/* Translate */

void am_translate_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
        glm::vec4 old_column = (*m)[3];
        (*m)[3] = (*m)[0] * v[0] + (*m)[1] * v[1] + (*m)[2] * v[2] + (*m)[3];
        render_children(rstate);
        (*m)[3] = old_column;
    } else {
        log_ignored_transform(name, "translate");
        render_children(rstate);
    }
}

int am_translate_node::specialized_index(lua_State *L) {
    return am_vec3_index(L, &v);
}

int am_translate_node::specialized_newindex(lua_State *L) {
    return am_vec3_newindex(L, &v);
}

int am_create_translate_node(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    am_translate_node *node = am_new_userdata(L, am_translate_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    am_read_vec3(L, &node->v, 3, nargs);
    return 1;
}

static void register_translate_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "translate");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_translate_node, MT_am_scene_node);
}

/* Scale */

void am_scale_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
        glm::mat4 old = *m;
        *m = glm::scale(*m, v);
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(name, "scale");
        render_children(rstate);
    }
}

int am_scale_node::specialized_index(lua_State *L) {
    return am_vec3_index(L, &v);
}

int am_scale_node::specialized_newindex(lua_State *L) {
    return am_vec3_newindex(L, &v);
}

int am_create_scale_node(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    am_scale_node *node = am_new_userdata(L, am_scale_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->v = glm::vec3(1.0f);
    am_read_vec3(L, &node->v, 3, nargs);
    return 1;
}

static void register_scale_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "scale");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_scale_node, MT_am_scene_node);
}

/* Rotate */

void am_rotate_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
        glm::mat4 old = *m;
        *m = glm::rotate(*m, angle, about);
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(name, "rotate");
        render_children(rstate);
    }
}

int am_rotate_node::specialized_index(lua_State *L) {
    return am_vec3_index(L, &about);
}

int am_rotate_node::specialized_newindex(lua_State *L) {
    return am_vec3_newindex(L, &about);
}

int am_create_rotate_node(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    am_rotate_node *node = am_new_userdata(L, am_rotate_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->angle = 0.0;
    node->about = glm::vec3(0.0f, 0.0f, 1.0f);
    if (nargs > 2) {
        node->angle = luaL_checknumber(L, 3);
        am_read_vec3(L, &node->about, 4, nargs);
    }
    return 1;
}

static void get_rotate_angle(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    lua_pushnumber(L, node->angle);
}

static void set_rotate_angle(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    node->angle = luaL_checknumber(L, 3);
}

static am_property rotate_angle_property = {get_rotate_angle, set_rotate_angle};

static void register_rotate_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");

    am_register_property(L, "angle", &rotate_angle_property);

    lua_pushstring(L, "rotate");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_rotate_node, MT_am_scene_node);
}

/* Multiply */

void am_mult_mat4_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
        glm::mat4 old = *m;
        *m = *m * mat;
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(name, "mult_mat4");
        render_children(rstate);
    }
}

int am_create_mult_mat4_node(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    am_mult_mat4_node *node = am_new_userdata(L, am_mult_mat4_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    if (nargs > 2) {
        node->mat = am_get_userdata(L, am_mat4, 3)->m;
    }
    return 1;
}

static void get_mult_mat4_mat4(lua_State *L, void *obj) {
    am_mult_mat4_node *node = (am_mult_mat4_node*)obj;
    am_mat4 *mat = am_new_userdata(L, am_mat4);
    mat->m = node->mat;
}

static void set_mult_mat4_mat4(lua_State *L, void *obj) {
    am_mult_mat4_node *node = (am_mult_mat4_node*)obj;
    node->mat = am_get_userdata(L, am_mat4, 3)->m;
}

static am_property mult_mat4_mat4_property = {get_mult_mat4_mat4, set_mult_mat4_mat4};

static void register_mult_mat4_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");

    am_register_property(L, "mat4", &mult_mat4_mat4_property);

    lua_pushstring(L, "mult_mat4");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_mult_mat4_node, MT_am_scene_node);
}

/* Module init */

void am_open_transforms_module(lua_State *L) {
    register_translate_node_mt(L);
    register_scale_node_mt(L);
    register_rotate_node_mt(L);
    register_mult_mat4_node_mt(L);
}
