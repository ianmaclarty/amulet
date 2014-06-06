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

void am_draw_command::execute(am_render_state *rstate) {
    rstate->draw();
}
