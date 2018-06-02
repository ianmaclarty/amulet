struct am_texture2d;

struct am_buffer : am_nonatomic_userdata {
    int                 size;  // in bytes
    uint8_t             *data;
    const char          *origin;
    int                 origin_ref;
    am_buffer_id        arraybuf_id;
    am_buffer_id        elembuf_id;
    am_texture2d        *texture2d;
    int                 texture2d_ref;
    int                 dirty_start;
    int                 dirty_end;
    bool                track_dirty;
    uint32_t            version;
    am_buffer_usage     usage;

    am_buffer();

    void destroy(lua_State *L);
    void create_arraybuf();
    void create_elembuf();
    void update_if_dirty();

    void mark_dirty(int byte_start, int byte_end) {
        assert(byte_start >= 0);
        assert(byte_end <= size);
        if (track_dirty) {
            if (byte_start < dirty_start) {
                dirty_start = byte_start;
            } 
            if (byte_end > dirty_end) {
                dirty_end = byte_end;
            }
        }
    }
};

struct am_pooled_buffer_slot {
    am_buffer *buf;
    int ref;
};

struct am_buffer_data_allocator : am_nonatomic_userdata {
    am_lua_array<am_pooled_buffer_slot> pooled_buffers;
    am_buffer_data_allocator();
};

// use these instead of am_new_userdata to create a buffer
am_buffer *am_push_new_buffer_and_init(lua_State *L, int size);
am_buffer *am_push_new_buffer_with_data(lua_State *L, int size, void* data);

// use this instead of am_get_userdata for buffers (does some extra checking)
am_buffer* am_check_buffer(lua_State *L, int idx);

void am_open_buffer_module(lua_State *L);
