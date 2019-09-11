#include "amulet.h"

static bool bind_attribute_array(am_render_state *rstate, am_gluint location,
    am_buffer_view *view)
{
    if (!view->can_be_gl_attrib()) {
        return false;
    }
    am_buffer *buf = view->buffer;
    if (buf->data == NULL || buf->arraybuf == NULL) {
        return false;
    }
    buf->update_if_dirty();
    am_bind_buffer(AM_ARRAY_BUFFER, buf->arraybuf->get_latest_id());
    am_set_attribute_pointer(location, view->components, view->gl_client_type(), view->is_normalized(), view->stride, view->offset);
    if (view->size < rstate->max_draw_array_size) {
        rstate->max_draw_array_size = view->size;
    }
    return true;
}

static void bind_sampler2d(am_render_state *rstate,
    am_gluint location, int texture_unit, am_texture2d *texture)
{
    if (texture->image_buffer != NULL) {
        texture->image_buffer->buffer->update_if_dirty();
    }
    am_set_active_texture_unit(texture_unit);
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, texture->texture_id);
    am_set_uniform1i(location, texture_unit);
}

static void report_incompatible_param_type(am_render_state *rstate, am_program_param *param) {
    am_program_param_name_slot *slot = &rstate->param_name_map[param->name];
    const char *shader_type;
    const char *client_type;
    shader_type = am_program_param_type_name(param->type);
    client_type = am_program_param_client_type_name(slot);
    const char *client_type_prefix = " ";
    if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY ||
        slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED)
    {
        client_type_prefix = "n ";
    }
    if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED) {
        am_log1("WARNING: %s '%s' was not bound to anything",
            shader_type, slot->name);
    } else {
        am_log1("WARNING: ignoring incompatible binding of %s '%s' to a%s%s",
            shader_type, slot->name, client_type_prefix, client_type);
    }
}

bool am_program_param::bind(am_render_state *rstate) {
    am_program_param_name_slot *slot = &rstate->param_name_map[name];
    bool bound = false;
    switch (type) {
        case AM_PROGRAM_PARAM_UNIFORM_1F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_1F) {
                am_set_uniform1f(location, slot->value.value.f);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_2F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_2F) {
                float fs[2];
                fs[0] = (float)slot->value.value.v2[0];
                fs[1] = (float)slot->value.value.v2[1];
                am_set_uniform2f(location, fs);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_3F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_3F) {
                float fs[3];
                fs[0] = (float)slot->value.value.v3[0];
                fs[1] = (float)slot->value.value.v3[1];
                fs[2] = (float)slot->value.value.v3[2];
                am_set_uniform3f(location, fs);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_4F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_4F) {
                float fs[4];
                fs[0] = (float)slot->value.value.v4[0];
                fs[1] = (float)slot->value.value.v4[1];
                fs[2] = (float)slot->value.value.v4[2];
                fs[3] = (float)slot->value.value.v4[3];
                am_set_uniform4f(location, fs);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_MAT2:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2) {
                float fs[4];
                for (int i = 0; i < 4; i++) {
                    fs[i] = (float)slot->value.value.m2[i];
                }
                am_set_uniform_mat2(location, fs);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_MAT3:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3) {
                float fs[9];
                for (int i = 0; i < 9; i++) {
                    fs[i] = (float)slot->value.value.m3[i];
                }
                am_set_uniform_mat3(location, fs);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_MAT4:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
                float fs[16];
                for (int i = 0; i < 16; i++) {
                    fs[i] = (float)slot->value.value.m4[i];
                }
                am_set_uniform_mat4(location, fs);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_SAMPLER2D:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
                assert(slot->value.value.sampler2d.texture_unit >= 0);
                bind_sampler2d(rstate, location, slot->value.value.sampler2d.texture_unit, slot->value.value.sampler2d.texture);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_ATTRIBUTE_1F:
        case AM_PROGRAM_PARAM_ATTRIBUTE_2F:
        case AM_PROGRAM_PARAM_ATTRIBUTE_3F:
        case AM_PROGRAM_PARAM_ATTRIBUTE_4F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY) {
                bound = bind_attribute_array(rstate, location, slot->value.value.arr);
            }
            break;
    }
    if (!bound) report_incompatible_param_type(rstate, this);
    return bound;
}

