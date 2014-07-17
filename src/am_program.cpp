#include "amulet.h"

am_program_param::am_program_param(am_param_name_id name) {
    am_program_param::name = name;
}

void am_program_param::trailed_set_float(am_render_state *rstate, float val) {
    am_report_error("attempt to set param '%s' to float %f",
        am_param_name_map[name].name, val);
}

void am_program_param::trailed_set_array(am_render_state *rstate,
    am_vertex_buffer *vbo, am_attribute_client_type type, int dimensions,
    bool normalized, int stride, int offset, int max_draw_elements)
{
    am_report_error("attempt to set param '%s' to an array",
        am_param_name_map[name].name);
}

struct am_uniform_param : am_program_param {
    am_uniform_index index;

    am_uniform_param(am_param_name_id name, am_uniform_index index)
        : am_program_param(name)
    {
        am_uniform_param::index = index;
    }
};

struct am_float_uniform_param : am_uniform_param {
    float value;

    am_float_uniform_param(am_param_name_id name, am_uniform_index index)
        : am_uniform_param(name, index)
    {
        value = 0.0f;
    }

    virtual void bind(am_render_state *rstate) {
        am_set_uniform1f(index, 1, &value);
    }

    virtual void trailed_set_float(am_render_state *rstate, float val) {
        rstate->trail.trail(&value, sizeof(float));
        value = val;
    }

    virtual void trailed_mul_float(am_render_state *rstate, float val) {
        rstate->trail.trail(&value, sizeof(float));
        value *= val;
    }

    virtual void trailed_add_float(am_render_state *rstate, float val) {
        rstate->trail.trail(&value, sizeof(float));
        value += val;
    }
};

struct am_attribute_state {
    bool is_array;
    union {
        float scalar[4];
        struct {
            am_vertex_buffer *vbo;
            am_attribute_client_type type;
            bool normalized;
            int stride;
            int offset;
            int max_draw_elements;
        } array;
    } u;
};

struct am_attribute_param : am_program_param {
    int dimensions;
    am_attribute_index index;
    am_attribute_state state;

    am_attribute_param(am_param_name_id name, am_attribute_index index, int dimensions)
        : am_program_param(name)
    {
        assert(dimensions >= 1 && dimensions <= 4);
        am_attribute_param::index = index;
        state.is_array = false;
        am_attribute_param::dimensions = dimensions;
        state.u.scalar[0] = 0.0f;
        state.u.scalar[1] = 0.0f;
        state.u.scalar[2] = 0.0f;
        state.u.scalar[3] = 0.0f;
    }

    virtual void bind(am_render_state *rstate) {
        if (state.is_array) {
            am_set_attribute_array_enabled(index, true);
            am_bind_buffer(AM_ARRAY_BUFFER, state.u.array.vbo->buffer_id);
            if (state.u.array.vbo->dirty_start < state.u.array.vbo->dirty_end) {
                am_vertex_buffer *vbo = state.u.array.vbo;
                am_set_buffer_sub_data(AM_ARRAY_BUFFER, vbo->dirty_start, vbo->dirty_end - vbo->dirty_start, vbo->buffer->data + vbo->dirty_start);
                vbo->dirty_end = 0;
                vbo->dirty_start = INT_MAX;
            }
            am_set_attribute_pointer(index, dimensions, state.u.array.type, state.u.array.normalized, state.u.array.stride, state.u.array.offset);
            if (state.u.array.max_draw_elements < rstate->max_draw_array_size) {
                rstate->max_draw_array_size = state.u.array.max_draw_elements;
            }
        } else {
            am_set_attribute_array_enabled(index, false);
            switch (dimensions) {
                case 1: am_set_attribute1f(index, state.u.scalar[0]); break;
                case 2: am_set_attribute2f(index, state.u.scalar); break;
                case 3: am_set_attribute3f(index, state.u.scalar); break;
                case 4: am_set_attribute4f(index, state.u.scalar); break;
            }
        }
    }

    virtual void trailed_set_float(am_render_state *rstate, float val) {
        rstate->trail.trail(&state, sizeof(am_attribute_state));
        state.is_array = false;
        state.u.scalar[0] = val;
        state.u.scalar[1] = 0.0f;
        state.u.scalar[2] = 0.0f;
        state.u.scalar[3] = 0.0f;
    }

    virtual void trailed_set_array(am_render_state *rstate,
        am_vertex_buffer *vbo, am_attribute_client_type type, int dimensions,
        bool normalized, int stride, int offset, int max_draw_elements)
    {
        if (dimensions == am_attribute_param::dimensions) {
            rstate->trail.trail(&state, sizeof(am_attribute_state));
            state.is_array = true;
            state.u.array.vbo = vbo;
            state.u.array.type = type;
            state.u.array.normalized = normalized;
            state.u.array.stride = stride;
            state.u.array.offset = offset;
            state.u.array.max_draw_elements = max_draw_elements;
        } else {
            am_report_error("invalid dimensions when setting shader attribute '%s' (expected %d, got %d)",
                am_param_name_map[name].name, am_attribute_param::dimensions, dimensions);
        }
    }
};

static int am_param_name_map_capacity = 0;

void am_init_param_name_map(lua_State *L) {
    if (am_param_name_map != NULL) free(am_param_name_map);
    am_param_name_map_capacity = 32;
    am_param_name_map = (am_program_param_name_info*)malloc(sizeof(am_program_param_name_info) * am_param_name_map_capacity);
    for (int i = 0; i < am_param_name_map_capacity; i++) {
        am_param_name_map[i].param = NULL;
        am_param_name_map[i].name = NULL;
    }
    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_PARAM_NAME_STRING_TABLE);
}

