#include "amulet.h"

void am_viewport_state::set(int x, int y, int w, int h) {
    dirty = am_viewport_state::x != x ||
        am_viewport_state::y != y ||
        am_viewport_state::w != w ||
        am_viewport_state::h != h;
    am_viewport_state::x = x;
    am_viewport_state::y = y;
    am_viewport_state::w = w;
    am_viewport_state::h = h;
}

void am_viewport_state::update() {
    if (dirty) {
        am_set_viewport(x, y, w, h);
        dirty = false;
    }
}

void am_render_state::update_state() {
    viewport_state.update();
    bind_active_program();
    bind_active_program_params();
}

void am_render_state::draw_arrays(int first, int draw_array_count) {
    if (active_program == NULL) return;
    update_state();
    if (validate_active_program()) {
        if (max_draw_array_size == INT_MAX && draw_array_count == INT_MAX) {
            // no vertex arrays have been set and no size has been specified
            // for the draw command, so do nothing.
            draw_array_count = 0;
        }
        int count = max_draw_array_size - first;
        if (count > draw_array_count) count = draw_array_count;
        if (count > 0) {
            am_draw_arrays(draw_mode, first, count);
        }
    }
}

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
        am_program_param *param = &active_program->params[i];
        param->bind(this);
    }
}

void am_render_state::bind_active_indices() {
    am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, active_indices_id);
}

am_render_state::am_render_state() {
    viewport_state.x = 0;
    viewport_state.y = 0;
    viewport_state.w = 0;
    viewport_state.h = 0;
    viewport_state.dirty = true;

    active_indices_id = 0;
    active_indices_max_value = 0;
    active_indices_max_size = 0;
    active_indices_type = AM_ELEMENT_TYPE_USHORT;

    max_draw_array_size = 0;

    draw_mode = AM_DRAWMODE_TRIANGLES;

    bound_program_id = 0;
    active_program = NULL;

    next_free_texture_unit = 0;
}

am_render_state::~am_render_state() {
}

static void register_draw_arrays_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "draw_arrays");
    lua_setfield(L, -2, "tname");

    am_register_metatable(L, MT_am_draw_arrays_node, MT_am_scene_node);
}

void am_draw_arrays_node::render(am_render_state *rstate) {
    rstate->draw_arrays(first, count);
}

static int create_draw_arrays_node(lua_State *L) {
    int nargs = am_check_nargs(L, 0);
    int first = 0;
    int count = INT_MAX;
    if (nargs > 0) {
        first = luaL_checkinteger(L, 1);
    }
    if (nargs > 1) {
        count = luaL_checkinteger(L, 2);
    }
    am_draw_arrays_node *node = am_new_userdata(L, am_draw_arrays_node);
    node->first = first;
    node->count = count;
    return 1;
}

void am_open_renderer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"draw_arrays", create_draw_arrays_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_draw_arrays_node_mt(L);
}
