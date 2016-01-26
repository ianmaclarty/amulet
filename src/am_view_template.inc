#ifndef TNAME
#error TNAME undefined
#endif
#ifndef CTYPE
#error CTYPE undefined
#endif
#ifndef GET_CTYPE
#error GET_CTYPE undefined
#endif
#ifndef PUSH_CTYPE
#error PUSH_CTYPE undefined
#endif
#ifndef LUA_TYPE
#error LUA_TYPE undefined
#endif
#if (LUA_TYPE != LUA_TNUMBER) && !defined(VEC_SZ)
#error VEC_SZ undefined
#endif
#if (LUA_TYPE != LUA_TNUMBER) && !defined(GET_VEC_COMPONENT)
#error GET_VEC_COMPONENT undefined
#endif

static int AM_CONCAT(TNAME,_view_index)(lua_State *L) {
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 1);
    int index = lua_tointeger(L, 2);
    if (index < 1 || index > view->size) {
        return am_default_index_func(L);
    }
    uint8_t *ptr = view->buffer->data + view->offset + view->stride * (index-1);
    PUSH_CTYPE(L, *((CTYPE*)ptr));
    return 1;
}

static int AM_CONCAT(TNAME,_view_newindex)(lua_State *L) {
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 1);
    int index = lua_tointeger(L, 2);
    if (index < 1 || index > view->size) {
        if (lua_isnumber(L, 2)) {
            return luaL_error(L, "view index %d not in range [1, %d]", index, view->size);
        } else {
            return luaL_error(L, "view index must be an integer (in fact a %s)", lua_typename(L, lua_type(L, 2)));
        }
    }
    if (am_get_type(L, 3) != LUA_TYPE) {
        return luaL_error(L, "expecting assigned value to be a %s (in fact a %s)",
            am_get_typename(L, LUA_TYPE), am_get_typename(L, am_get_type(L, 3)));
    }
    am_buffer *buf = view->buffer;
    int offset = view->offset + view->stride * (index-1);
    uint8_t *ptr = buf->data + offset;
    *((CTYPE*)ptr) = GET_CTYPE(L, 3);
    view->buffer->mark_dirty(offset, offset + sizeof(CTYPE));
    return 0;
}

#ifdef VEC_SZ
static void AM_CONCAT(TNAME,_view_set_vecs_from_num_table)(lua_State *L, am_buffer_view *view, int idx, int sz) {
    int size = am_min(sz, view->size);
    int stride = view->stride;
    int start_offset = view->offset;
    int end_offset = start_offset + stride * size;
    am_buffer *buf = view->buffer;
    uint8_t *ptr = buf->data + start_offset;
    uint8_t *end = buf->data + end_offset;
    int j = 1;
    while (ptr < end) {
        CTYPE val;
        for (int i = 0; i < VEC_SZ; i++) {
            lua_rawgeti(L, idx, j);
            int t = lua_type(L, -1);
            if (t == LUA_TNIL) {
                if (i == 0) {
                    lua_pop(L, 1);
                    goto done;
                } else {
                    luaL_error(L, "table length should be divisible by %d (in fact %d)",
                        VEC_SZ, j-1);
                    return;
                }
            } else if (t != LUA_TNUMBER) {
                luaL_error(L, "unexpected %s in table at index %d (expecting only numbers)",
                    am_get_typename(L, t), j);
                return;
            }
            val[i] = GET_VEC_COMPONENT(L, -1);
            lua_pop(L, 1);
            j++;
        }
        *((CTYPE*)ptr) = val;
        ptr += stride;
    }
    done:
    assert((j - 1) % VEC_SZ == 0);
    int len = (j-1)/VEC_SZ;
    buf->mark_dirty(start_offset, start_offset + (len-1) * stride + sizeof(CTYPE));
}
#endif

static void AM_CONCAT(TNAME,_view_set_from_table)(lua_State *L, am_buffer_view *view, int idx, int sz) {
    lua_rawgeti(L, idx, 1);
    int first_type = lua_type(L, -1);
    lua_pop(L, 1);
    if (first_type == LUA_TNIL) {
        return;
    }
#ifdef VEC_SZ
    if (first_type == LUA_TNUMBER) {
        AM_CONCAT(TNAME,_view_set_vecs_from_num_table(L, view, idx, sz));
        return;
    }
#endif
    int size = am_min(sz, view->size);
    int stride = view->stride;
    int start_offset = view->offset;
    int end_offset = start_offset + stride * size;
    am_buffer *buf = view->buffer;
    uint8_t *ptr = buf->data + start_offset;
    uint8_t *end = buf->data + end_offset;
    int j = 1;
    while (ptr < end) {
        lua_rawgeti(L, idx, j);
#ifdef VEC_SZ
        int t = am_get_type(L, -1);
#else
        int t = lua_type(L, -1);
#endif
        if (t != LUA_TYPE) {
            //The following crashes Chrome 47.0.2526.106 (64-bit):
            //if (t == LUA_TNIL) {
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                break;
            } else {
                luaL_error(L, "unexpected %s in table at index %d (expecting only %ss)",
                    am_get_typename(L, t), j, am_get_typename(L, LUA_TYPE));
                return;
            }
        }
        *((CTYPE*)ptr) = GET_CTYPE(L, -1);
        lua_pop(L, 1);
        ptr += stride;
        j++;
    }
    buf->mark_dirty(start_offset, start_offset + (j-2) * stride + sizeof(CTYPE));
}

