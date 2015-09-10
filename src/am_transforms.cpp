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

static int create_translate_node(lua_State *L) {
    am_check_nargs(L, 2);
    if (lua_type(L, 1) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 1");
    am_translate_node *node = am_new_userdata(L, am_translate_node);
    node->tags.push_back(L, AM_TAG_TRANSLATE);
    node->name = am_lookup_param_name(L, 1);
    node->v = am_get_userdata(L, am_vec3, 2)->v;
    return 1;
}

static void get_translation(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    am_new_userdata(L, am_vec3)->v = node->v;
}

static void set_translation(lua_State *L, void *obj) {
    am_translate_node *node = (am_translate_node*)obj;
    node->v = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property translation_property = {get_translation, set_translation};

static void register_translate_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "position", &translation_property);

    am_register_metatable(L, "translate", MT_am_translate_node, MT_am_scene_node);
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

static int create_scale_node(lua_State *L) {
    am_check_nargs(L, 2);
    if (lua_type(L, 1) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 1");
    am_scale_node *node = am_new_userdata(L, am_scale_node);
    node->tags.push_back(L, AM_TAG_SCALE);
    node->name = am_lookup_param_name(L, 1);
    node->v = am_get_userdata(L, am_vec3, 2)->v;
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

static void register_scale_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "scale", &scale_property);

    am_register_metatable(L, "scale", MT_am_scale_node, MT_am_scene_node);
}

/* Rotate */

void am_rotate_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
        glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
        glm::mat4 old = *m;
        *m *= glm::mat4_cast(rotation);
        render_children(rstate);
        *m = old;
    } else {
        log_ignored_transform(name, "rotate");
        render_children(rstate);
    }
}

static int create_rotate_node(lua_State *L) {
    am_check_nargs(L, 2);
    if (lua_type(L, 1) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 1");
    am_rotate_node *node = am_new_userdata(L, am_rotate_node);
    node->tags.push_back(L, AM_TAG_ROTATE);
    node->name = am_lookup_param_name(L, 1);
    node->rotation = am_get_userdata(L, am_quat, 2)->q;
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
    memcpy(&param->value.m4[0], glm::value_ptr(glm::lookAt(eye, center, up)), 16 * sizeof(float));
    render_children(rstate);
    *param = old_val;
}

static int create_lookat_node(lua_State *L) {
    am_check_nargs(L, 4);
    if (lua_type(L, 1) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 1");
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

static int create_billboard_node(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    if (lua_type(L, 1) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 1");
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
        {"billboard", create_billboard_node},
        {"lookat", create_lookat_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_translate_node_mt(L);
    register_scale_node_mt(L);
    register_rotate_node_mt(L);
    register_lookat_node_mt(L);
    register_billboard_node_mt(L);
}
