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

    void update_max_elem_if_required();
};

// use this instead of am_new_userdata to create a view
am_buffer_view* am_new_buffer_view(lua_State *L, am_buffer_view_type type);

// use this instead of am_get_userdata (does some extra checking)
am_buffer_view* am_check_buffer_view(lua_State *L, int idx);

int am_create_buffer_view(lua_State *L);
int am_view_op_add(lua_State *L);
int am_view_op_sub(lua_State *L);
int am_view_op_mul(lua_State *L);
int am_view_op_div(lua_State *L);

void am_open_view_module(lua_State *L);
