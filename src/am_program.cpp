#include "amulet.h"

struct am_uniform_param : am_program_param {
    am_uniform_index index;
};

struct am_float_uniform_param : am_uniform_param {
    float value;

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

struct am_attribute_param : am_program_param {
    am_attribute_index index;
};

struct am_float_attribute_state {
    float value; // only used if buffer_id == 0
    am_buffer_id buffer_id;
    am_attribute_client_type type;
    bool normalized;
    int stride;
    int offset;
    int max_draw_elements;
};

struct am_float_attribute_param : am_attribute_param {
    am_float_attribute_state state;

    virtual void bind(am_render_state *rstate) {
        if (state.buffer_id) {
            am_set_attribute_array_enabled(index, true);
            am_bind_buffer(AM_ARRAY_BUFFER, state.buffer_id);
            am_set_attribute_pointer(index, 1, state.type, state.normalized, state.stride, state.offset);
            if (state.max_draw_elements < rstate->max_draw_array_size) {
                rstate->max_draw_array_size = state.max_draw_elements;
            }
        } else {
            am_set_attribute_array_enabled(index, false);
            am_set_attribute1f(index, state.value);
        }
    }

    virtual void trailed_set_float(am_render_state *rstate, float val) {
        rstate->trail.trail(&state, sizeof(am_float_attribute_state));
        state.buffer_id = 0;
        state.value = val;
    }

    virtual void trailed_set_float_array(am_render_state *rstate,
        am_buffer_id buffer_id, am_attribute_client_type type, bool normalized, int stride, int offset, int max_draw_elements)
    {
        rstate->trail.trail(&state, sizeof(am_float_attribute_state));
        state.buffer_id = buffer_id;
        state.type = type;
        state.normalized = normalized;
        state.stride = stride;
        state.offset = offset;
        state.max_draw_elements = max_draw_elements;
    }

    virtual void trailed_mul_float(am_render_state *rstate, float val) {
        rstate->trail.trail(&state, sizeof(am_float_attribute_state));
        state.buffer_id = 0;
        state.value *= val;
    }

    virtual void trailed_add_float(am_render_state *rstate, float val) {
        rstate->trail.trail(&state, sizeof(am_float_attribute_state));
        state.buffer_id = 0;
        state.value += val;
    }
};

static int am_param_name_map_capacity = 0;

void am_init_param_name_map() {
    if (am_param_name_map != NULL) free(am_param_name_map);
    am_param_name_map_capacity = 32;
    am_param_name_map = (am_program_param**)malloc(sizeof(am_program_param*) * am_param_name_map_capacity);
    for (int i = 0; i < am_param_name_map_capacity; i++) {
        am_param_name_map[i] = NULL;
    }
}

int am_lookup_param_name(lua_State *L, int name_idx) {
    name_idx = lua_absindex(L, name_idx);
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_PARAM_NAME_STRING_TABLE);
    int strt_idx = lua_gettop(L);
    lua_pushvalue(L, name_idx);
    lua_rawget(L, strt_idx);
    if (lua_isnil(L, -1)) {
        // param name not seen before, register it.
        lua_pop(L, 1); // nil
        lua_pushvalue(L, name_idx);
        int name_ref = lua_ref(L, strt_idx);
        lua_pushvalue(L, name_idx);
        lua_pushinteger(L, name_ref);
        lua_rawset(L, strt_idx);
        lua_pop(L, 1); // string table
        if (name_ref >= am_param_name_map_capacity) {
            int old_capacity = am_param_name_map_capacity;
            while (name_ref >= am_param_name_map_capacity) {
                am_param_name_map_capacity *= 2;
            }
            am_param_name_map = (am_program_param**)realloc(am_param_name_map, sizeof(am_program_param*) * am_param_name_map_capacity);
            for (int i = old_capacity; i < am_param_name_map_capacity; i++) {
                am_param_name_map[i] = NULL;
            }
        }
        return name_ref;
    } else {
        int name_ref = lua_tointeger(L, -1);
        lua_pop(L, 2); // name ref, string table
        return name_ref;
    }
}
