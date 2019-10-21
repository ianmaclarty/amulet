#include "amulet.h"

static size_t total_buffer_malloc_bytes = 0;

am_buffer_data_allocator::am_buffer_data_allocator() {
    pooled_buffers.owner = this;
    pool_scratch = NULL;
    pool_scratch_capacity = 0;
    pool_used = 0;
    pool_hwm = 0;
}

am_buffer_data_allocator* get_buffer_data_allocator(lua_State *L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_BUFFER_DATA_ALLOCATOR);
    am_buffer_data_allocator *a = (am_buffer_data_allocator*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return a;
}

static void prepare_pool(am_buffer_data_allocator *a) {
    if (a->pool_used == 0 
        && a->pool_hwm > a->pool_scratch_capacity)
    {
        // hwm has increased since last time we used the pool, so extend
        if (a->pool_scratch != NULL) free(a->pool_scratch);
        a->pool_scratch = (uint8_t*)malloc(a->pool_hwm);
        a->pool_scratch_capacity = a->pool_hwm;
        a->pool_hwm = 0;
    } else if (a->pool_used == 0 && a->pool_scratch_capacity > 0) {
        // starting to reuse an existing pool, reset hwm
        assert(a->pool_scratch != NULL);
        a->pool_hwm = 0;
    }
}

static void alloc_from_pool(lua_State *L, am_buffer_data_allocator *a, am_buffer *buf, int size, int buf_idx) {
    if (a->pool_used + size <= a->pool_scratch_capacity) {
        // enough space in scratch area, so alloc there
        buf->data = &a->pool_scratch[a->pool_used];
        buf->alloc_method = AM_BUF_ALLOC_POOL_SCRATCH;
    } else {
        // scratch area full, so use malloc
        buf->data = (uint8_t*)malloc(size);
        buf->alloc_method = AM_BUF_ALLOC_MALLOC;
        total_buffer_malloc_bytes += size;
    }
    buf->size = size;
    a->pool_used += size;
    am_align_size(a->pool_used);
    a->pool_hwm = am_max(a->pool_hwm, a->pool_used);
    am_pooled_buffer_slot slot;
    slot.buf = buf;
    slot.ref = a->ref(L, buf_idx);
    a->pooled_buffers.push_back(L, slot);
}

static void push_buffer_pool(lua_State *L) {
    am_buffer_data_allocator *a = get_buffer_data_allocator(L);
    prepare_pool(a);
    am_pooled_buffer_slot slot;
    // We use a NULL slot to mark the start of this pool
    // We record the current used marker in the ref
    slot.buf = NULL;
    slot.ref = a->pool_used;
    a->pooled_buffers.push_back(L, slot);
}

static void pop_buffer_pool(lua_State *L) {
    am_buffer_data_allocator *a = get_buffer_data_allocator(L);
    for (int i = a->pooled_buffers.size - 1; i >= 0; i--) {
        am_pooled_buffer_slot slot = a->pooled_buffers.arr[i];
        a->pooled_buffers.remove(i);
        if (slot.buf != NULL) {
            slot.buf->free_data();
            a->unref(L, slot.ref);
        } else {
            // start of pool marker reached, reset scratch area
            a->pool_used = slot.ref;
            return; 
        }
    }
}

static int run_with_buffer_pool(lua_State *L) {
    am_check_nargs(L, 1);
    push_buffer_pool(L);
    // XXX if the called function errors, pop_buffer_pool won't run and this will
    // be a leak. Probably not a big deal though, since this would normally end
    // the program too.
    lua_call(L, 0, 0);
    pop_buffer_pool(L);
    return 0;
}

am_buffer::am_buffer() {
    size = 0;
    data = NULL;
    arraybuf = NULL;
    elembuf = NULL;
    texture2d = NULL;
    dirty_start = INT_MAX;
    dirty_end = 0;
    version = 1;
    alloc_method = AM_BUF_ALLOC_LUA;
    origin = "anonymous buffer";
    usage = AM_BUFFER_USAGE_STATIC_DRAW;
}

