#include "amulet.h"

static void bind_attribute_array(am_render_state *rstate,
    am_gluint location, am_attribute_client_type type, int dims, am_buffer_view *view)
{
    am_set_attribute_array_enabled(location, true);
    am_buffer *buf = view->buffer;
    if (buf->vbo_id == 0) buf->create_vbo();
    buf->update_if_dirty();
    am_bind_buffer(AM_ARRAY_BUFFER, buf->vbo_id);
    am_set_attribute_pointer(location, dims, type, view->normalized, view->stride, view->offset);
    if (view->size < rstate->max_draw_array_size) {
        rstate->max_draw_array_size = view->size;
    }
}

static void bind_sampler2d(am_render_state *rstate,
    am_gluint location, int texture_unit, am_texture2d *texture)
{
    if (texture->buffer != NULL) {
        texture->buffer->update_if_dirty();
    }
    am_set_active_texture_unit(texture_unit);
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, texture->texture_id);
    am_set_uniform1i(location, texture_unit);
}

/*
static void report_incompatible_param_type(am_program_param *param, am_program_param_client_type ctype) {
    am_program_param_name_slot *slot = &am_param_name_map[param->name];
}
*/

void am_program_param::bind(am_render_state *rstate) {
    am_program_param_name_slot *slot = &am_param_name_map[name];
    switch (type) {
        case AM_PROGRAM_PARAM_UNIFORM_1F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_1F) {
                am_set_uniform1f(location, slot->value.value.f);
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_2F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_2F) {
                am_set_uniform2f(location, slot->value.value.v2);
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_3F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_3F) {
                am_set_uniform3f(location, slot->value.value.v3);
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_4F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_4F) {
                am_set_uniform4f(location, slot->value.value.v4);
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_MAT2:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2) {
                am_set_uniform_mat2(location, slot->value.value.m2);
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_MAT3:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3) {
                am_set_uniform_mat3(location, slot->value.value.m3);
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_MAT4:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
                am_set_uniform_mat4(location, slot->value.value.m4);
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_SAMPLER2D:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
                bind_sampler2d(rstate, location, slot->value.value.sampler2d.texture_unit, slot->value.value.sampler2d.texture);
            }
            break;
        case AM_PROGRAM_PARAM_ATTRIBUTE_1F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_1F) {
                am_set_attribute_array_enabled(location, false);
                am_set_attribute1f(location, slot->value.value.f);
            } else if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY && slot->value.value.arr->type == AM_BUF_ELEM_TYPE_FLOAT) {
                bind_attribute_array(rstate, location, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT, 1, slot->value.value.arr);
            }
            break;
        case AM_PROGRAM_PARAM_ATTRIBUTE_2F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_2F) {
                am_set_attribute_array_enabled(location, false);
                am_set_attribute2f(location, slot->value.value.v2);
            } else if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY && slot->value.value.arr->type == AM_BUF_ELEM_TYPE_FLOAT2) {
                bind_attribute_array(rstate, location, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT, 2, slot->value.value.arr);
            }
            break;
        case AM_PROGRAM_PARAM_ATTRIBUTE_3F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_3F) {
                am_set_attribute_array_enabled(location, false);
                am_set_attribute3f(location, slot->value.value.v3);
            } else if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY && slot->value.value.arr->type == AM_BUF_ELEM_TYPE_FLOAT3) {
                bind_attribute_array(rstate, location, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT, 3, slot->value.value.arr);
            }
            break;
        case AM_PROGRAM_PARAM_ATTRIBUTE_4F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_4F) {
                am_set_attribute_array_enabled(location, false);
                am_set_attribute4f(location, slot->value.value.v4);
            } else if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY && slot->value.value.arr->type == AM_BUF_ELEM_TYPE_FLOAT4) {
                bind_attribute_array(rstate, location, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT, 4, slot->value.value.arr);
            }
            break;
    }
}

static int am_param_name_map_capacity = 0;

am_program_param_name_slot *am_param_name_map = NULL;

