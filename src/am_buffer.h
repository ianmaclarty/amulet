enum am_buffer_view_type {
    AM_BUF_ELEM_TYPE_FLOAT,
    AM_BUF_ELEM_TYPE_FLOAT2,
    AM_BUF_ELEM_TYPE_FLOAT3,
    AM_BUF_ELEM_TYPE_FLOAT4,
    AM_BUF_ELEM_TYPE_UBYTE,
};

struct am_vbo_info {
    am_buffer_id buffer_id; // 0 if no vbo for this buffer
    int dirty_start;
    int dirty_end;

    am_vbo_info();
};

struct am_buffer : am_nonatomic_userdata {
    int                 size;  // in bytes
    uint8_t             *data;
    am_vbo_info         vbo;

    am_buffer(int sz);
    void destroy();
    void create_vbo();
};

struct am_buffer_view : am_nonatomic_userdata {
    am_buffer           *buffer;
    int                 buffer_ref;
    int                 offset; // in bytes
    int                 stride; // in bytes
    int                 size;   // number of elements
    am_buffer_view_type type;
    int                 type_size; // size of each element in bytes
    bool                normalized;

    inline uint8_t* view_index_ptr(am_buffer_view *view, int i) {
        return &view->buffer->data[view->offset + view->stride * i];
    }

    inline float get_float(int i) {
        return *((float*)view_index_ptr(this, i));
    }

    inline void get_float2(int i, am_vec2 *v2) {
        memcpy(&v2->v, view_index_ptr(this, i), sizeof(float) * 2);
    }

    inline void get_float3(int i, am_vec3 *v3) {
        memcpy(&v3->v, view_index_ptr(this, i), sizeof(float) * 3);
    }

    inline void get_float4(int i, am_vec4 *v4) {
        memcpy(&v4->v, view_index_ptr(this, i), sizeof(float) * 4);
    }

    inline void set_float(int i, float f) {
        *((float*)view_index_ptr(this, i)) = f;
    }

    inline void set_float2(int i, am_vec2 *v2) {
        memcpy(view_index_ptr(this, i), &v2->v, sizeof(float) * 2);
    }

    inline void set_float3(int i, am_vec3 *v3) {
        memcpy(view_index_ptr(this, i), &v3->v, sizeof(float) * 3);
    }

    inline void set_float4(int i, am_vec4 *v4) {
        memcpy(view_index_ptr(this, i), &v4->v, sizeof(float) * 4);
    }

    inline uint8_t get_ubyte(int i) {
        return *((uint8_t*)view_index_ptr(this, i));
    }

    inline void set_ubyte(int i, uint8_t v) {
        *((uint8_t*)view_index_ptr(this, i)) = v;
    }
};

void am_open_buffer_module(lua_State *L);
void am_buf_view_type_to_attr_client_type_and_dimensions(am_buffer_view_type t, am_attribute_client_type *ctype, int *dims);
