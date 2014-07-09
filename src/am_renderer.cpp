#include "amulet.h"

void am_render_state::draw_arrays(int first, int draw_array_count) {
    if (active_program == NULL) return;
    bind_active_program();
    bind_active_program_params();
    if (validate_active_program()) {
        if (max_draw_array_size == INT_MAX && draw_array_count == INT_MAX) {
            // no vertex arrays have been set and no size has been specified
            // for the draw command. Pick something smaller.
            draw_array_count = 6;
        }
        int count = max_draw_array_size - first;
        if (count > draw_array_count) count = draw_array_count;
        if (count > 0) {
            am_draw_arrays(draw_mode, first, count);
        }
    }
}

/*
void am_render_state::draw_elements(int first, int active_indices_count) {
        if (active_indices_id) {
            bind_active_indices();
            if (active_indices_max_value <= max_draw_array_size) {
                int count = active_indices_max_size - active_indices_offset;
                if (count > active_indices_count) count = active_indices_count;
                if (count > 0) {
                    am_draw_elements(draw_mode, count, active_indices_type, active_indices_offset);
                }
            } else {
                am_report_error("buffer index out of range");
            }
*/

bool am_render_state::validate_active_program() {
    if (am_conf_validate_shader_programs) {
        bool valid = am_validate_program(active_program->program_id);
        if (!valid) {
            char *log = am_get_program_info_log(active_program->program_id);
            am_report_error("shader program failed validation: %s", log);
            free(log);
        }
        return valid;
    } else {
        return true;
    }
}

void am_render_state::bind_active_program() {
    if (bound_program_id != active_program->program_id) {
        am_use_program(active_program->program_id);
        bound_program_id = active_program->program_id;
    }
}

void am_render_state::bind_active_program_params() {
    max_draw_array_size = INT_MAX;
    for (int i = 0; i < active_program->num_params; i++) {
        am_program_param *param = active_program->params[i];
        param->bind(this);
    }
}

void am_render_state::bind_active_indices() {
    am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, active_indices_id);
}

am_render_state::am_render_state() {
    framebuffer_state_dirty = true;
    blend_state_dirty = true;
    depth_test_state_dirty = true;
    stencil_test_state_dirty = true;
    scissor_test_state_dirty = true;
    sample_coverage_state_dirty = true;
    viewport_state_dirty = true;
    facecull_state_dirty = true;
    polygon_offset_state_dirty = true;
    line_state_dirty = true;
    dither_state_dirty = true;

    active_indices_id = 0;
    active_indices_max_value = 0;
    active_indices_max_size = 0;
    active_indices_type = AM_ELEMENT_TYPE_UNSIGNED_SHORT;

    max_draw_array_size = 0;

    draw_mode = AM_DRAWMODE_TRIANGLES;

    bound_program_id = 0;
    active_program = NULL;
}

am_render_state::~am_render_state() {
}