static void AM_CONCAT(TNAME,_view_set_from_single)(lua_State *L, am_buffer_view *view, int idx, int sz) {
    int size = am_min(sz, view->size);
    if (size == 0) return;
    int stride = view->stride;
    am_buffer *buf = view->buffer;
    int start_offset = view->offset;
    int end_offset = start_offset + stride * (size-1) + sizeof(CTYPE);
    uint8_t *ptr = buf->data + start_offset;
    uint8_t *end = ptr + stride * size;
    CTYPE val = GET_CTYPE(L, idx);
    while (ptr < end) {
        *((CTYPE*)ptr) = val;
        ptr += stride;
    }
    buf->mark_dirty(start_offset, end_offset);
}

static void AM_CONCAT(TNAME,_view_set_from_view)(lua_State *L, am_buffer_view *dest, am_buffer_view *src, int sz) {
    int dest_size = am_min(sz, dest->size);
    int dest_stride = dest->stride;
    am_buffer *dest_buf = dest->buffer;
    int dest_start_offset = dest->offset;
    uint8_t *dest_ptr = dest_buf->data + dest_start_offset;
    uint8_t *dest_end = dest_ptr + dest_stride * dest_size;
    int src_size = src->size;
    int src_stride = src->stride;
    uint8_t *src_ptr = src->buffer->data + src->offset;
    uint8_t *src_end = src_ptr + src_stride * src_size;

    if (dest_size == 0 || src_size == 0) return;

    if (dest->type == src->type) {
        while (dest_ptr < dest_end && src_ptr < src_end) {
            *((CTYPE*)dest_ptr) = *((CTYPE*)src_ptr);
            dest_ptr += dest_stride;
            src_ptr += src_stride;
        }
    } else {
        // XXX handle the case where src and dest are vectors
        // of the same dimensions with different, but convertable, types.
        lua_Number (*read_num)(uint8_t*) = view_number_reader[src->type];
        if (read_num == NULL) {
            luaL_error(L, "cannot convert a %s view to a %s view",
                view_type_name[src->type], view_type_name[dest->type]);
        }
        while (dest_ptr < dest_end && src_ptr < src_end) {
            lua_pushnumber(L, read_num(src_ptr));
            *((CTYPE*)dest_ptr) = GET_CTYPE(L, -1);
            lua_pop(L, 1);
            dest_ptr += dest_stride;
            src_ptr += src_stride;
        }
    }

    dest_buf->mark_dirty(dest_start_offset, (dest_ptr - dest_buf->data) - dest_stride + sizeof(CTYPE));
}

static int AM_CONCAT(TNAME,_view_set)(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 1);
    int sz = INT_MAX;
    if (nargs > 2) {
        sz = luaL_checkinteger(L, 3);
        if (sz < 0) {
            return luaL_error(L, "size can't be negative");
        }
    }
    switch (am_get_type(L, 2)) {
        case LUA_TTABLE:
            AM_CONCAT(TNAME,_view_set_from_table)(L, view, 2, sz);
            break;
        case LUA_TNUMBER:
        case MT_am_vec2:
        case MT_am_vec3:
        case MT_am_vec4:
            AM_CONCAT(TNAME,_view_set_from_single)(L, view, 2, sz);
            break;
        case MT_am_buffer_view:
            AM_CONCAT(TNAME,_view_set_from_view)(L, view, am_get_userdata(L, am_buffer_view, 2), sz);
            break;
        default:
            return luaL_error(L, "expecting a number, table or view in position 2 (got %s[%d])",
                am_get_typename(L, 2), am_get_type(L, 2));
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static void AM_CONCAT3(register_,TNAME,_view_mt)(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L, AM_CONCAT(TNAME,_view_index), 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, AM_CONCAT(TNAME,_view_newindex), 0);
    lua_setfield(L, -2, "__newindex");

    lua_pushcclosure(L, AM_CONCAT(TNAME,_view_set), 0);
    lua_setfield(L, -2, "set");

    lua_pushvalue(L, -1);
    am_register_metatable(L, AM_STR(TNAME) "_view", AM_CONCAT(MT_VIEW_TYPE_,TNAME), MT_am_buffer_view);

    // set the metatable id to MT_am_buffer_view so that am_get_type returns that.
    // this makes typechecking easier in some spots (e.g. am_program.cpp).
    lua_pushinteger(L, MT_am_buffer_view);
    lua_rawseti(L, -2, AM_METATABLE_ID_INDEX);
    lua_pop(L, 1);
}

#undef TNAME
#undef CTYPE
#undef GET_CTYPE
#undef PUSH_CTYPE
#undef VEC_SZ
#undef GET_VEC_COMPONENT
#undef LUA_TYPE