void am_init_param_name_map(lua_State *L) {
    if (am_param_name_map != NULL) free(am_param_name_map);
    am_param_name_map_capacity = 32;
    am_param_name_map = (am_program_param_name_slot*)malloc(sizeof(am_program_param_name_slot) * am_param_name_map_capacity);
    for (int i = 0; i < am_param_name_map_capacity; i++) {
        am_param_name_map[i].name = NULL;
        am_param_name_map[i].value.type = AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED;
    }
    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_PARAM_NAME_STRING_TABLE);
}

am_param_name_id am_lookup_param_name(lua_State *L, int name_idx) {
    name_idx = am_absindex(L, name_idx);
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_PARAM_NAME_STRING_TABLE);
    int strt_idx = lua_gettop(L);
    lua_pushvalue(L, name_idx);
    lua_rawget(L, strt_idx);
    if (lua_isnil(L, -1)) {
        // param name not seen before, register it.
        lua_pop(L, 1); // nil
        lua_pushvalue(L, name_idx);
        int name_ref = luaL_ref(L, strt_idx);
        lua_pushvalue(L, name_idx);
        lua_pushinteger(L, name_ref);
        lua_rawset(L, strt_idx);
        lua_pop(L, 1); // string table
        if (name_ref >= am_param_name_map_capacity) {
            int old_capacity = am_param_name_map_capacity;
            while (name_ref >= am_param_name_map_capacity) {
                am_param_name_map_capacity *= 2;
            }
            am_param_name_map = (am_program_param_name_slot*)realloc(am_param_name_map, sizeof(am_program_param_name_slot) * am_param_name_map_capacity);
            for (int i = old_capacity; i < am_param_name_map_capacity; i++) {
                am_param_name_map[i].name = NULL;
            }
            am_param_name_map[name_ref].name = lua_tostring(L, name_idx);
        }
        return name_ref;
    } else {
        int name_ref = lua_tointeger(L, -1);
        lua_pop(L, 2); // name ref, string table
        return name_ref;
    }
}

am_shader_id load_shader(lua_State *L, am_shader_type type, const char *src) {
    am_shader_id shader = am_create_shader(type);
    if (shader == 0) {
        lua_pushstring(L, "unable to create new shader");
        return 0;
    }

    char *msg = NULL;
    char *line_str = NULL;
    int line_no = -1;
    bool compiled = am_compile_shader(shader, type, src, &msg, &line_no, &line_str);
    if (!compiled) {
        assert(msg != NULL);
        const char *type_str = "<unknown>";
        switch (type) {
            case AM_VERTEX_SHADER: type_str = "vertex"; break;
            case AM_FRAGMENT_SHADER: type_str = "fragment"; break;
        }
        if (line_str != NULL && line_no > 0) {
            const char *nl = "";
            if (strlen(msg) > 0 && msg[strlen(msg)-1] != '\n') nl = "\n";
            lua_pushfstring(L, "%s shader compilation error:\n%s%s[%d: %s]", type_str, msg, nl, line_no, line_str);
            free((void*)line_str);
        } else {
            lua_pushfstring(L, "%s shader compilation error:\n%s", type_str, msg);
        }
        free((void*)msg);
        am_delete_shader(shader);
        return 0;
    } else {
        assert(msg == NULL);
    }

   return shader;
}

