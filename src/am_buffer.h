// grep for AM_NUM_VIEW_TYPES when updating the following enum
enum am_buffer_view_type {
    AM_VIEW_TYPE_FLOAT,
    AM_VIEW_TYPE_FLOAT2,
    AM_VIEW_TYPE_FLOAT3,
    AM_VIEW_TYPE_FLOAT4,
    AM_VIEW_TYPE_UBYTE,
    AM_VIEW_TYPE_BYTE,
    AM_VIEW_TYPE_UBYTE_NORM,
    AM_VIEW_TYPE_BYTE_NORM,
    AM_VIEW_TYPE_USHORT,
    AM_VIEW_TYPE_SHORT,
    AM_VIEW_TYPE_USHORT_ELEM,
    AM_VIEW_TYPE_USHORT_NORM,
    AM_VIEW_TYPE_SHORT_NORM,
    AM_VIEW_TYPE_UINT,
    AM_VIEW_TYPE_INT,
    AM_VIEW_TYPE_UINT_ELEM,

    AM_NUM_VIEW_TYPES,
};

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

    am_buffer();
    am_buffer(int sz);
    void destroy();
    void create_arraybuf();
    void create_elembuf();
    void update_if_dirty();

    void mark_dirty(int byte_start, int byte_end) {
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

struct am_buffer_view : am_nonatomic_userdata {
    am_buffer           *buffer;
    int                 buffer_ref;
    int                 offset; // in bytes
    int                 stride; // in bytes
    int                 size;   // number of elements
    int                 max_size;
    am_buffer_view_type type;
    int                 type_size; // size of each element in bytes
    bool                normalized;
    uint32_t            max_elem;
    uint32_t            last_max_elem_version;

    uint8_t* view_index_ptr(am_buffer_view *view, int i) {
        return &view->buffer->data[view->offset + view->stride * i];
    }

    float get_float(int i) {
        return *((float*)view_index_ptr(this, i));
    }

    void get_float2(int i, am_vec2 *v2) {
        memcpy(&v2->v, view_index_ptr(this, i), sizeof(float) * 2);
    }

    void get_float3(int i, am_vec3 *v3) {
        memcpy(&v3->v, view_index_ptr(this, i), sizeof(float) * 3);
    }

    void get_float4(int i, am_vec4 *v4) {
        memcpy(&v4->v, view_index_ptr(this, i), sizeof(float) * 4);
    }

    void set_float(int i, float f) {
        *((float*)view_index_ptr(this, i)) = f;
    }

    void set_float2(int i, am_vec2 *v2) {
        memcpy(view_index_ptr(this, i), &v2->v, sizeof(float) * 2);
    }

    void set_float3(int i, am_vec3 *v3) {
        memcpy(view_index_ptr(this, i), &v3->v, sizeof(float) * 3);
    }

    void set_float4(int i, am_vec4 *v4) {
        memcpy(view_index_ptr(this, i), &v4->v, sizeof(float) * 4);
    }

    int8_t get_byte(int i) {
        return *((int8_t*)view_index_ptr(this, i));
    }

    void set_byte(int i, int8_t v) {
        *((int8_t*)view_index_ptr(this, i)) = v;
    }

    uint8_t get_ubyte(int i) {
        return *((uint8_t*)view_index_ptr(this, i));
    }

    void set_ubyte(int i, uint8_t v) {
        *((uint8_t*)view_index_ptr(this, i)) = v;
    }

    uint16_t get_ushort(int i) {
        return *((uint16_t*)view_index_ptr(this, i));
    }

    void set_ushort(int i, uint16_t v) {
        *((uint16_t*)view_index_ptr(this, i)) = v;
    }

    int16_t get_short(int i) {
        return *((int16_t*)view_index_ptr(this, i));
    }

    void set_short(int i, int16_t v) {
        *((int16_t*)view_index_ptr(this, i)) = v;
    }

    uint32_t get_uint(int i) {
        return *((uint32_t*)view_index_ptr(this, i));
    }

    void set_uint(int i, uint32_t v) {
        *((uint32_t*)view_index_ptr(this, i)) = v;
    }

    int32_t get_int(int i) {
        return *((int32_t*)view_index_ptr(this, i));
    }

    void set_int(int i, int32_t v) {
        *((int32_t*)view_index_ptr(this, i)) = v;
    }

    void update_max_elem_if_required();
};

void am_open_buffer_module(lua_State *L);
