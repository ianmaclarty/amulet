enum am_buffer_view_type {
    AM_BUF_ELEM_TYPE_FLOAT,
    AM_BUF_ELEM_TYPE_FLOAT_VEC2,
    AM_BUF_ELEM_TYPE_FLOAT_VEC3,
    AM_BUF_ELEM_TYPE_FLOAT_VEC4,
    AM_BUF_ELEM_TYPE_BYTE,
    AM_BUF_ELEM_TYPE_BYTE_VEC2,
    AM_BUF_ELEM_TYPE_BYTE_VEC3,
    AM_BUF_ELEM_TYPE_BYTE_VEC4,
    AM_BUF_ELEM_TYPE_UNSIGNED_BYTE,
    AM_BUF_ELEM_TYPE_UNSIGNED_BYTE_VEC2,
    AM_BUF_ELEM_TYPE_UNSIGNED_BYTE_VEC3,
    AM_BUF_ELEM_TYPE_UNSIGNED_BYTE_VEC4,
    AM_BUF_ELEM_TYPE_SHORT,
    AM_BUF_ELEM_TYPE_SHORT_VEC2,
    AM_BUF_ELEM_TYPE_SHORT_VEC3,
    AM_BUF_ELEM_TYPE_SHORT_VEC4,
    AM_BUF_ELEM_TYPE_UNSIGNED_SHORT,
    AM_BUF_ELEM_TYPE_UNSIGNED_SHORT_VEC2,
    AM_BUF_ELEM_TYPE_UNSIGNED_SHORT_VEC3,
    AM_BUF_ELEM_TYPE_UNSIGNED_SHORT_VEC4,
};

struct am_buffer;

struct am_vertex_buffer {
    am_buffer_id buffer_id;
    am_buffer *buffer;
    int buffer_ref; // ref from this vbo to am_buffer object
    int vbo_ref; // ref from am_buffer to this vbo
    int size;
    int dirty_start;
    int dirty_end;
};

struct am_buffer {
    am_vertex_buffer    *vbo;
    int                 size;  // in bytes
    char                data[];
};

struct am_buffer_view {
    am_buffer           *buffer;
    int                 buffer_ref;
    int                 offset; // in bytes
    int                 stride; // in bytes
    int                 size;   // number of elements
    am_buffer_view_type type;
    bool                normalized;
};

void am_open_buffer_module(lua_State *L);
void am_push_new_vertex_buffer(lua_State *L, am_buffer *buf, int buf_idx);
void am_buf_view_type_to_attr_client_type_and_dimensions(am_buffer_view_type t, am_attribute_client_type *ctype, int *dims);