am_param_name_id am_lookup_param_name(lua_State *L, int name_idx) {
    am_render_state *g = am_global_render_state;
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
        if (name_ref >= g->param_name_map_capacity) {
            int old_capacity = g->param_name_map_capacity;
            while (name_ref >= g->param_name_map_capacity) {
                g->param_name_map_capacity *= 2;
            }
            g->param_name_map = (am_program_param_name_slot*)realloc(g->param_name_map, sizeof(am_program_param_name_slot) * g->param_name_map_capacity);
            for (int i = old_capacity; i < g->param_name_map_capacity; i++) {
                g->param_name_map[i].name = NULL;
                g->param_name_map[i].value.type = AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED;
            }
        }
        g->param_name_map[name_ref].name = lua_tostring(L, name_idx);
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
            lua_pushfstring(L, "%s shader compilation error:\n%s%sline %d:%s", type_str, msg, nl, line_no, line_str);
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

    if (!linked) {
        char *msg = am_get_program_info_log(program);
        lua_pushfstring(L, "shader program link error:\n%s", msg);
        free(msg);
        am_detach_shader(program, vertex_shader);
        am_detach_shader(program, fragment_shader);
        am_delete_shader(vertex_shader);
        am_delete_shader(fragment_shader);
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
        am_gluint loc;
        am_get_active_attribute(program, index, &name_str, &type, &size, &loc);
        // XXX check size
        lua_pushstring(L, name_str);
        int name = am_lookup_param_name(L, -1);
        lua_pop(L, 1); // name str
        am_program_param *param = &params[i];
        param->location = loc;
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
                lua_pushfstring(L, "sorry, attribute '%s' is of an unsupported type", name_str);
                free(name_str);
                am_delete_program(program);
                return lua_error(L);
        }
        free(name_str);
        i++;
    }

    // Generate uniform params
    for (int index = 0; index < num_uniforms; index++) {
        char *name_str;
        am_uniform_var_type type;
        int arr_size;
        am_gluint loc;
        am_get_active_uniform(program, index, &name_str, &type, &arr_size, &loc);
        if (arr_size > 1) {
            lua_pushfstring(L, "sorry, uniform '%s' is of an unsupported type", name_str);
            free(name_str);
            am_delete_program(program);
            return lua_error(L);
        }
        lua_pushstring(L, name_str);
        int name = am_lookup_param_name(L, -1);
        lua_pop(L, 1); // name
        am_program_param *param = &params[i];
        param->location = loc;
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
                lua_pushfstring(L, "sorry, uniform '%s' is of an unsupported type", name_str);
                free(name_str);
                am_delete_program(program);
                return lua_error(L);
        }
        free(name_str);
        i++;
    }

    // Deleting the shaders must be done after reading the uniforms and
    // attributes, because doing it before breaks in Safari on OSX
    // (emscripten backend).
    am_detach_shader(program, vertex_shader);
    am_detach_shader(program, fragment_shader);
    am_delete_shader(vertex_shader);
    am_delete_shader(fragment_shader);

    am_program *prog = am_new_userdata(L, am_program);
    prog->program_id = program;
    prog->num_params = num_params;
    prog->sets_point_size = (strstr(vertex_shader_src, "gl_PointSize") != NULL);
    prog->params = params;
    prog->num_vaas = num_attributes;

    return 1;
}

static int gc_program(lua_State *L) {
    am_program *prog = (am_program*)lua_touserdata(L, 1);
    am_use_program(0);
    am_delete_program(prog->program_id);
    free(prog->params);
    return 0;
}

static void register_program_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);
    lua_pushcclosure(L, gc_program, 0);
    lua_setfield(L, -2, "__gc");
    am_register_metatable(L, "program", MT_am_program, 0);
}

static int create_program_node(lua_State *L) {
    am_check_nargs(L, 1);
    am_program *prog = am_get_userdata(L, am_program, 1);
    am_program_node *node = am_new_userdata(L, am_program_node);
    node->tags.push_back(L, AM_TAG_USE_PROGRAM);
    node->program = prog;
    node->program_ref = node->ref(L, 1);
    return 1;
}

static void get_program(lua_State *L, void *obj) {
    am_program_node *node = (am_program_node*)obj;
    node->program->push(L);
}

static void set_program(lua_State *L, void *obj) {
    am_program_node *node = (am_program_node*)obj;
    node->program = am_get_userdata(L, am_program, 3);
    node->reref(L, node->program_ref, 3);
}

static am_property program_property = {get_program, set_program};

static void register_program_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "program", &program_property);

    am_register_metatable(L, "program_node", MT_am_program_node, MT_am_scene_node);
}

void am_program_node::render(am_render_state *rstate) {
    am_program* old_program = rstate->active_program;
    rstate->active_program = program;
    render_children(rstate);
    rstate->active_program = old_program;
}

