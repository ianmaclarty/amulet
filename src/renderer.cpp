#include "amulet.h"

void am_render_state::draw() {
    bind_active_program();
    bind_active_program_params();
    if (validate_active_program()) {
        if (active_indices_id) {
            bind_active_indices();
            if (active_indices_max_value <= active_attribute_array_max_size) {
                int count = active_indices_max_size - active_indices_offset;
                if (count > active_indices_count) count = active_indices_count;
                if (count > 0) {
                    am_draw_elements(active_draw_mode, active_indices_count, active_indices_type, active_indices_offset);
                }
            } else {
                am_report_error("buffer index out of range");
            }
        } else {
            int count = active_attribute_array_max_size - active_attribute_array_first;
            if (count > active_attribute_array_count) count = active_attribute_array_count;
            if (count > 0) {
                am_draw_arrays(active_draw_mode, active_attribute_array_first, count);
            }
        }
    }
}

bool am_render_state::validate_active_program() {
    if (am_conf_validate_shaders) {
        bool valid = am_validate_program(active_program_id);
        if (!valid || am_conf_report_all_shader_validation_messages) {
            char *log = am_get_program_info_log(active_program_id);
            if (valid) {
                am_report_message("shader program validation messages: %s", log);
            } else {
                am_report_error("shader program failed validation: %s", log);
            }
            free(log);
        }
        return valid;
    } else {
        return true;
    }
}

void am_render_state::bind_active_program() {
    if (bound_program_id != active_program_id) {
        am_use_program(active_program_id);
        bound_program_id = active_program_id;
    }
}

void am_render_state::bind_active_program_params() {
    active_attribute_array_max_size = INT_MAX;
    am_program_param **ptr = active_program_params;
    while (*ptr != NULL) {
        am_program_param *param = *ptr;
        param->bind(this);
        ptr++;
    }
}

void am_render_state::bind_active_indices() {
    am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, active_indices_id);
}

struct am_uniform_param : am_program_param {
    am_uniform_index index;
};

struct am_float_uniform_param : am_uniform_param {
    float value;

    virtual void bind(am_render_state *rstate) {
        am_set_uniform1f(index, 1, &value);
    }
};

struct am_attribute_param : am_program_param {
    am_attribute_index index;
};

struct am_float_attribute_param : am_attribute_param {
    float value;

    virtual void bind(am_render_state *rstate) {
        am_set_attribute_array_enabled(index, false);
        am_set_attribute1f(index, value);
    }
};

struct am_array_attribute_param : am_attribute_param {
    am_attribute_array *array;
    int pos; // attribute position in the attribute array

    virtual void bind(am_render_state *rstate) {
        array->bind_attribute(rstate, index, pos);
        if (array->size < rstate->active_attribute_array_max_size) {
            rstate->active_attribute_array_max_size = array->size;
        }
    }
};
