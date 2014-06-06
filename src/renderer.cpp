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
}

void am_render_state::bind_active_indices() {
    am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, active_indices_id);
}

