#define AM_MAX_VBO_SLOTS 1 // set to 1 for now because it needs more testing

// Each am_vbo may contain several slots (actual gpu vbos) which we
// cycle between to avoid contension on vbos between frames.
struct am_vbo_slot {
    am_buffer_id id;
    uint32_t last_update_frame;
    int last_update_start;
    int last_update_end;
};

struct am_vbo {
    am_vbo_slot slots[AM_MAX_VBO_SLOTS];
    am_buffer_target target;

    void init(am_buffer_target t);
    void delete_vbo_slots();

    am_buffer_id get_latest_id();
    void create_slot_if_missing(am_buffer *buf);
    void update_dirty(am_buffer *buf);
};

void am_open_vbo_module(lua_State *L);
