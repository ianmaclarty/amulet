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
        glm::vec3 v = position->v;
        (*m)[3] = (*m)[0] * v[0] + (*m)[1] * v[1] + (*m)[2] * v[2] + (*m)[3];
        render_children(rstate);
        (*m)[3] = old_column;
    } else {
        log_ignored_transform(name, "translate");
        render_children(rstate);
    }
}

int am_create_translate_node(lua_State *L) {
    am_check_nargs(L, 3);
    if (lua_type(L, 2) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 2");
    am_translate_node *node = am_new_userdata(L, am_translate_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->position = am_get_userdata(L, am_vec3, 3);
    node->position_ref = node->ref(L, 3);
    return 1;
}

static void get_position(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    node->pushref(L, node->position_ref);
}

static void set_position(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    node->position = am_get_userdata(L, am_vec3, 3);
    node->reref(L, node->position_ref, 3);
}

static am_property position_property = {get_position, set_position};

static void register_translate_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "position", &position_property);

    am_register_metatable(L, "translate", MT_am_translate_node, MT_am_scene_node);
}

/* Scale */

void am_scale_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
        glm::mat4 old = *m;
        glm::vec3 v = scaling->v;
        *m = glm::scale(*m, v);
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(name, "scale");
        render_children(rstate);
    }
}

int am_create_scale_node(lua_State *L) {
    am_check_nargs(L, 3);
    if (lua_type(L, 2) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 2");
    am_scale_node *node = am_new_userdata(L, am_scale_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->scaling = am_get_userdata(L, am_vec3, 3);
    node->scaling_ref = node->ref(L, 3);
    return 1;
}

static void get_scaling(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    node->pushref(L, node->scaling_ref);
}

static void set_scaling(lua_State *L, void *obj) {
    am_scale_node *node = (am_scale_node*)obj;
    node->scaling = am_get_userdata(L, am_vec3, 3);
    node->reref(L, node->scaling_ref, 3);
}

static am_property scaling_property = {get_scaling, set_scaling};

static void register_scale_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "scaling", &scaling_property);

    am_register_metatable(L, "scale", MT_am_scale_node, MT_am_scene_node);
}

/* Rotate */

void am_rotate_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
        glm::mat4 old = *m;
        *m *= glm::mat4_cast(rotation->q);
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(name, "rotate");
        render_children(rstate);
    }
}

int am_create_rotate_node(lua_State *L) {
    am_check_nargs(L, 3);
    if (lua_type(L, 2) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 2");
    am_rotate_node *node = am_new_userdata(L, am_rotate_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->rotation = am_get_userdata(L, am_quat, 3);
    node->rotation_ref = node->ref(L, 3);
    return 1;
}

static void get_rotation(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    node->pushref(L, node->rotation_ref);
}

static void set_rotation(lua_State *L, void *obj) {
    am_rotate_node *node = (am_rotate_node*)obj;
    node->rotation = am_get_userdata(L, am_quat, 3);
    node->reref(L, node->rotation_ref, 3);
}

static am_property rotation_property = {get_rotation, set_rotation};

static void register_rotate_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "rotation", &rotation_property);

    am_register_metatable(L, "rotate", MT_am_rotate_node, MT_am_scene_node);
}

/* Lookat */

void am_lookat_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    am_program_param_value old_val = *param;
    param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4;
    memcpy(&param->value.m4[0], glm::value_ptr(glm::lookAt(eye->v, center->v, up->v)), 16 * sizeof(float));
    render_children(rstate);
    *param = old_val;
}

int am_create_lookat_node(lua_State *L) {
    am_check_nargs(L, 5);
    if (lua_type(L, 2) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 2");
    am_lookat_node *node = am_new_userdata(L, am_lookat_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->eye = am_get_userdata(L, am_vec3, 3);
    node->eye_ref = node->ref(L, 3);
    node->center = am_get_userdata(L, am_vec3, 4);
    node->center_ref = node->ref(L, 4);
    node->up = am_get_userdata(L, am_vec3, 5);
    node->up_ref = node->ref(L, 5);
    return 1;
}

static void get_lookat_eye(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    node->pushref(L, node->eye_ref);
}

static void set_lookat_eye(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    node->eye = am_get_userdata(L, am_vec3, 3);
    node->reref(L, node->eye_ref, 3);
}

static am_property lookat_eye_property = {get_lookat_eye, set_lookat_eye};

static void get_lookat_center(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    node->pushref(L, node->center_ref);
}

static void set_lookat_center(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    node->center = am_get_userdata(L, am_vec3, 3);
    node->reref(L, node->center_ref, 3);
}

static am_property lookat_center_property = {get_lookat_center, set_lookat_center};

static void get_lookat_up(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    node->pushref(L, node->up_ref);
}

static void set_lookat_up(lua_State *L, void *obj) {
    am_lookat_node *node = (am_lookat_node*)obj;
    node->up = am_get_userdata(L, am_vec3, 3);
    node->reref(L, node->up_ref, 3);
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
    am_program_param_value *param = &am_param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
        glm::mat4 old = *m;
        float s = 1.0f;
        if (preserve_uniform_scaling) {
            s = glm::length(glm::vec3((*m)[0][0], (*m)[1][0], (*m)[2][0]));
        }
        (*m)[0][0] = s;
        (*m)[0][1] = 0.0f;
        (*m)[0][2] = 0.0f;
        (*m)[1][0] = 0.0f;
        (*m)[1][1] = s;
        (*m)[1][2] = 0.0f;
        (*m)[2][0] = 0.0f;
        (*m)[2][1] = 0.0f;
        (*m)[2][2] = s;
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(name, "billboard");
        render_children(rstate);
    }
}

int am_create_billboard_node(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    if (lua_type(L, 2) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 2");
    am_billboard_node *node = am_new_userdata(L, am_billboard_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->preserve_uniform_scaling = false;
    if (nargs > 2) {
        node->preserve_uniform_scaling = lua_toboolean(L, 3);
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
    register_translate_node_mt(L);
    register_scale_node_mt(L);
    register_rotate_node_mt(L);
    register_lookat_node_mt(L);
    register_billboard_node_mt(L);
}