void am_bind_node::render(am_render_state *rstate) {
    am_program_param_value *old_vals = (am_program_param_value*)alloca(sizeof(am_program_param_value) * num_params);
    int texture_units_to_release = 0;
    for (int i = 0; i < num_params; i++) {
        am_program_param_value *param = &rstate->param_name_map[names[i]].value;
        old_vals[i] = *param;
        *param = values[i];
        // assign texture unit if binding a sampler2D
        // XXX should this be moved into renderer.cpp?
        if (values[i].type == AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
            if (old_vals[i].type != AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
                param->value.sampler2d.texture_unit = rstate->next_free_texture_unit++;
                texture_units_to_release++;
            } else {
                // reuse texture unit
                param->value.sampler2d.texture_unit = old_vals[i].value.sampler2d.texture_unit;
            }
            assert(param->value.sampler2d.texture_unit >= 0);
        }
    }
    render_children(rstate);
    for (int i = 0; i < num_params; i++) {
        am_program_param_value *param = &rstate->param_name_map[names[i]].value;
        *param = old_vals[i];
    }
    rstate->next_free_texture_unit -= texture_units_to_release;
    assert(rstate->next_free_texture_unit >= 0);
}

static void push_program_param_value(lua_State *L, am_program_param_value *param) {
    switch (param->type) {
        case AM_PROGRAM_PARAM_CLIENT_TYPE_1F:
            lua_pushnumber(L, param->value.f);
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_2F:
            memcpy(&am_new_userdata(L, am_vec2)->v, &param->value.v2, 2 * sizeof(double));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_3F:
            memcpy(&am_new_userdata(L, am_vec3)->v, &param->value.v3, 3 * sizeof(double));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_4F:
            memcpy(&am_new_userdata(L, am_vec4)->v, &param->value.v4, 4 * sizeof(double));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2:
            memcpy(&am_new_userdata(L, am_mat2)->m, &param->value.m2, 4 * sizeof(double));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3:
            memcpy(&am_new_userdata(L, am_mat3)->m, &param->value.m3, 9 * sizeof(double));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4:
            memcpy(&am_new_userdata(L, am_mat4)->m, &param->value.m4, 16 * sizeof(double));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY:
            param->value.arr->push(L);
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D:
            param->value.sampler2d.texture->push(L);
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED:
            lua_pushnil(L);
            break;
    }
}

static void set_param_value(lua_State *L, am_program_param_value *param, int val_idx, int name_idx, int *ref, am_scene_node *node) {
    val_idx = am_absindex(L, val_idx);
    name_idx = am_absindex(L, name_idx);
    *ref = LUA_NOREF;
    switch (am_get_type(L, val_idx)) {
        case LUA_TNUMBER:
            param->set_float(lua_tonumber(L, val_idx));
            break;
        case MT_am_vec2:
            param->set_vec2(am_get_userdata(L, am_vec2, val_idx)->v);
            break;
        case MT_am_vec3:
            param->set_vec3(am_get_userdata(L, am_vec3, val_idx)->v);
            break;
        case MT_am_vec4:
            param->set_vec4(am_get_userdata(L, am_vec4, val_idx)->v);
            break;
        case MT_am_mat2:
            param->set_mat2(am_get_userdata(L, am_mat2, val_idx)->m);
            break;
        case MT_am_mat3:
            param->set_mat3(am_get_userdata(L, am_mat3, val_idx)->m);
            break;
        case MT_am_mat4:
            param->set_mat4(am_get_userdata(L, am_mat4, val_idx)->m);
            break;
        case MT_am_buffer_view: {
            am_buffer_view *view = am_get_userdata(L, am_buffer_view, val_idx);
            if (view->buffer->arraybuf == NULL) {
                view->buffer->create_arraybuf(L);
            }
            param->set_arr(view);
            *ref = node->ref(L, val_idx);
            break;
        }
        case MT_am_texture2d:
            param->set_samp2d(am_get_userdata(L, am_texture2d, val_idx));
            *ref = node->ref(L, val_idx);
            break;
        default:
            luaL_error(L, "invalid bind value for %s (%s)", lua_tostring(L, name_idx), am_get_typename(L, am_get_type(L, val_idx)));
    }
}

static void get_bind_node_value(lua_State *L, void *obj) {
    am_bind_node *node = (am_bind_node*)obj;
    node->pushuservalue(L);
    lua_pushlightuserdata(L, (void*)lua_tostring(L, 2));
    lua_rawget(L, -2);
    int index = lua_tointeger(L, -1);
    lua_pop(L, 2); // index, uservalue
    am_program_param_value *param = &node->values[index];
    push_program_param_value(L, param);
}