am_param_name_id am_lookup_param_name(lua_State *L, int name_idx) {
    name_idx = lua_absindex(L, name_idx);
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
            am_param_name_map = (am_program_param_name_info*)realloc(am_param_name_map, sizeof(am_program_param_name_info) * am_param_name_map_capacity);
            for (int i = old_capacity; i < am_param_name_map_capacity; i++) {
                am_param_name_map[i].param = NULL;
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

#define VERTEX_SHADER 0
#define FRAGMENT_SHADER 1

am_shader_id load_shader(lua_State *L, int type, const char *src) {
    am_shader_id shader = 0;
    switch (type) {
        case VERTEX_SHADER: shader = am_create_vertex_shader(); break;
        case FRAGMENT_SHADER: shader = am_create_fragment_shader(); break;
    }
    if (shader == 0) {
        lua_pushstring(L, "unable to create new shader");
        return 0;
    }

    am_set_shader_source(shader, src);
    bool compiled = am_compile_shader(shader);
    if (!compiled) {
        const char *type_str = "<unknown>";
        switch (type) {
            case VERTEX_SHADER: type_str = "vertex"; break;
            case FRAGMENT_SHADER: type_str = "fragment"; break;
        }
        char *msg = am_get_shader_info_log(shader);
        lua_pushfstring(L, "%s shader compilation error:\n%s", type_str, msg);
        free(msg);
        am_delete_shader(shader);
        return 0;
   }

   return shader;
}

static int create_program(lua_State *L) {
    if (!am_gl_context_exists()) {
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

    am_shader_id vertex_shader = load_shader(L, VERTEX_SHADER, vertex_shader_src);
    if (vertex_shader == 0) {
        return lua_error(L);
    }
    am_shader_id fragment_shader = load_shader(L, FRAGMENT_SHADER, fragment_shader_src);
    if (fragment_shader == 0) {
        am_delete_shader(vertex_shader);
        return lua_error(L);
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
        lua_error(L);
    }

    am_delete_shader(vertex_shader);
    am_delete_shader(fragment_shader);

    int num_attributes = am_get_program_active_attributes(program);
    int num_uniforms = am_get_program_active_uniforms(program);

    int num_params = num_attributes + num_uniforms;    

    am_program_param **params = (am_program_param**)malloc(sizeof(am_program_param*) * num_params);

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
        am_program_param *param = NULL;
        switch (type) {
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT:
                param = new am_attribute_param(name, index, 1);
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC2:
                param = new am_attribute_param(name, index, 2);
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC3:
                param = new am_attribute_param(name, index, 3);
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC4:
                param = new am_attribute_param(name, index, 4);
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT2:
                am_abort("NYI: AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT2");
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT3:
                am_abort("NYI: AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT3");
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT4:
                am_abort("NYI: AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT4");
                break;
            case AM_ATTRIBUTE_VAR_TYPE_UNKNOWN:
                am_report_message("warning: ignoring attribute '%s' with unknown type", name_str);
                num_params--;
                free(name_str);
                continue;
        }
        free(name_str);
        params[i] = param;
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
        am_program_param *param = NULL;
        switch (type) {
            case AM_UNIFORM_VAR_TYPE_FLOAT:
                param = new am_float_uniform_param(name, index);
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_VEC2:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_FLOAT_VEC2");
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_VEC3:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_FLOAT_VEC3");
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_VEC4:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_FLOAT_VEC4");
                break;
            case AM_UNIFORM_VAR_TYPE_INT:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_INT");
                break;
            case AM_UNIFORM_VAR_TYPE_INT_VEC2:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_INT_VEC2");
                break;
            case AM_UNIFORM_VAR_TYPE_INT_VEC3:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_INT_VEC3");
                break;
            case AM_UNIFORM_VAR_TYPE_INT_VEC4:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_INT_VEC4");
                break;
            case AM_UNIFORM_VAR_TYPE_BOOL:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_BOOL");
                break;
            case AM_UNIFORM_VAR_TYPE_BOOL_VEC2:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_BOOL_VEC2");
                break;
            case AM_UNIFORM_VAR_TYPE_BOOL_VEC3:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_BOOL_VEC3");
                break;
            case AM_UNIFORM_VAR_TYPE_BOOL_VEC4:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_BOOL_VEC4");
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_MAT2:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_FLOAT_MAT2");
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_MAT3:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_FLOAT_MAT3");
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_MAT4:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_FLOAT_MAT4");
                break;
            case AM_UNIFORM_VAR_TYPE_SAMPLER_2D:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_SAMPLER_2D");
                break;
            case AM_UNIFORM_VAR_TYPE_SAMPLER_CUBE:
                am_abort("NYI: AM_UNIFORM_VAR_TYPE_SAMPLER_CUBE");
                break;
            case AM_UNIFORM_VAR_TYPE_UNKNOWN:
                am_report_message("warning: ignoring uniform '%s' with unknown type", name_str);
                num_params--;
                free(name_str);
                continue;
        }
        free(name_str);
        params[i] = param;
        i++;
    }

    am_program *prog = new (lua_newuserdata(L, sizeof(am_program))) am_program();
    prog->program_id = program;
    prog->num_params = num_params;
    prog->params = params;
    am_set_metatable(L, AM_MT_PROGRAM, -1);

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
    am_register_metatable(L, AM_MT_PROGRAM, 0);
}

void am_open_program_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"program", create_program},
        {NULL, NULL}
    };
    am_open_module(L, "amulet", funcs);
    register_program_mt(L);
}
