#include "amulet.h"

am_buffer_data_allocator::am_buffer_data_allocator() {
    pooled_buffers.owner = this;
}

am_buffer_data_allocator* get_buffer_data_allocator(lua_State *L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_BUFFER_DATA_ALLOCATOR);
    am_buffer_data_allocator *ballocator = (am_buffer_data_allocator*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ballocator;
}

static void alloc_buffer_data(lua_State *L, int size, am_buffer *buf, int buf_idx) {
    buf->size = size;
    if (size == 0) {
        buf->data = NULL;
        return;
    }
    am_buffer_data_allocator *ballocator = get_buffer_data_allocator(L);
    buf->data = (uint8_t*)malloc(size);
    memset(buf->data, 0, size);
    if (ballocator->pooled_buffers.size > 0) {
        // we're using a pool, so add the buffer to the pool
        am_pooled_buffer_slot slot;
        slot.buf = buf;
        slot.ref = ballocator->ref(L, buf_idx);
        ballocator->pooled_buffers.push_back(L, slot);
    }
}

static void free_buffer_data(lua_State *L, am_buffer *buf) {
    if (buf->data == NULL) return;
    free(buf->data);
    buf->data = NULL;
}

static void push_buffer_pool(lua_State *L) {
    am_buffer_data_allocator *ballocator = get_buffer_data_allocator(L);
    am_pooled_buffer_slot slot;
    // we use a NULL slot to mark the start of this pool
    slot.buf = NULL;
    slot.ref = LUA_NOREF;
    ballocator->pooled_buffers.push_back(L, slot);
}

static void pop_buffer_pool(lua_State *L) {
    am_buffer_data_allocator *ballocator = get_buffer_data_allocator(L);
    for (int i = ballocator->pooled_buffers.size - 1; i >= 0; i--) {
        am_pooled_buffer_slot slot = ballocator->pooled_buffers.arr[i];
        ballocator->pooled_buffers.remove(i);
        if (slot.buf != NULL) {
            slot.buf->destroy(L);
            ballocator->unref(L, slot.ref);
        } else {
            return; // start of pool marker reached
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
    origin = "unnamed buffer";
    origin_ref = LUA_NOREF;
    arraybuf_id = 0;
    elembuf_id = 0;
    texture2d = NULL;
    texture2d_ref = LUA_NOREF;
    dirty_start = INT_MAX;
    dirty_end = 0;
    track_dirty = false;
    version = 1;
    usage = AM_BUFFER_USAGE_STATIC_DRAW;
}

am_buffer *am_push_new_buffer_and_init(lua_State *L, int size) {
    am_buffer *buf = am_new_userdata(L, am_buffer);
    alloc_buffer_data(L, size, buf, -1);
    return buf;
}

am_buffer *am_push_new_buffer_with_data(lua_State *L, int size, void* data) {
    am_buffer *buf = am_new_userdata(L, am_buffer);
    buf->data = (uint8_t*)data;
    buf->size = size;
    return buf;
}

void am_buffer::destroy(lua_State *L) {
    free_buffer_data(L, this);
    if (arraybuf_id != 0) {
        am_bind_buffer(AM_ARRAY_BUFFER, 0);
        am_delete_buffer(arraybuf_id);
        arraybuf_id = 0;
    }
    if (elembuf_id != 0) {
        am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, 0);
        am_delete_buffer(elembuf_id);
        elembuf_id = 0;
    }
}

void am_buffer::update_if_dirty() {
    if (data != NULL && dirty_start < dirty_end) {
        if (arraybuf_id != 0) {
            am_bind_buffer(AM_ARRAY_BUFFER, arraybuf_id);
            am_set_buffer_sub_data(AM_ARRAY_BUFFER, dirty_start, dirty_end - dirty_start, data + dirty_start);
        } 
        if (elembuf_id != 0) {
            am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, elembuf_id);
            am_set_buffer_sub_data(AM_ELEMENT_ARRAY_BUFFER, dirty_start, dirty_end - dirty_start, data + dirty_start);
        } 
        if (texture2d != NULL) {
            texture2d->update_from_image_buffer();
        }
        dirty_start = INT_MAX;
        dirty_end = 0;
        version++;
    }
}

void am_buffer::create_arraybuf() {
    assert(arraybuf_id == 0);
    if (data == NULL) return;
    update_if_dirty();
    arraybuf_id = am_create_buffer_object();
    am_bind_buffer(AM_ARRAY_BUFFER, arraybuf_id);
    am_set_buffer_data(AM_ARRAY_BUFFER, size, &data[0], usage);
    track_dirty = true;
}

void am_buffer::create_elembuf() {
    assert(elembuf_id == 0);
    if (data == NULL) return;
    update_if_dirty();
    elembuf_id = am_create_buffer_object();
    am_bind_buffer(AM_ELEMENT_ARRAY_BUFFER, elembuf_id);
    am_set_buffer_data(AM_ELEMENT_ARRAY_BUFFER, size, &data[0], usage);
    track_dirty = true;
}

static int create_buffer(lua_State *L) {
    am_check_nargs(L, 1);
    int size = luaL_checkinteger(L, 1);
    if (size <= 0) return luaL_error(L, "size should be greater than 0");
    am_push_new_buffer_and_init(L, size);
    return 1;
}

am_buffer* am_check_buffer(lua_State *L, int idx) {
    am_buffer *buf = am_get_userdata(L, am_buffer, idx);
    if (buf->data == NULL && buf->size > 0) {
        luaL_error(L, "attempt to access freed buffer");
    }
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
        buf->origin_ref = buf->ref(L, 1);
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
    int buf_sz = buf->size;
    int b64_sz = (buf_sz + 2) / 3 * 4;
    char *b64_str = (char*)malloc(b64_sz);
    int i = 0;
    int j = 0;
    int extra = 0;
    int b1, b2, b3;
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
        int triple = b1 << 16 | b2 << 8 | b3;
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

static int free_buffer(lua_State *L) {
    am_buffer *buf = am_get_userdata(L, am_buffer, 1);
    if (buf->data != NULL) {
        buf->destroy(L);
    }
    return 0;
}

static int release_vbo(lua_State *L) {
    am_buffer *buf = am_check_buffer(L, 1);
    if (buf->arraybuf_id != 0) {
        am_delete_buffer(buf->arraybuf_id);
        buf->arraybuf_id = 0;
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
    lua_pushcclosure(L, free_buffer, 0);
    lua_setfield(L, -2, "__gc");

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
}

static void register_buffer_data_allocator_mt(lua_State *L) {
    lua_newtable(L);
    am_register_metatable(L, "buffer_data_allocator", MT_am_buffer_data_allocator, 0);
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