static void set_bind_node_value(lua_State *L, void *obj) {
    am_bind_node *node = (am_bind_node*)obj;
    node->pushuservalue(L);
    lua_pushlightuserdata(L, (void*)lua_tostring(L, 2));
    lua_rawget(L, -2);
    assert(lua_type(L, -1) == LUA_TNUMBER);
    int index = lua_tointeger(L, -1);
    lua_pop(L, 2); // index, uservalue
    am_program_param_value *param = &node->values[index];
    if (node->refs[index] != LUA_NOREF) {
        node->unref(L, node->refs[index]);
        node->refs[index] = LUA_NOREF;
    }
    set_param_value(L, param, 3, 2, &node->refs[index], node);
}

static am_property bind_node_value_property =
    {get_bind_node_value, set_bind_node_value};

static am_bind_node *new_bind_node(lua_State *L, int num_params) {
    // allocate extra space for the shader paramter names, values and refs
    size_t node_sz = sizeof(am_bind_node);
    am_align_size(node_sz);
    size_t names_sz = sizeof(am_param_name_id) * num_params;
    am_align_size(names_sz);
    size_t values_sz = sizeof(am_program_param_value) * num_params;
    am_align_size(values_sz);
    size_t refs_sz = sizeof(int) * num_params;
    am_align_size(refs_sz);
    am_bind_node *node = (am_bind_node*)am_set_metatable(L,
        new (lua_newuserdata(L, node_sz + names_sz + values_sz + refs_sz))
        am_bind_node(), MT_am_bind_node);
    node->num_params = num_params;
    node->names = (am_param_name_id*)(((uint8_t*)node) + node_sz);
    node->values = (am_program_param_value*)(((uint8_t*)node) + node_sz + names_sz);
    node->refs = (int*)(((uint8_t*)node) + node_sz + names_sz + values_sz);
    return node;
}

static void set_bind_property(lua_State *L, am_bind_node *node, int index, int name_idx) {
    name_idx = am_absindex(L, name_idx);
    node->pushuservalue(L);
    lua_pushvalue(L, name_idx); // param name
    lua_pushlightuserdata(L, (void*)&bind_node_value_property);
    lua_rawset(L, -3);
    lua_pushlightuserdata(L, (void*)lua_tostring(L, name_idx));
    lua_pushinteger(L, index);
    lua_rawset(L, -3);
    lua_pop(L, 1); // uservalue table
}

static int create_bind_node(lua_State *L) {
    am_check_nargs(L, 1);
    am_bind_node *node;
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "expecting a table in position 1");
    }
    int num_params = 0;
    lua_pushnil(L);
    while (lua_next(L, 1)) {
        if (!lua_isstring(L, -2)) {
            return luaL_error(L, "all bind param names must be strings");
        }
        num_params++;
        lua_pop(L, 1);
    }
    node = new_bind_node(L, num_params);
    node->tags.push_back(L, AM_TAG_BIND);
    lua_pushnil(L);
    int index = 0;
    while (lua_next(L, 1)) {
        if (!lua_isstring(L, -2)) {
            return luaL_error(L, "bind parameters must be strings");
        }
        node->names[index] = am_lookup_param_name(L, -2);
        set_param_value(L, &node->values[index], -1, -2, &node->refs[index], node);
        set_bind_property(L, node, index, -2);
        index++;
        lua_pop(L, 1);
    }
    return 1;
}

static void register_bind_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_metatable(L, "bind", MT_am_bind_node, MT_am_scene_node);
}

void am_read_uniform_node::render(am_render_state *rstate) {
    am_program_param_value *param = &rstate->param_name_map[name].value;
    switch (param->type) {
        case AM_PROGRAM_PARAM_CLIENT_TYPE_1F:
        case AM_PROGRAM_PARAM_CLIENT_TYPE_2F:
        case AM_PROGRAM_PARAM_CLIENT_TYPE_3F:
        case AM_PROGRAM_PARAM_CLIENT_TYPE_4F:
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2:
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3:
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4:
        case AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED:
            value = *param;
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY:
        case AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D:
            value.type = AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED;
            break;
    }
    render_children(rstate);
}

static int create_read_uniform_node(lua_State *L) {
    am_check_nargs(L, 1);
    if (!lua_isstring(L, 1)) return luaL_error(L, "expecting a string in position 2");
    am_read_uniform_node *node = am_new_userdata(L, am_read_uniform_node);
    node->tags.push_back(L, AM_TAG_READ_UNIFORM);
    node->name = am_lookup_param_name(L, 1);
    return 1;
}

