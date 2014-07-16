#include "amulet.h"

int am_command::alias_index(lua_State *L) {
    return luaL_error(L, "alias_index NYI");
}

int am_command::alias_newindex(lua_State *L) {
    return luaL_error(L, "alias_newindex NYI");
}

void am_draw_children_command::execute(am_node *node, am_render_state *rstate) {
    for (int i = 0; i < node->children.size; i++) {
        node->children.arr[i].child->render(rstate);
    }
}

void am_use_program_command::execute(am_node *node, am_render_state *rstate) {
    for (int i = 0; i < program->num_params; i++) {
        am_program_param *param = program->params[i];
        am_program_param **slot = &am_param_name_map[param->name].param;
        rstate->trail.trail(slot, sizeof(am_program_param*));
        *slot = param;
    }
    rstate->trail.trail(&rstate->active_program, sizeof(am_program*));
    rstate->active_program = program;
}

am_draw_arrays_command::am_draw_arrays_command(int f, int c) {
    first = f;
    count = c;
}

void am_draw_arrays_command::execute(am_node *node, am_render_state *rstate) {
    rstate->draw_arrays(first, count);
}

am_set_float_param_command::am_set_float_param_command(lua_State *L, int nargs, am_node *node) {
    if (nargs < 2 || !lua_isstring(L, 2)) {
        luaL_error(L, "expecting a string in position 2");
    }
    name = am_lookup_param_name(L, 2);
    if (nargs > 2) {
        value = lua_tonumber(L, 3);
    } else {
        value = 0.0f;
    }
}

int am_set_float_param_command::alias_index(lua_State *L) {
    lua_pushnumber(L, value);
    return 1;
}

int am_set_float_param_command::alias_newindex(lua_State *L) {
    value = luaL_checknumber(L, 3);
    return 0;
}

void am_set_float_param_command::execute(am_node *node, am_render_state *rstate) {
    am_program_param *p = am_param_name_map[name].param;
    if (p != NULL) p->trailed_set_float(rstate, value);
}

am_set_array_command::am_set_array_command(lua_State *L, int nargs, am_node *node) {
    if (nargs < 2 || !lua_isstring(L, 2)) {
        luaL_error(L, "expecting a string in position 2");
    }
    name = am_lookup_param_name(L, 2);
    if (nargs < 3) {
        luaL_error(L, "expecting a buffer view in position 3");
    }
    am_buffer_view *view = (am_buffer_view*)am_check_metatable_id(L, AM_MT_BUFFER_VIEW, 3);

    am_push_ref(L, 3, view->buffer_ref); // push buffer
    if (view->buffer->vbo == NULL) {
        am_push_new_vertex_buffer(L, view->buffer, -1);  // push new vbo
    } else {
        am_push_ref(L, -1, view->buffer->vbo->vbo_ref);  // push existing vbo
    }
    int vbo_idx = lua_absindex(L, -1);

    vbo = view->buffer->vbo;
    vbo_ref = am_new_ref(L, 1, vbo_idx); // create ref from node to vbo
    am_buf_view_type_to_attr_client_type_and_dimensions(view->type, &type, &dimensions);
    normalized = view->normalized;
    stride = view->stride;
    offset = view->offset;

    lua_pop(L, 2); // buffer, vbo
}

void am_set_array_command::execute(am_node *node, am_render_state *rstate) {
    am_program_param *p = am_param_name_map[name].param;
    if (p == NULL) return;
    int available_bytes = vbo->size - offset - am_attribute_client_type_size(type);
    int max_draw_elements = 0;
    if (available_bytes > 0) {
        max_draw_elements = available_bytes / stride + 1;
    }
    p->trailed_set_array(rstate, vbo, type, dimensions, normalized, stride, offset,
        max_draw_elements);
}