static int create_program(lua_State *L) {
    if (!am_gl_is_initialized()) {
        return luaL_error(L, "you need to create a window before creating a shader program");
    }
    am_check_nargs(L, 2);
    const char *vertex_shader_src = lua_tostring(L, 1);
    const char *fragment_shader_src = lua_tostring(L, 2);
    if (vertex_shader_src == NULL) {
        return luaL_error(L, "expecting vertex shader source string in position 1");
    }
    if (fragment_shader_src == NULL) {
        return luaL_error(L, "expecting fragment shader source string in position 2");
    }

    am_shader_id vertex_shader = load_shader(L, AM_VERTEX_SHADER, vertex_shader_src);
    if (vertex_shader == 0) {
        return luaL_error(L, lua_tostring(L, -1));
    }
    am_shader_id fragment_shader = load_shader(L, AM_FRAGMENT_SHADER, fragment_shader_src);
    if (fragment_shader == 0) {
        am_delete_shader(vertex_shader);
        return luaL_error(L, lua_tostring(L, -1));
    }

    am_program_id program = am_create_program();
    if (program == 0) {
        am_delete_shader(vertex_shader);
        am_delete_shader(fragment_shader);
        return luaL_error(L, "unable to create shader program");
    }

    am_attach_shader(program, vertex_shader);
    am_attach_shader(program, fragment_shader);
    bool linked = am_link_program(program);

    am_delete_shader(vertex_shader);
    am_delete_shader(fragment_shader);

    if (!linked) {
        char *msg = am_get_program_info_log(program);
        lua_pushfstring(L, "shader program link error:\n%s", msg);
        free(msg);
        am_delete_program(program);
        return luaL_error(L, lua_tostring(L, -1));
    }

    int num_attributes = am_get_program_active_attributes(program);
    int num_uniforms = am_get_program_active_uniforms(program);

    int num_params = num_attributes + num_uniforms;    

    am_program_param *params = (am_program_param*)malloc(sizeof(am_program_param) * num_params);

    // Generate attribute params
    int i = 0;
    for (int index = 0; index < num_attributes; index++) {
        char *name_str;
        am_attribute_var_type type;
        int size;
        am_get_active_attribute(program, index, &name_str, &type, &size);
        lua_pushstring(L, name_str);
        int name = am_lookup_param_name(L, -1);
        lua_pop(L, 1); // name str
        am_program_param *param = &params[i];
        param->location = am_get_attribute_location(program, name_str);
        param->name = name;
        switch (type) {
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT:
                param->type = AM_PROGRAM_PARAM_ATTRIBUTE_1F;
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC2:
                param->type = AM_PROGRAM_PARAM_ATTRIBUTE_2F;
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC3:
                param->type = AM_PROGRAM_PARAM_ATTRIBUTE_3F;
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC4:
                param->type = AM_PROGRAM_PARAM_ATTRIBUTE_4F;
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT2:
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT3:
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT4:
            case AM_ATTRIBUTE_VAR_TYPE_UNKNOWN:
                am_log(L, 1, false, "WARNING: ignoring attribute '%s' with unsupported type", name_str);
                num_params--;
                free(name_str);
                continue;
        }
        free(name_str);
        i++;
    }

    // Generate uniform params
    for (int index = 0; index < num_uniforms; index++) {
        char *name_str;
        am_uniform_var_type type;
        int size;
        am_get_active_uniform(program, index, &name_str, &type, &size);
        lua_pushstring(L, name_str);
        int name = am_lookup_param_name(L, -1);
        lua_pop(L, 1); // name
        am_program_param *param = &params[i];
        param->location = am_get_uniform_location(program, name_str);
        param->name = name;
        switch (type) {
            case AM_UNIFORM_VAR_TYPE_FLOAT:
                param->type = AM_PROGRAM_PARAM_UNIFORM_1F;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_VEC2:
                param->type = AM_PROGRAM_PARAM_UNIFORM_2F;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_VEC3:
                param->type = AM_PROGRAM_PARAM_UNIFORM_3F;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_VEC4:
                param->type = AM_PROGRAM_PARAM_UNIFORM_4F;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_MAT2:
                param->type = AM_PROGRAM_PARAM_UNIFORM_MAT2;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_MAT3:
                param->type = AM_PROGRAM_PARAM_UNIFORM_MAT3;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_MAT4:
                param->type = AM_PROGRAM_PARAM_UNIFORM_MAT4;
                break;
            case AM_UNIFORM_VAR_TYPE_SAMPLER_2D:
                param->type = AM_PROGRAM_PARAM_UNIFORM_SAMPLER2D;
                break;
            case AM_UNIFORM_VAR_TYPE_INT:
            case AM_UNIFORM_VAR_TYPE_INT_VEC2:
            case AM_UNIFORM_VAR_TYPE_INT_VEC3:
            case AM_UNIFORM_VAR_TYPE_INT_VEC4:
            case AM_UNIFORM_VAR_TYPE_BOOL:
            case AM_UNIFORM_VAR_TYPE_BOOL_VEC2:
            case AM_UNIFORM_VAR_TYPE_BOOL_VEC3:
            case AM_UNIFORM_VAR_TYPE_BOOL_VEC4:
            case AM_UNIFORM_VAR_TYPE_SAMPLER_CUBE:
            case AM_UNIFORM_VAR_TYPE_UNKNOWN:
                am_log(L, 1, false, "WARNING: ignoring uniform '%s' with unsupported type", name_str);
                num_params--;
                free(name_str);
                continue;
        }
        free(name_str);
        i++;
    }

    am_program *prog = am_new_userdata(L, am_program);
    prog->program_id = program;
    prog->num_params = num_params;
    prog->params = params;

    return 1;
}

