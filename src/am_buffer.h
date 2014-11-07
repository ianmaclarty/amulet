enum am_buffer_view_type {
    AM_BUF_ELEM_TYPE_FLOAT,
    AM_BUF_ELEM_TYPE_FLOAT_VEC2,
    AM_BUF_ELEM_TYPE_FLOAT_VEC3,
    AM_BUF_ELEM_TYPE_FLOAT_VEC4,
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

    float get_float(int i);
    void get_float2(int i, am_vec2 *v2);
    void get_float3(int i, am_vec3 *v3);
    void get_float4(int i, am_vec4 *v4);

    void set_float(int i, float f);
    void set_float2(int i, am_vec2 *v2);
    void set_float3(int i, am_vec3 *v3);
    void set_float4(int i, am_vec4 *v4);
};

void am_open_buffer_module(lua_State *L);
void am_buf_view_type_to_attr_client_type_and_dimensions(am_buffer_view_type t, am_attribute_client_type *ctype, int *dims);