am_buffer *am_push_new_buffer_and_init(lua_State *L, int size) {
    am_buffer *buf;
    if (size == 0) {
        buf = am_new_userdata(L, am_buffer);
        buf->size = 0;
        buf->data = NULL;
        buf->alloc_method = AM_BUF_ALLOC_LUA;
        return buf;
    }
    am_buffer_data_allocator *a = get_buffer_data_allocator(L);
    if (a->pooled_buffers.size > 0) {
        // we're using the pool
        buf = am_new_userdata(L, am_buffer);
        alloc_from_pool(L, a, buf, size, -1);
    } else {
        if (size <= am_conf_buffer_malloc_threshold) {
            // alloc buffer and data as one lua userdata
            int data_offset = sizeof(am_buffer);
            am_align_size(data_offset);
            buf = new(lua_newuserdata(L, data_offset + size)) am_buffer();
            am_set_metatable(L, buf, MT_am_buffer);
            buf->data = (uint8_t*)buf + data_offset;
            buf->size = size;
            buf->alloc_method = AM_BUF_ALLOC_LUA;
        } else {
            // use system malloc
            buf = new(lua_newuserdata(L, sizeof(am_buffer))) am_buffer();
            am_set_metatable(L, buf, MT_am_buffer_gc);
            buf->data = (uint8_t*)malloc(size);
            buf->size = size;
            buf->alloc_method = AM_BUF_ALLOC_MALLOC;
            total_buffer_malloc_bytes += size;
        }
    }
    memset(buf->data, 0, size);
    buf->mark_dirty(0, size);
    return buf;
}

// the new buffer will own data and assumes it was allocated
// with system malloc.
am_buffer *am_push_new_buffer_with_data(lua_State *L, int size, void* data) {
    am_buffer *buf = new(lua_newuserdata(L, sizeof(am_buffer))) am_buffer();
    am_set_metatable(L, buf, MT_am_buffer_gc);
    buf->data = (uint8_t*)data;
    buf->size = size;
    buf->alloc_method = AM_BUF_ALLOC_MALLOC;
    total_buffer_malloc_bytes += size;
    return buf;
}

void am_buffer::free_data() {
    if (data == NULL) return;
    switch (alloc_method) {
        case AM_BUF_ALLOC_MALLOC:
            free(data);
            total_buffer_malloc_bytes -= size;
            break;
        case AM_BUF_ALLOC_LUA:
            break;
        case AM_BUF_ALLOC_POOL_SCRATCH:
            break;
    }
    data = NULL;
}

static int free_buffer(lua_State *L) {
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    buf->free_data();
    return 0;
}

void am_buffer::update_if_dirty() {
    if (data != NULL && dirty_start < dirty_end) {
        if (arraybuf != NULL) {
            arraybuf->update_dirty(this);
        } 
        if (elembuf != NULL) {
            elembuf->update_dirty(this);
        } 
        if (texture2d != NULL) {
            texture2d->update_dirty();
        }
        dirty_start = INT_MAX;
        dirty_end = 0;
        version++;
    }
}

void am_buffer::create_arraybuf(lua_State *L) {
    assert(arraybuf == NULL);
    assert(L != NULL);
    arraybuf = am_new_userdata(L, am_vbo);
    arraybuf->init(AM_ARRAY_BUFFER);
    ref(L, -1);
    lua_pop(L, 1);
    mark_dirty(0, size);
}

void am_buffer::create_elembuf(lua_State *L) {
    assert(elembuf == NULL);
    assert(L != NULL);
    elembuf = am_new_userdata(L, am_vbo);
    elembuf->init(AM_ELEMENT_ARRAY_BUFFER);
    ref(L, -1);
    lua_pop(L, 1);
    mark_dirty(0, size);
}

static int create_buffer(lua_State *L) {
    am_check_nargs(L, 1);
    int size = luaL_checkinteger(L, 1);
    if (size < 0) return luaL_error(L, "size should be non-negative");
    am_push_new_buffer_and_init(L, size);
    return 1;
}

uint8_t *am_check_buffer_data(lua_State *L, am_buffer *buf) {
    if (buf->data == NULL && buf->size > 0) {
        luaL_error(L, "attempt to access freed buffer");
        return NULL;
    }
    return buf->data;
}

am_buffer* am_check_buffer(lua_State *L, int idx) {
    am_buffer *buf = am_get_userdata(L, am_buffer, idx);
    am_check_buffer_data(L, buf);
    return buf;
}

static int load_buffer(lua_State *L) {
    am_check_nargs(L, 1);
    const char *filename = luaL_checkstring(L, 1);
    int len;
    char *errmsg;
    void *data = am_read_resource(filename, &len, &errmsg);
    if (data == NULL) {
        free(errmsg);
        lua_pushnil(L);
    } else {
        am_buffer *buf = am_push_new_buffer_with_data(L, len, data);
        buf->origin = filename;
        buf->ref(L, 1);
    }
    return 1;
}

