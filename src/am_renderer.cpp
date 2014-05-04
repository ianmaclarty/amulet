void am_render_state::apply_non_program_state() {

}

void am_render_state::render_geometry() {
    apply_non_program_state();
    am_program *program = program_stack.top(version);
    if (live_program_id != program->program_id) {
        am_use_program(program->program_id);
        live_program_id = program->program_id;
    }
    for (int i = 0; i < program->float_uniforms.size(); i++) {
        am_uniform_info<float> *info = &program->float_uniforms[i];
        if (info->stack->dirty) {
            am_set_uniform1f(info->index, 1, &info->stack->top(version));
        }
    }
    for (int i = 0; i < program->vec2_uniforms.size(); i++) {
        am_uniform_info<glm::vec2> *info = program->vec2_uniforms[i];
        if (info->stack->dirty) {
            am_set_uniform2f(info->index, 1, glm::value_ptr(*info->stack->top(version)));
        }
    }
    bind_program_uniforms();
    bind_program_attributes();

    if (am_devmode && !am_validate_program(program->program_id)) {
        const char *log = am_get_program_info_log(program->program_id);
        am_report_error("shader program failed validation: %s", log);
        free(log);
    } else {
}
