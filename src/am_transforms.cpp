#include "amulet.h"

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
        // ignore translation
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
    am_check_nargs(L, 3);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    am_vec3 *v = am_get_userdata(L, am_vec3, 3);
    am_translate_node *node = am_new_userdata(L, am_translate_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->v = v->v;
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
        // ignore translation
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
    am_check_nargs(L, 3);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    am_vec3 *v = am_get_userdata(L, am_vec3, 3);
    am_scale_node *node = am_new_userdata(L, am_scale_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->v = v->v;
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

/* Module init */

void am_open_transforms_module(lua_State *L) {
    register_translate_node_mt(L);
    register_scale_node_mt(L);
}
