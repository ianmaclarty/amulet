// grep for AM_NUM_VIEW_TYPES when updating the following enum
enum am_buffer_view_type {
    AM_VIEW_TYPE_F32,
    AM_VIEW_TYPE_F64,
    AM_VIEW_TYPE_U8,
    AM_VIEW_TYPE_I8,
    AM_VIEW_TYPE_U8N,
    AM_VIEW_TYPE_I8N,
    AM_VIEW_TYPE_U16,
    AM_VIEW_TYPE_I16,
    AM_VIEW_TYPE_U16E,
    AM_VIEW_TYPE_U16N,
    AM_VIEW_TYPE_I16N,
    AM_VIEW_TYPE_U32,
    AM_VIEW_TYPE_I32,
    AM_VIEW_TYPE_U32E,
    AM_NUM_VIEW_TYPES,
};

enum am_buffer_view_type_lua {
    AM_VIEW_TYPE_LUA_F32_1,
    AM_VIEW_TYPE_LUA_F32_2,
    AM_VIEW_TYPE_LUA_F32_3,
    AM_VIEW_TYPE_LUA_F32_4,
    AM_VIEW_TYPE_LUA_F32_3x3,
    AM_VIEW_TYPE_LUA_F32_4x4,
    AM_VIEW_TYPE_LUA_F64_1,
    AM_VIEW_TYPE_LUA_F64_2,
    AM_VIEW_TYPE_LUA_F64_3,
    AM_VIEW_TYPE_LUA_F64_4,
    AM_VIEW_TYPE_LUA_F64_3x3,
    AM_VIEW_TYPE_LUA_F64_4x4,
    AM_VIEW_TYPE_LUA_U8_1,
    AM_VIEW_TYPE_LUA_U8_2,
    AM_VIEW_TYPE_LUA_U8_3,
    AM_VIEW_TYPE_LUA_U8_4,
    AM_VIEW_TYPE_LUA_I8_1,
    AM_VIEW_TYPE_LUA_I8_2,
    AM_VIEW_TYPE_LUA_I8_3,
    AM_VIEW_TYPE_LUA_I8_4,
    AM_VIEW_TYPE_LUA_U8N_1,
    AM_VIEW_TYPE_LUA_U8N_2,
    AM_VIEW_TYPE_LUA_U8N_3,
    AM_VIEW_TYPE_LUA_U8N_4,
    AM_VIEW_TYPE_LUA_I8N_1,
    AM_VIEW_TYPE_LUA_I8N_2,
    AM_VIEW_TYPE_LUA_I8N_3,
    AM_VIEW_TYPE_LUA_I8N_4,
    AM_VIEW_TYPE_LUA_U16_1,
    AM_VIEW_TYPE_LUA_U16_2,
    AM_VIEW_TYPE_LUA_U16_3,
    AM_VIEW_TYPE_LUA_U16_4,
    AM_VIEW_TYPE_LUA_I16_1,
    AM_VIEW_TYPE_LUA_I16_2,
    AM_VIEW_TYPE_LUA_I16_3,
    AM_VIEW_TYPE_LUA_I16_4,
    AM_VIEW_TYPE_LUA_U16E,
    AM_VIEW_TYPE_LUA_U16N_1,
    AM_VIEW_TYPE_LUA_U16N_2,
    AM_VIEW_TYPE_LUA_U16N_3,
    AM_VIEW_TYPE_LUA_U16N_4,
    AM_VIEW_TYPE_LUA_I16N_1,
    AM_VIEW_TYPE_LUA_I16N_2,
    AM_VIEW_TYPE_LUA_I16N_3,
    AM_VIEW_TYPE_LUA_I16N_4,
    AM_VIEW_TYPE_LUA_U32_1,
    AM_VIEW_TYPE_LUA_U32_2,
    AM_VIEW_TYPE_LUA_U32_3,
    AM_VIEW_TYPE_LUA_U32_4,
    AM_VIEW_TYPE_LUA_I32_1,
    AM_VIEW_TYPE_LUA_I32_2,
    AM_VIEW_TYPE_LUA_I32_3,
    AM_VIEW_TYPE_LUA_I32_4,
    AM_VIEW_TYPE_LUA_U32E,
};

struct am_view_type_info {
    const char *name;
    int size;
    bool normalized;
    am_buffer_view_type base_type;
    bool can_be_gl_attrib;
    am_attribute_client_type gl_client_type;
    lua_Number(*num_reader)(uint8_t*);
};

extern am_view_type_info am_view_type_infos[];

struct am_buffer_view : am_nonatomic_userdata {
    am_buffer_view_type type;
    int                 components;

    am_buffer           *buffer;
    int                 buffer_ref;

    int                 offset; // in bytes
    int                 stride; // in bytes
    int                 size;   // number of elements

    uint32_t            max_elem;
    uint32_t            last_max_elem_version;

    am_buffer_view();

    bool is_normalized();
    bool can_be_gl_attrib();
    am_attribute_client_type gl_client_type();

    void update_max_elem_if_required();

    void mark_dirty(int start, int end) {
        int start_bytes = offset + start * stride;
        int end_bytes = offset + (end - 1) * stride + components * am_view_type_infos[type].size;
        buffer->mark_dirty(start_bytes, end_bytes);
    }
};

// use this instead of am_new_userdata to create a view
am_buffer_view* am_new_buffer_view(lua_State *L, am_buffer_view_type type, int components);

// use this instead of am_get_userdata (does some extra checking)
am_buffer_view* am_check_buffer_view(lua_State *L, int idx);

int am_create_buffer_view(lua_State *L);
int am_view_op_add(lua_State *L);
int am_view_op_sub(lua_State *L);
int am_view_op_mul(lua_State *L);
int am_view_op_div(lua_State *L);

void am_open_view_module(lua_State *L);
