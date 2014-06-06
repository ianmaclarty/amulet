#include "amulet.h"

void am_render_state::draw() {
    bind_active_program();
    bind_active_program_params();
    if (validate_active_program()) {
        if (active_indices_id) {
            bind_active_indices();
            if (active_indices_max_value <= max_draw_array_size) {
                int count = active_indices_max_size - active_indices_offset;
                if (count > active_indices_count) count = active_indices_count;
                if (count > 0) {
                    am_draw_elements(draw_mode, active_indices_count, active_indices_type, active_indices_offset);
                }
            } else {
                am_report_error("buffer index out of range");
            }
        } else {
            int count = max_draw_array_size - draw_array_offset;
            if (count > draw_array_count) count = draw_array_count;
            if (count > 0) {
                am_draw_arrays(draw_mode, draw_array_offset, count);
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
    max_draw_array_size = INT_MAX;
    am_program_param **ptr = active_program_params;
    while (*ptr != NULL) {
        am_program_param *param = *ptr;
        param->bind(this);
        ptr++;
    }
    if (max_draw_array_size == INT_MAX) {
        max_draw_array_size = 0; // no array attributes were bound
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

struct am_array_attribute_config {
    am_buffer_id buffer_id;
    am_attribute_client_type type;
    int size;
    bool normalized;
    int stride;
    int offset;
    int max_draw_elements;
};

struct am_array_attribute_param : am_attribute_param {
    am_array_attribute_config config;

    virtual void bind(am_render_state *rstate) {
        am_set_attribute_array_enabled(index, true);
        am_bind_buffer(AM_ARRAY_BUFFER, config.buffer_id);
        am_set_attribute_pointer(index, config.size, config.type, config.normalized, config.stride, config.offset);
        if (config.max_draw_elements < rstate->max_draw_array_size) {
            rstate->max_draw_array_size = config.max_draw_elements;
        }
    }
};
