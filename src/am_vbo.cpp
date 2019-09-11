#include "amulet.h"

static int vbo_gc(lua_State *L) {
    am_vbo *vbo = am_get_userdata(L, am_vbo, 1);
    vbo->delete_vbo_slots();
    return 0;
}

void am_vbo::init(am_buffer_target t) {
    target = t;
    for (int i = 0; i < AM_MAX_VBO_SLOTS; i++) {
        slots[i].id = 0;
        slots[i].last_update_frame = -1;
        slots[i].last_update_start = -1;
        slots[i].last_update_end = -1;
    }
}

void am_vbo::delete_vbo_slots() {
    for (int i = 0; i < AM_MAX_VBO_SLOTS; i++) {
        if (slots[i].id != 0) {
            am_bind_buffer(target, 0);
            am_delete_buffer(slots[i].id);
            slots[i].id = 0;
            slots[i].last_update_frame = -1;
            slots[i].last_update_start = -1;
            slots[i].last_update_end = -1;
        }
    }
}

static am_vbo_slot *get_latest_slot(am_vbo *vbo) {
    am_vbo_slot *latest = NULL;
    for (int i = 0; i < AM_MAX_VBO_SLOTS; i++) {
        am_vbo_slot *s = &vbo->slots[i];
        if (s->id > 0 && (latest == NULL || s->last_update_frame > latest->last_update_frame)) {
            latest = s;
        }
    }
    return latest;
}

static am_vbo_slot *get_earliest_slot(am_vbo *vbo) {
    am_vbo_slot *earliest = NULL;
    for (int i = 0; i < AM_MAX_VBO_SLOTS; i++) {
        am_vbo_slot *s = &vbo->slots[i];
        if (s->id > 0 && (earliest == NULL || s->last_update_frame < earliest->last_update_frame)) {
            earliest = s;
        }
    }
    return earliest;
}

static am_vbo_slot *get_free_slot(am_vbo *vbo) {
    for (int i = 0; i < AM_MAX_VBO_SLOTS; i++) {
        am_vbo_slot *s = &vbo->slots[i];
        if (s->id == 0) {
            //am_debug("created new vbo slot %d", i);
            return s;
        }
    }
    return NULL;
}

static void create_slot(am_vbo *vbo, am_vbo_slot *slot, am_buffer *buf) {
    uint32_t curr_frame = am_global_render_state->render_count;
    slot->id = am_create_buffer_object();
    slot->last_update_frame = curr_frame;
    slot->last_update_start = 0;
    slot->last_update_end = buf->size;
    am_bind_buffer(vbo->target, slot->id);
    am_set_buffer_data(vbo->target, buf->size, &buf->data[0], AM_BUFFER_USAGE_STATIC_DRAW);
}

static void update_slot(am_vbo *vbo, am_vbo_slot *slot, am_buffer *buf, int start, int end) {
    am_bind_buffer(vbo->target, slot->id);
    am_set_buffer_sub_data(vbo->target, start, end - start, buf->data + start);
    uint32_t curr_frame = am_global_render_state->render_count;
    slot->last_update_frame = curr_frame;
    slot->last_update_start = start;
    slot->last_update_end = end;
}

static void compute_update_start_end(am_vbo *vbo, uint32_t since, int *start, int *end) {
    // when updating vbo slot sub-data, we need to make sure we also include ranges in
    // vbo slots that were updated since the vbo slot we're currently using.
    for (int i = 0; i < AM_MAX_VBO_SLOTS; i++) {
        am_vbo_slot *s = &vbo->slots[i];
        if (s->id > 0 && s->last_update_frame > since) {
            *start = am_min(*start, s->last_update_start);
            *end = am_max(*end, s->last_update_end);
        }
    }
}

void am_vbo::create_slot_if_missing(am_buffer *buf) {
    if (slots[0].id == 0) {
        create_slot(this, &slots[0], buf);
    }
}

void am_vbo::update_dirty(am_buffer *buf) {
    uint32_t curr_frame = am_global_render_state->render_count;
    // sanity check: there shouldn't be any slots for the current frame
    for (int i = 0; i < AM_MAX_VBO_SLOTS; i++) {
        if (slots[i].last_update_frame == curr_frame) {
            am_log0("INTERNAL ERROR: vbo slot %d already updated this frame (%d)!", i, curr_frame);
            return;
        }
    }
    am_vbo_slot *earliest = get_earliest_slot(this);
    if (earliest == NULL) {
        create_slot(this, &slots[0], buf);
    } else {
        if (curr_frame - earliest->last_update_frame < AM_MAX_VBO_SLOTS) {
            // too soon to re-use the earliest vbo, so create a new one
            am_vbo_slot *new_slot = get_free_slot(this);
            if (new_slot == NULL) {
                am_log0("INTERNAL ERROR: couldn't find free vbo slot! (%d %d)", curr_frame, earliest->last_update_frame);
                for (int i = 0; i < AM_MAX_VBO_SLOTS; i++) {
                    am_log0("slot %d: %d (%d)", i, slots[i].last_update_frame, slots[i].id);
                }
                return;
            }
            create_slot(this, new_slot, buf);
        } else {
            // it's been long enough since this vbo was updated that we
            // can re-use it.
            int start = buf->dirty_start;
            int end = buf->dirty_end;
            compute_update_start_end(this, earliest->last_update_frame, &start, &end);
            update_slot(this, earliest, buf, start, end);
        }
    }
}

am_buffer_id am_vbo::get_latest_id() {
    am_vbo_slot *latest = get_latest_slot(this);
    if (latest == NULL) return 0;
    return latest->id;
}

static void register_vbo_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L, vbo_gc, 0);
    lua_setfield(L, -2, "__gc");

    am_register_metatable(L, "vbo", MT_am_vbo, 0);
}

void am_open_vbo_module(lua_State *L) {
    register_vbo_mt(L);
}