static int load_string(lua_State *L) {
    am_check_nargs(L, 1);
    const char *filename = luaL_checkstring(L, 1);
    int len;
    char *errmsg;
    void *data = am_read_resource(filename, &len, &errmsg);
    if (data == NULL) {
        free(errmsg);
        lua_pushnil(L);
    } else {
        lua_pushlstring(L, (const char*)data, len);
        free(data);
    }
    return 1;
}

static int load_script(lua_State *L) {
    am_check_nargs(L, 1);
    const char *filename = luaL_checkstring(L, 1);
    int len;
    char *errmsg;
    void *data = am_read_resource(filename, &len, &errmsg);
    if (data == NULL) {
        free(errmsg);
        lua_pushnil(L);
    } else {
        luaL_loadbuffer(L, (const char*)data, len, filename);
        free(data);
    }
    return 1;
}

static const char *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_encode(lua_State *L) {
    am_check_nargs(L, 1);
    am_buffer *buf = am_check_buffer(L, 1);
    uint8_t *data = buf->data;
    unsigned int buf_sz = (unsigned int)buf->size;
    unsigned int b64_sz = (buf_sz + 2) / 3 * 4;
    char *b64_str = (char*)malloc(b64_sz);
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int extra = 0;
    unsigned int b1, b2, b3;
    while (i < buf_sz) {
        b1 = data[i++];
        if (i < buf_sz) {
            b2 = data[i++];
        } else {
            b2 = 0;
            extra++;
        }
        if (i < buf_sz) {
            b3 = data[i++];
        } else {
            b3 = 0;
            extra++;
        }
        unsigned int triple = b1 << 16 | b2 << 8 | b3;
        b64_str[j++] = base64[triple >> 18];
        b64_str[j++] = base64[triple >> 12 & 63];
        b64_str[j++] = base64[triple >> 6 & 63];
        b64_str[j++] = base64[triple & 63];
    }
    assert(extra <= 2);
    switch (extra) {
        case 2:
            b64_str[b64_sz-2] = '=';
        case 1:
            b64_str[b64_sz-1] = '=';
    }
    lua_pushlstring(L, b64_str, b64_sz);
    free(b64_str);
    return 1;
}

static int base64_decode(lua_State *L) {
    am_check_nargs(L, 1);
    size_t b64_sz;
    const char *b64_str = (const char*)lua_tolstring(L, 1, &b64_sz);
    if (b64_str == NULL) return luaL_error(L, "expecting a string in position 1");
    if (b64_sz % 4 != 0) return luaL_error(L, "string length should be divisble by 4");
    int buf_sz = b64_sz / 4 * 3;
    if (buf_sz == 0) {
        am_push_new_buffer_and_init(L, 0);
        return 1;
    }
    if (b64_str[b64_sz-1] == '=') buf_sz--;
    if (b64_str[b64_sz-2] == '=') buf_sz--;
    am_buffer *buf = am_push_new_buffer_and_init(L, buf_sz);
    uint8_t *data = buf->data;
    int i = 0;
    int j = 0;
    while (j < (int)b64_sz) {
        int triple = 0;
        for (int k = 0; k < 4; k++) {
            triple <<= 6;
            int c = b64_str[j++];
            switch (c) {
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                case 'F':
                case 'G':
                case 'H':
                case 'I':
                case 'J':
                case 'K':
                case 'L':
                case 'M':
                case 'N':
                case 'O':
                case 'P':
                case 'Q':
                case 'R':
                case 'S':
                case 'T':
                case 'U':
                case 'V':
                case 'W':
                case 'X':
                case 'Y':
                case 'Z':
                    triple |= c - 'A';
                    break;
                case 'a':
                case 'b':
                case 'c':
                case 'd':
                case 'e':
                case 'f':
                case 'g':
                case 'h':
                case 'i':
                case 'j':
                case 'k':
                case 'l':
                case 'm':
                case 'n':
                case 'o':
                case 'p':
                case 'q':
                case 'r':
                case 's':
                case 't':
                case 'u':
                case 'v':
                case 'w':
                case 'x':
                case 'y':
                case 'z':
                    triple |= c - 'a' + 26;
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    triple |= c - '0' + 52;
                    break;
                case '+':
                    triple |= 62;
                    break;
                case '/':
                    triple |= 63;
                    break;
                case '=':
                    break;
                default:
                    return luaL_error(L, "unexpected character in base64 string: %c", (char)c);
            }
        }
        assert(i < buf_sz);
        data[i++] = triple >> 16;
        if (i < buf_sz) {
            data[i++] = triple >> 8 & 255;
        }
        if (i < buf_sz) {
            data[i++] = triple & 255;
        }
    }
    return 1;
}

