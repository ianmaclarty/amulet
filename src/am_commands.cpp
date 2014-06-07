#include "amulet.h"

void am_set_float_param_command::execute(am_render_state *rstate) {
    rstate->params_by_name[name]->trailed_set_float(rstate, value);
}

void am_mul_float_param_command::execute(am_render_state *rstate) {
    rstate->params_by_name[name]->trailed_mul_float(rstate, value);
}

void am_add_float_param_command::execute(am_render_state *rstate) {
    rstate->params_by_name[name]->trailed_add_float(rstate, value);
}

void am_set_float_array_command::execute(am_render_state *rstate) {
    int available_bytes = vbo->size - offset - am_attribute_client_type_size(type);
    int max_draw_elements = 0;
    if (available_bytes > 0) {
        max_draw_elements = available_bytes / stride + 1;
    }
    rstate->params_by_name[name]->trailed_set_float_array(rstate,
        vbo->buffer_id, type, normalized, stride, offset, max_draw_elements);
}

void am_draw_command::execute(am_render_state *rstate) {
    rstate->draw();
}