static int gc_program(lua_State *L) {
    am_program *prog = (am_program*)lua_touserdata(L, 1);
    am_delete_program(prog->program_id);
    free(prog->params);
    return 0;
}

static void register_program_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, gc_program, 0);
    lua_setfield(L, -2, "__gc");
    lua_pushstring(L, "program");
    lua_setfield(L, -2, "tname");
    am_register_metatable(L, MT_am_program, 0);
}

int am_create_program_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_program *prog = am_get_userdata(L, am_program, 2);
    am_program_node *node = am_new_userdata(L, am_program_node);
    am_set_scene_node_child(L, node);
    node->program = prog;
    node->program_ref = node->ref(L, 2);
    return 1;
}

static void register_program_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "program_node");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_program_node, MT_am_scene_node);
}

void am_program_node::render(am_render_state *rstate) {
    am_program* old_program = rstate->active_program;
    rstate->active_program = program;
    render_children(rstate);
    rstate->active_program = old_program;
}

void am_bind_float_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    am_program_param_value old_val = *param;
    param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_1F;
    param->value.f = value;
    render_children(rstate);
    *param = old_val;
}

int am_create_bind_float_node(lua_State *L) {
    am_check_nargs(L, 3);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    float val = luaL_checknumber(L, 3);
    am_bind_float_node *node = am_new_userdata(L, am_bind_float_node);

    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->value = val;

    return 1;
}

static void get_bind_float_node_value(lua_State *L, void *obj) {
    am_bind_float_node *node = (am_bind_float_node*)obj;
    lua_pushnumber(L, node->value);
}

static void set_bind_float_node_value(lua_State *L, void *obj) {
    am_bind_float_node *node = (am_bind_float_node*)obj;
    node->value = luaL_checknumber(L, 3);
}

static am_property bind_float_node_value_property =
    {get_bind_float_node_value, set_bind_float_node_value};

static void register_bind_float_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");

    am_register_property(L, "value", &bind_float_node_value_property);

    lua_pushstring(L, "bind_float");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_bind_float_node, MT_am_scene_node);
}

void am_bind_array_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    am_program_param_value old_val = *param;
    param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY;
    param->value.arr = arr;
    render_children(rstate);
    *param = old_val;
}

int am_create_bind_array_node(lua_State *L) {
    am_check_nargs(L, 3);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 3);
    am_bind_array_node *node = am_new_userdata(L, am_bind_array_node);

    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->arr = view;
    node->arr_ref = node->ref(L, 3); // ref from node to view

    return 1;
}

static void register_bind_array_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "bind_array");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_bind_array_node, MT_am_scene_node);
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
int am_create_bind_mat##D##_node(lua_State *L) {                        \
    am_check_nargs(L, 3);                                               \
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2"); \
    am_mat##D *m = am_get_userdata(L, am_mat##D, 3);                    \
    am_bind_mat##D##_node *node = am_new_userdata(L, am_bind_mat##D##_node); \
    am_set_scene_node_child(L, node);                                                 \
    node->name = am_lookup_param_name(L, 2);                            \
    node->m = m->m;                                                     \
    return 1;                                                           \
}                                                                       \
static void register_bind_mat##D##_node_mt(lua_State *L) {              \
    lua_newtable(L);                                                    \
    lua_pushcclosure(L, am_scene_node_index, 0);                        \
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

