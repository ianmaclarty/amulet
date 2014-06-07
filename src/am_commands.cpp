#include "amulet.h"

am_command *am_command_list::first() {
    if (end > 0) {
        return (am_command*)base;
    } else {
        return NULL;
    }
}

void *am_command_list::extend(int size) {
    if (end + size <= capacity) {
        am_command *cmd = (am_command*)(base + end);
        am_pointer_align_size(size);
        if (end > 0) {
            am_command *prev_cmd = (am_command*)(base + last);
            cmd->prev_offset = end - last;
            prev_cmd->next_offset = end - last;
        } else {
            cmd->prev_offset = -1;
        }
        cmd->next_offset = -1;
        last = end;
        end += size;
        return cmd;
    } else {
        // list needs resizing
        return NULL;
    }
}

am_command *am_command::prev() {
    if (prev_offset < 0) return NULL;
    else return (am_command*)((char*)this - prev_offset);
}

am_command *am_command::next() {
    if (next_offset < 0) return NULL;
    else return (am_command*)((char*)this + next_offset);
}

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