static void get_read_uniform_node_value(lua_State *L, void *obj) {
    am_read_uniform_node *node = (am_read_uniform_node*)obj;
    push_program_param_value(L, &node->value);
}

static am_property read_uniform_node_value_property =
    {get_read_uniform_node_value, NULL};

static void register_read_uniform_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "value", &read_uniform_node_value_property);

    am_register_metatable(L, "read_uniform", MT_am_read_uniform_node, MT_am_scene_node);
}

const char *am_program_param_type_name(am_program_param_type t) {
    switch (t) {
        case AM_PROGRAM_PARAM_UNIFORM_1F: return "float uniform";
        case AM_PROGRAM_PARAM_UNIFORM_2F: return "vec2 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_3F: return "vec3 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_4F: return "vec4 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_MAT2: return "mat2 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_MAT3: return "mat3 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_MAT4: return "mat4 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_SAMPLER2D: return "sampler2D uniform";
        case AM_PROGRAM_PARAM_ATTRIBUTE_1F: return "float attribute array";
        case AM_PROGRAM_PARAM_ATTRIBUTE_2F: return "vec2 attribute array";
        case AM_PROGRAM_PARAM_ATTRIBUTE_3F: return "vec3 attribute array";
        case AM_PROGRAM_PARAM_ATTRIBUTE_4F: return "vec4 attribute array";
    }
    return NULL;
}

const char *am_program_param_client_type_name(am_program_param_name_slot *slot) {
    switch (slot->value.type) {
        case AM_PROGRAM_PARAM_CLIENT_TYPE_1F: return "float";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_2F: return "vec2";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_3F: return "vec3";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_4F: return "vec4";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2: return "mat2";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3: return "mat3";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4: return "mat4";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY: {
            if (slot->value.value.arr->buffer == NULL) {
                return "freed buffer";
            } else {
                switch (slot->value.value.arr->type) {
                    case AM_VIEW_TYPE_F32:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of float";
                            case 2: return "array of vec2";
                            case 3: return "array of vec3";
                            case 4: return "array of vec4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_F64:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of double";
                            case 2: return "array of dvec2";
                            case 3: return "array of dvec3";
                            case 4: return "array of dvec4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_U8:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of ubyte";
                            case 2: return "array of ubyte2";
                            case 3: return "array of ubyte3";
                            case 4: return "array of ubyte4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_I8:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of byte";
                            case 2: return "array of byte2";
                            case 3: return "array of byte3";
                            case 4: return "array of byte4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_U8N:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of ubyte_norm";
                            case 2: return "array of ubyte_norm2";
                            case 3: return "array of ubyte_norm3";
                            case 4: return "array of ubyte_norm4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_I8N:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of byte_norm";
                            case 2: return "array of byte_norm2";
                            case 3: return "array of byte_norm3";
                            case 4: return "array of byte_norm4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_U16:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of ushort";
                            case 2: return "array of ushort2";
                            case 3: return "array of ushort3";
                            case 4: return "array of ushort4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_I16:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of short";
                            case 2: return "array of short2";
                            case 3: return "array of short3";
                            case 4: return "array of short4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_U16E:
                        return "array of ushort_elem";
                    case AM_VIEW_TYPE_U16N:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of ushort_norm";
                            case 2: return "array of ushort_norm2";
                            case 3: return "array of ushort_norm3";
                            case 4: return "array of ushort_norm4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_I16N:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of short_norm";
                            case 2: return "array of short_norm2";
                            case 3: return "array of short_norm3";
                            case 4: return "array of short_norm4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_U32:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of uint";
                            case 2: return "array of uint2";
                            case 3: return "array of uint3";
                            case 4: return "array of uint4";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_I32:
                        switch (slot->value.value.arr->components) {
                            case 1: return "array of int";
                            case 2: return "array of int";
                            case 3: return "array of int";
                            case 4: return "array of int";
                        }
                        return "array of unknown type";
                    case AM_VIEW_TYPE_U32E:
                        return "array of uint_elem";
                    case AM_NUM_VIEW_TYPES: 
                        assert(false); 
                        break;
                }
            }
            break;
        }
        case AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D: return "texture2d";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED: return "uninitialized parameter";
    }
    return NULL;
}

void am_open_program_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"program", create_program},
        {"bind", create_bind_node},
        {"use_program", create_program_node},
        {"read_uniform", create_read_uniform_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_program_mt(L);
    register_program_node_mt(L);
    register_bind_node_mt(L);
    register_read_uniform_node_mt(L);
}