#define AM_BIND_VEC_NODE_IMPL(D)                                        \
void am_bind_vec##D##_node::render(am_render_state *rstate) {           \
    am_program_param_value *param = &am_param_name_map[name].value;     \
    am_program_param_value old_val = *param;                            \
    param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_##D##F;                  \
    memcpy(&param->value.v##D[0], glm::value_ptr(v), D * sizeof(float)); \
    render_children(rstate);                                            \
    *param = old_val;                                                   \
}                                                                       \
int am_bind_vec##D##_node::specialized_index(lua_State *L) {            \
    return am_vec##D##_index(L, &v);                                    \
}                                                                       \
int am_bind_vec##D##_node::specialized_newindex(lua_State *L) {         \
    return am_vec##D##_newindex(L, &v);                                 \
}                                                                       \
int am_create_bind_vec##D##_node(lua_State *L) {                        \
    int nargs = am_check_nargs(L, 3);                                   \
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2"); \
    am_bind_vec##D##_node *node = am_new_userdata(L, am_bind_vec##D##_node); \
    am_set_scene_node_child(L, node);                                   \
    node->name = am_lookup_param_name(L, 2);                            \
    am_read_vec##D(L, &node->v, 3, nargs);                              \
    return 1;                                                           \
}                                                                       \
static void register_bind_vec##D##_node_mt(lua_State *L) {              \
    lua_newtable(L);                                                    \
    lua_pushcclosure(L, am_scene_node_index, 0);                        \
    lua_setfield(L, -2, "__index");                                     \
                                                                        \
    lua_pushstring(L, "bind_vec" #D);                                   \
    lua_setfield(L, -2, "tname");                                       \
                                                                        \
    am_register_metatable(L, MT_am_bind_vec##D##_node, MT_am_scene_node);\
}

AM_BIND_VEC_NODE_IMPL(2)
AM_BIND_VEC_NODE_IMPL(3)
AM_BIND_VEC_NODE_IMPL(4)

void am_bind_sampler2d_node::render(am_render_state *rstate) {
    am_program_param_value *param = &am_param_name_map[name].value;
    if (param->type != AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
        am_program_param_value old_val = *param;
        param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D;
        param->value.sampler2d.texture_unit = rstate->next_free_texture_unit++;
        param->value.sampler2d.texture = texture;
        render_children(rstate);
        *param = old_val;
        rstate->next_free_texture_unit--;
    } else {
        // Reuse texture unit
        am_texture2d *old_tex = param->value.sampler2d.texture;
        param->value.sampler2d.texture = texture;
        render_children(rstate);
        param->value.sampler2d.texture = old_tex;
    }
}

int am_create_bind_sampler2d_node(lua_State *L) {
    am_check_nargs(L, 3);
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2");
    am_texture2d *texture = am_get_userdata(L, am_texture2d, 3);
    am_bind_sampler2d_node *node = am_new_userdata(L, am_bind_sampler2d_node);
    am_set_scene_node_child(L, node);
    node->name = am_lookup_param_name(L, 2);
    node->texture = texture;
    node->texture_ref = node->ref(L, 3); // ref from node to texture

    return 1;
}

static void register_bind_sampler2d_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "bind_sampler2d");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_bind_sampler2d_node, MT_am_scene_node);
}


void am_open_program_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"program", create_program},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_program_mt(L);
    register_program_node_mt(L);
    register_bind_float_node_mt(L);
    register_bind_array_node_mt(L);
    register_bind_sampler2d_node_mt(L);
    register_bind_mat2_node_mt(L);
    register_bind_mat3_node_mt(L);
    register_bind_mat4_node_mt(L);
    register_bind_vec2_node_mt(L);
    register_bind_vec3_node_mt(L);
    register_bind_vec4_node_mt(L);
}
