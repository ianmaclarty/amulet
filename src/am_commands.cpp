#include "amulet.h"

am_set_float_param_command::am_set_float_param_command(lua_State *L, am_node *node) {
    if (!lua_isstring(L, 2)) {
        luaL_error(L, "expecting a string in position 2");
    }
    name = am_lookup_param_name(L, 2);
    value = lua_tonumber(L, 3);
}

void am_set_float_param_command::execute(am_render_state *rstate) {
    am_program_param *p = am_param_name_map[name];
    if (p != NULL) p->trailed_set_float(rstate, value);
}

void am_mul_float_param_command::execute(am_render_state *rstate) {
    am_program_param *p = am_param_name_map[name];
    if (p != NULL) p->trailed_mul_float(rstate, value);
}

void am_add_float_param_command::execute(am_render_state *rstate) {
    am_program_param *p = am_param_name_map[name];
    if (p != NULL) p->trailed_add_float(rstate, value);
}

void am_set_float_array_command::execute(am_render_state *rstate) {
    am_program_param *p = am_param_name_map[name];
    if (p == NULL) return;
    int available_bytes = vbo->size - offset - am_attribute_client_type_size(type);
    int max_draw_elements = 0;
    if (available_bytes > 0) {
        max_draw_elements = available_bytes / stride + 1;
    }
    p->trailed_set_float_array(rstate,
        vbo->buffer_id, type, normalized, stride, offset, max_draw_elements);
}

void am_draw_command::execute(am_render_state *rstate) {
    rstate->draw();
}
