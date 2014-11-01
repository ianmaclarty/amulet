#include "amulet.h"

void am_translate_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    switch (param->type) {
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2: {
            glm::mat2 *m = (glm::mat2*)&param->value.m2[0];
            glm::vec2 old_column = (*m)[1];
            (*m)[1] = (*m)[0] * v[0] + (*m)[1];
            render_children(rstate);
            (*m)[1] = old_column;
            break;
        }
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3: {
            glm::mat3 *m = (glm::mat3*)&param->value.m3[0];
            glm::vec3 old_column = (*m)[2];
            (*m)[2] = (*m)[0] * v[0] + (*m)[1] * v[1] + (*m)[2];
            render_children(rstate);
            (*m)[2] = old_column;
            break;
        }
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4: {
            glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
            glm::vec4 old_column = (*m)[3];
            (*m)[3] = (*m)[0] * v[0] + (*m)[1] * v[1] + (*m)[2] * v[2] + (*m)[3];
            render_children(rstate);
            (*m)[3] = old_column;
            break;
        }
        default:
            // ignore translation
            render_children(rstate);
    }
}

int am_translate_node::specialized_index(lua_State *L) {
    return am_vec3_index(L, &v);
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

void am_open_transforms_module(lua_State *L) {
    register_translate_node_mt(L);
}