static int buffer_len(lua_State *L) {
    am_buffer *buf = am_check_buffer(L, 1);
    lua_pushinteger(L, buf->size);
    return 1;
}

static int release_vbo(lua_State *L) {
    am_buffer *buf = am_check_buffer(L, 1);
    if (buf->arraybuf != NULL) {
        buf->arraybuf->delete_vbo_slots();
    }
    return 0;
}

static int mark_buffer_dirty(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_buffer *buf = am_check_buffer(L, 1);
    int start = 0;
    int end = buf->size;
    if (nargs > 1) {
        start = lua_tointeger(L, 2);
        if (start < 0 || start >= buf->size) {
            return luaL_error(L, "start must be in the range 0 to %d", buf->size - 1);
        }
    }
    if (nargs > 2) {
        end = lua_tointeger(L, 3);
        if (end <= 0 || end > buf->size) {
            return luaL_error(L, "end be in the range 1 to %d", buf->size);
        }
    }
    buf->mark_dirty(start, end);
    return 0;
}

static void get_buffer_dataptr(lua_State *L, void *obj) {
    am_buffer *buf = (am_buffer*)obj;
    lua_pushlightuserdata(L, (void*)buf->data);
}

static am_property buffer_dataptr_property = {get_buffer_dataptr, NULL};

static void get_buffer_usage(lua_State *L, void *obj) {
    am_buffer *buf = (am_buffer*)obj;
    am_push_enum(L, am_buffer_usage, buf->usage);
}

static void set_buffer_usage(lua_State *L, void *obj) {
    am_buffer *buf = (am_buffer*)obj;
    buf->usage = am_get_enum(L, am_buffer_usage, 3);
}

static am_property buffer_usage_property = {get_buffer_usage, set_buffer_usage};

static void register_buffer_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, buffer_len, 0);
    lua_setfield(L, -2, "__len");

    am_register_property(L, "dataptr", &buffer_dataptr_property);
    am_register_property(L, "usage", &buffer_usage_property);

    lua_pushcclosure(L, am_create_buffer_view, 0);
    lua_setfield(L, -2, "view");
    lua_pushcclosure(L, release_vbo, 0);
    lua_setfield(L, -2, "release_vbo");
    lua_pushcclosure(L, mark_buffer_dirty, 0);
    lua_setfield(L, -2, "mark_dirty");
    lua_pushcclosure(L, free_buffer, 0);
    lua_setfield(L, -2, "free");

    am_register_metatable(L, "buffer", MT_am_buffer, 0);

    // buffer with gc metamethod
    lua_newtable(L);
    lua_pushcclosure(L, free_buffer, 0);
    lua_setfield(L, -2, "__gc");
    am_register_metatable(L, "buffer_gc", MT_am_buffer_gc, MT_am_buffer);
}

static void register_buffer_data_allocator_mt(lua_State *L) {
    lua_newtable(L);
    am_register_metatable(L, "buffer_data_allocator", MT_am_buffer_data_allocator, 0);
}

static int total_buffer_malloc_mem(lua_State *L) {
    lua_pushnumber(L, ((lua_Number)total_buffer_malloc_bytes) / 1024.0);
    return 1;
}

static int buffer_pool_mem(lua_State *L) {
    am_buffer_data_allocator *a = get_buffer_data_allocator(L);
    lua_pushnumber(L, ((lua_Number)a->pool_scratch_capacity) / 1024.0);
    return 1;
}

void am_open_buffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"buffer", create_buffer},
        {"load_buffer", load_buffer},
        {"load_string", load_string},
        {"load_script", load_script},
        {"base64_encode", base64_encode},
        {"base64_decode", base64_decode},
        {"buffer_pool", run_with_buffer_pool},
        {"_buffer_pool_mem", buffer_pool_mem},
        {"_total_buffer_malloc_mem", total_buffer_malloc_mem},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);

    register_buffer_data_allocator_mt(L);
    am_new_userdata(L, am_buffer_data_allocator); 
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_BUFFER_DATA_ALLOCATOR);

    am_enum_value buf_usage_enum[] = {
        {"static",          AM_BUFFER_USAGE_STATIC_DRAW},
        {"dynamic",         AM_BUFFER_USAGE_DYNAMIC_DRAW},
        {"stream",          AM_BUFFER_USAGE_STREAM_DRAW},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_buffer_usage, buf_usage_enum);

    register_buffer_mt(L);
}
