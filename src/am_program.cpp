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
