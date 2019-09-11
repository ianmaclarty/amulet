#ifndef TNAME
#error TNAME undefined
#endif
#ifndef CTYPE
#error CTYPE undefined
#endif
#ifndef FROM_LUA_NUM
#error FROM_LUA_NUM undefined
#endif
#ifndef READ_NUM
#error READ_NUM undefined
#endif

static int AM_CONCAT(TNAME,_view_index)(lua_State *L) {
    am_buffer_view *view = am_check_buffer_view(L, 1);
    int index = lua_tointeger(L, 2);
    if (index < 1 || index > view->size) {
        return view_swizzle_index(L, view);
    }
    uint8_t *ptr = view->buffer->data + view->offset + view->stride * (index-1);
    switch (view->components) {
        case 1: {
            lua_pushnumber(L, READ_NUM(ptr));
            return 1;
        }
        case 2: {
            am_vec2 *v = am_new_userdata(L, am_vec2);
            v->v.x = READ_NUM(ptr);
            v->v.y = READ_NUM(ptr + sizeof(CTYPE));
            return 1;
        }
        case 3: {
            am_vec3 *v = am_new_userdata(L, am_vec3);
            v->v.x = READ_NUM(ptr);
            v->v.y = READ_NUM(ptr + sizeof(CTYPE) * 1);
            v->v.z = READ_NUM(ptr + sizeof(CTYPE) * 2);
            return 1;
        }
        case 4: {
            am_vec4 *v = am_new_userdata(L, am_vec4);
            v->v.x = READ_NUM(ptr);
            v->v.y = READ_NUM(ptr + sizeof(CTYPE) * 1);
            v->v.z = READ_NUM(ptr + sizeof(CTYPE) * 2);
            v->v.w = READ_NUM(ptr + sizeof(CTYPE) * 3);
            return 1;
        }
        case 9: {
            am_mat3 *m = am_new_userdata(L, am_mat3);
            m->m[0][0] = READ_NUM(ptr);
            m->m[0][1] = READ_NUM(ptr + sizeof(CTYPE) * 1);
            m->m[0][2] = READ_NUM(ptr + sizeof(CTYPE) * 2);
            m->m[1][0] = READ_NUM(ptr + sizeof(CTYPE) * 3);
            m->m[1][1] = READ_NUM(ptr + sizeof(CTYPE) * 4);
            m->m[1][2] = READ_NUM(ptr + sizeof(CTYPE) * 5);
            m->m[2][0] = READ_NUM(ptr + sizeof(CTYPE) * 6);
            m->m[2][1] = READ_NUM(ptr + sizeof(CTYPE) * 7);
            m->m[2][2] = READ_NUM(ptr + sizeof(CTYPE) * 8);
            return 1;
        }
        case 16: {
            am_mat4 *m = am_new_userdata(L, am_mat4);
            m->m[0][0] = READ_NUM(ptr);
            m->m[0][1] = READ_NUM(ptr + sizeof(CTYPE) * 1);
            m->m[0][2] = READ_NUM(ptr + sizeof(CTYPE) * 2);
            m->m[0][3] = READ_NUM(ptr + sizeof(CTYPE) * 3);
            m->m[1][0] = READ_NUM(ptr + sizeof(CTYPE) * 4);
            m->m[1][1] = READ_NUM(ptr + sizeof(CTYPE) * 5);
            m->m[1][2] = READ_NUM(ptr + sizeof(CTYPE) * 6);
            m->m[1][3] = READ_NUM(ptr + sizeof(CTYPE) * 7);
            m->m[2][0] = READ_NUM(ptr + sizeof(CTYPE) * 8);
            m->m[2][1] = READ_NUM(ptr + sizeof(CTYPE) * 9);
            m->m[2][2] = READ_NUM(ptr + sizeof(CTYPE) * 10);
            m->m[2][3] = READ_NUM(ptr + sizeof(CTYPE) * 11);
            m->m[3][0] = READ_NUM(ptr + sizeof(CTYPE) * 12);
            m->m[3][1] = READ_NUM(ptr + sizeof(CTYPE) * 13);
            m->m[3][2] = READ_NUM(ptr + sizeof(CTYPE) * 14);
            m->m[3][3] = READ_NUM(ptr + sizeof(CTYPE) * 15);
            return 1;
        }
    }
    return luaL_error(L, "internal error: unexpected number of components (%d)", view->components);
}

static int AM_CONCAT(TNAME,_view_set_component_field)(lua_State *L, am_buffer_view *view);

static int AM_CONCAT(TNAME,_view_newindex)(lua_State *L) {
    am_buffer_view *view = am_check_buffer_view(L, 1);
    int index = lua_tointeger(L, 2);
    if (index < 1 || index > view->size) {
        if (lua_isnumber(L, 2)) {
            return luaL_error(L, "view index %d not in range [1, %d]", index, view->size);
        } else {
            return AM_CONCAT(TNAME,_view_set_component_field)(L, view);
        }
    }
    am_buffer *buf = view->buffer;
    int offset = view->offset + view->stride * (index-1);
    CTYPE *ptr = (CTYPE*)(buf->data + offset);
    switch (view->components) {
        case 1: {
            ptr[0] = FROM_LUA_NUM(luaL_checknumber(L, 3));
            break;
        }
        case 2: {
            am_vec2 *v = am_get_userdata(L, am_vec2, 3);
            ptr[0] = FROM_LUA_NUM(v->v.x);
            ptr[1] = FROM_LUA_NUM(v->v.y);
            break;
        }
        case 3: {
            am_vec3 *v = am_get_userdata(L, am_vec3, 3);
            ptr[0] = FROM_LUA_NUM(v->v.x);
            ptr[1] = FROM_LUA_NUM(v->v.y);
            ptr[2] = FROM_LUA_NUM(v->v.z);
            break;
        }
        case 4: {
            am_vec4 *v = am_get_userdata(L, am_vec4, 3);
            ptr[0] = FROM_LUA_NUM(v->v.x);
            ptr[1] = FROM_LUA_NUM(v->v.y);
            ptr[2] = FROM_LUA_NUM(v->v.z);
            ptr[3] = FROM_LUA_NUM(v->v.w);
            break;
        }
        case 9: {
            am_mat3 *m = am_get_userdata(L, am_mat3, 3);
            ptr[0] = FROM_LUA_NUM(m->m[0][0]);
            ptr[1] = FROM_LUA_NUM(m->m[0][1]);
            ptr[2] = FROM_LUA_NUM(m->m[0][2]);
            ptr[3] = FROM_LUA_NUM(m->m[1][0]);
            ptr[4] = FROM_LUA_NUM(m->m[1][1]);
            ptr[5] = FROM_LUA_NUM(m->m[1][2]);
            ptr[6] = FROM_LUA_NUM(m->m[2][0]);
            ptr[7] = FROM_LUA_NUM(m->m[2][1]);
            ptr[8] = FROM_LUA_NUM(m->m[2][2]);
            break;
        }
        case 16: {
            am_mat4 *m = am_get_userdata(L, am_mat4, 3);
            ptr[0] = FROM_LUA_NUM(m->m[0][0]);
            ptr[1] = FROM_LUA_NUM(m->m[0][1]);
            ptr[2] = FROM_LUA_NUM(m->m[0][2]);
            ptr[3] = FROM_LUA_NUM(m->m[0][3]);
            ptr[4] = FROM_LUA_NUM(m->m[1][0]);
            ptr[5] = FROM_LUA_NUM(m->m[1][1]);
            ptr[6] = FROM_LUA_NUM(m->m[1][2]);
            ptr[7] = FROM_LUA_NUM(m->m[1][3]);
            ptr[8] =  FROM_LUA_NUM(m->m[2][0]);
            ptr[9] =  FROM_LUA_NUM(m->m[2][1]);
            ptr[10] = FROM_LUA_NUM(m->m[2][2]);
            ptr[11] = FROM_LUA_NUM(m->m[2][3]);
            ptr[12] = FROM_LUA_NUM(m->m[3][0]);
            ptr[13] = FROM_LUA_NUM(m->m[3][1]);
            ptr[14] = FROM_LUA_NUM(m->m[3][2]);
            ptr[15] = FROM_LUA_NUM(m->m[3][3]);
            break;
        }
    }
    view->buffer->mark_dirty(offset, offset + sizeof(CTYPE) * view->components);
    return 0;
}

static void AM_CONCAT(TNAME,_view_set_vecs_from_num_table)(lua_State *L, am_buffer_view *view, int idx, int start, int sz) {
    int size = am_min(sz, view->size - start + 1);
    int stride = view->stride;
    int start_offset = view->offset + (start - 1) * stride;
    int end_offset = start_offset + stride * size;
    am_buffer *buf = view->buffer;
    uint8_t *ptr = buf->data + start_offset;
    uint8_t *end = buf->data + end_offset;
    int j = 1;
    while (ptr < end) {
        for (int i = 0; i < view->components; i++) {
            lua_rawgeti(L, idx, j);
            int t = lua_type(L, -1);
            if (t == LUA_TNUMBER) {
                CTYPE *val = (CTYPE*)ptr;
                val[i] = FROM_LUA_NUM(lua_tonumber(L, -1));
                lua_pop(L, 1);
                j++;
            } else if (t == LUA_TNIL) {
                if (i == 0) {
                    lua_pop(L, 1);
                    goto done;
                } else {
                    luaL_error(L, "table length should be divisible by %d (in fact %d)",
                        view->components, j-1);
                    return;
                }
            } else {
                luaL_error(L, "unexpected %s in table at index %d (expecting only numbers)",
                    am_get_typename(L, t), j);
                return;
            }
        }
        ptr += stride;
    }
    done:
    assert((j - 1) % view->components == 0);
    int len = (j - 1) / view->components;
    buf->mark_dirty(start_offset, start_offset + (len-1) * stride + sizeof(CTYPE) * view->components);
}

static void AM_CONCAT(TNAME,_view_set_from_table)(lua_State *L, am_buffer_view *view, int idx, int start, int sz) {
    lua_rawgeti(L, idx, 1);
    int first_type = lua_type(L, -1);
    lua_pop(L, 1);
    if (first_type == LUA_TNIL) {
        return;
    }
    if (view->components > 1 && first_type == LUA_TNUMBER) {
        AM_CONCAT(TNAME,_view_set_vecs_from_num_table)(L, view, idx, start, sz);
        return;
    }
    int size = am_min(sz, view->size - start + 1);
    int stride = view->stride;
    int start_offset = view->offset + (start - 1) * stride;
    int end_offset = start_offset + stride * size;
    am_buffer *buf = view->buffer;
    uint8_t *ptr = buf->data + start_offset;
    uint8_t *end = buf->data + end_offset;
    int j = 1;
    switch (view->components) {
        case 1: {
            while (ptr < end) {
                lua_rawgeti(L, idx, j);
                int t = lua_type(L, -1);
                if (t == LUA_TNUMBER) {
                    *((CTYPE*)ptr) = FROM_LUA_NUM(lua_tonumber(L, -1));
                    lua_pop(L, 1);
                    ptr += stride;
                    j++;
                } else if (t == LUA_TNIL) {
                    lua_pop(L, 1);
                    break;
                } else {
                    luaL_error(L, "unexpected %s in table at index %d",
                        am_get_typename(L, t), j);
                    return;
                }
            }
            break;
        }
        case 2: {
            while (ptr < end) {
                lua_rawgeti(L, idx, j);
                am_vec2 *v = am_get_userdata_or_nil(L, am_vec2, -1);
                lua_pop(L, 1);
                if (v != NULL) {
                    CTYPE *val = (CTYPE*)ptr;
                    val[0] = FROM_LUA_NUM(v->v.x);
                    val[1] = FROM_LUA_NUM(v->v.y);
                    ptr += stride;
                    j++;
                } else {
                    break;
                }
            }
            break;
        }
        case 3: {
            while (ptr < end) {
                lua_rawgeti(L, idx, j);
                am_vec3 *v = am_get_userdata_or_nil(L, am_vec3, -1);
                lua_pop(L, 1);
                if (v != NULL) {
                    CTYPE *val = (CTYPE*)ptr;
                    val[0] = FROM_LUA_NUM(v->v.x);
                    val[1] = FROM_LUA_NUM(v->v.y);
                    val[2] = FROM_LUA_NUM(v->v.z);
                    ptr += stride;
                    j++;
                } else {
                    break;
                }
            }
            break;
        }
        case 4: {
            while (ptr < end) {
                lua_rawgeti(L, idx, j);
                am_vec4 *v = am_get_userdata_or_nil(L, am_vec4, -1);
                lua_pop(L, 1);
                if (v != NULL) {
                    CTYPE *val = (CTYPE*)ptr;
                    val[0] = FROM_LUA_NUM(v->v.x);
                    val[1] = FROM_LUA_NUM(v->v.y);
                    val[2] = FROM_LUA_NUM(v->v.z);
                    val[3] = FROM_LUA_NUM(v->v.w);
                    ptr += stride;
                    j++;
                } else {
                    break;
                }
            }
            break;
        }
        case 9: {
            while (ptr < end) {
                lua_rawgeti(L, idx, j);
                am_mat3 *m = am_get_userdata_or_nil(L, am_mat3, -1);
                lua_pop(L, 1);
                if (m != NULL) {
                    CTYPE *val = (CTYPE*)ptr;
                    val[0] = FROM_LUA_NUM(m->m[0][0]);
                    val[1] = FROM_LUA_NUM(m->m[0][1]);
                    val[2] = FROM_LUA_NUM(m->m[0][2]);
                    val[3] = FROM_LUA_NUM(m->m[1][0]);
                    val[4] = FROM_LUA_NUM(m->m[1][1]);
                    val[5] = FROM_LUA_NUM(m->m[1][2]);
                    val[6] = FROM_LUA_NUM(m->m[2][0]);
                    val[7] = FROM_LUA_NUM(m->m[2][1]);
                    val[8] = FROM_LUA_NUM(m->m[2][2]);
                    ptr += stride;
                    j++;
                } else {
                    break;
                }
            }
            break;
        }
        case 16: {
            while (ptr < end) {
                lua_rawgeti(L, idx, j);
                am_mat4 *m = am_get_userdata_or_nil(L, am_mat4, -1);
                lua_pop(L, 1);
                if (m != NULL) {
                    CTYPE *val = (CTYPE*)ptr;
                    val[0] =  FROM_LUA_NUM(m->m[0][0]);
                    val[1] =  FROM_LUA_NUM(m->m[0][1]);
                    val[2] =  FROM_LUA_NUM(m->m[0][2]);
                    val[3] =  FROM_LUA_NUM(m->m[0][3]);
                    val[4] =  FROM_LUA_NUM(m->m[1][0]);
                    val[5] =  FROM_LUA_NUM(m->m[1][1]);
                    val[6] =  FROM_LUA_NUM(m->m[1][2]);
                    val[7] =  FROM_LUA_NUM(m->m[1][3]);
                    val[8] =  FROM_LUA_NUM(m->m[2][0]);
                    val[9] =  FROM_LUA_NUM(m->m[2][1]);
                    val[10] = FROM_LUA_NUM(m->m[2][2]);
                    val[11] = FROM_LUA_NUM(m->m[2][3]);
                    val[12] = FROM_LUA_NUM(m->m[3][0]);
                    val[13] = FROM_LUA_NUM(m->m[3][1]);
                    val[14] = FROM_LUA_NUM(m->m[3][2]);
                    val[15] = FROM_LUA_NUM(m->m[3][3]);
                    ptr += stride;
                    j++;
                } else {
                    break;
                }
            }
            break;
        }
    }
    buf->mark_dirty(start_offset, start_offset + (j-2) * stride + sizeof(CTYPE) * view->components);
}

static void AM_CONCAT(TNAME,_view_set_from_single)(lua_State *L, am_buffer_view *view, int idx, int start, int sz) {
    int size = am_min(sz, view->size - start + 1);
    if (size == 0) return;
    int stride = view->stride;
    am_buffer *buf = view->buffer;
    int start_offset = view->offset + (start - 1) * stride;
    int end_offset = start_offset + stride * (size-1) + sizeof(CTYPE) * view->components;
    uint8_t *ptr = buf->data + start_offset;
    uint8_t *end = ptr + stride * size;
    switch (view->components) {
        case 1: {
            CTYPE val = FROM_LUA_NUM(luaL_checknumber(L, idx));
            while (ptr < end) {
                *((CTYPE*)ptr) = val;
                ptr += stride;
            }
            break;
        }
        case 2: {
            am_vec2 *v = am_get_userdata(L, am_vec2, idx);
            CTYPE val[2];
            val[0] = FROM_LUA_NUM(v->v.x);
            val[1] = FROM_LUA_NUM(v->v.y);
            while (ptr < end) {
                CTYPE *dst = (CTYPE*)ptr;
                dst[0] = val[0];
                dst[1] = val[1];
                ptr += stride;
            }
            break;
        }
        case 3: {
            am_vec3 *v = am_get_userdata(L, am_vec3, idx);
            CTYPE val[3];
            val[0] = FROM_LUA_NUM(v->v.x);
            val[1] = FROM_LUA_NUM(v->v.y);
            val[2] = FROM_LUA_NUM(v->v.z);
            while (ptr < end) {
                CTYPE *dst = (CTYPE*)ptr;
                dst[0] = val[0];
                dst[1] = val[1];
                dst[2] = val[2];
                ptr += stride;
            }
            break;
        }
        case 4: {
            am_vec4 *v = am_get_userdata(L, am_vec4, idx);
            CTYPE val[4];
            val[0] = FROM_LUA_NUM(v->v.x);
            val[1] = FROM_LUA_NUM(v->v.y);
            val[2] = FROM_LUA_NUM(v->v.z);
            val[3] = FROM_LUA_NUM(v->v.w);
            while (ptr < end) {
                CTYPE *dst = (CTYPE*)ptr;
                dst[0] = val[0];
                dst[1] = val[1];
                dst[2] = val[2];
                dst[3] = val[3];
                ptr += stride;
            }
            break;
        }
        case 9: {
            am_mat3 *m = am_get_userdata(L, am_mat3, idx);
            CTYPE val[9];
            val[0] = FROM_LUA_NUM(m->m[0][0]);
            val[1] = FROM_LUA_NUM(m->m[0][1]);
            val[2] = FROM_LUA_NUM(m->m[0][2]);
            val[3] = FROM_LUA_NUM(m->m[1][0]);
            val[4] = FROM_LUA_NUM(m->m[1][1]);
            val[5] = FROM_LUA_NUM(m->m[1][2]);
            val[6] = FROM_LUA_NUM(m->m[2][0]);
            val[7] = FROM_LUA_NUM(m->m[2][1]);
            val[8] = FROM_LUA_NUM(m->m[2][2]);
            while (ptr < end) {
                CTYPE *dst = (CTYPE*)ptr;
                dst[0] = val[0];
                dst[1] = val[1];
                dst[2] = val[2];
                dst[3] = val[3];
                dst[4] = val[4];
                dst[5] = val[5];
                dst[6] = val[6];
                dst[7] = val[7];
                dst[8] = val[8];
                ptr += stride;
            }
            break;
        }
        case 16: {
            am_mat4 *m = am_get_userdata(L, am_mat4, idx);
            CTYPE val[16];
            val[0] =  FROM_LUA_NUM(m->m[0][0]);
            val[1] =  FROM_LUA_NUM(m->m[0][1]);
            val[2] =  FROM_LUA_NUM(m->m[0][2]);
            val[3] =  FROM_LUA_NUM(m->m[0][3]);
            val[4] =  FROM_LUA_NUM(m->m[1][0]);
            val[5] =  FROM_LUA_NUM(m->m[1][1]);
            val[6] =  FROM_LUA_NUM(m->m[1][2]);
            val[7] =  FROM_LUA_NUM(m->m[1][3]);
            val[8] =  FROM_LUA_NUM(m->m[2][0]);
            val[9] =  FROM_LUA_NUM(m->m[2][1]);
            val[10] = FROM_LUA_NUM(m->m[2][2]);
            val[11] = FROM_LUA_NUM(m->m[2][3]);
            val[12] = FROM_LUA_NUM(m->m[3][0]);
            val[13] = FROM_LUA_NUM(m->m[3][1]);
            val[14] = FROM_LUA_NUM(m->m[3][2]);
            val[15] = FROM_LUA_NUM(m->m[3][3]);
            while (ptr < end) {
                CTYPE *dst = (CTYPE*)ptr;
                dst[0] = val[0];
                dst[1] = val[1];
                dst[2] = val[2];
                dst[3] = val[3];
                dst[4] = val[4];
                dst[5] = val[5];
                dst[6] = val[6];
                dst[7] = val[7];
                dst[8] = val[8];
                dst[9] = val[9];
                dst[10] = val[10];
                dst[11] = val[11];
                dst[12] = val[12];
                dst[13] = val[13];
                dst[14] = val[14];
                dst[15] = val[15];
                ptr += stride;
            }
            break;
        }
    }
    buf->mark_dirty(start_offset, end_offset);
}

static void AM_CONCAT(TNAME,_view_set_from_view)(lua_State *L, am_buffer_view *dest, am_buffer_view *src, int start, int sz) {
    unsigned int dest_size = (unsigned int)am_min(sz, dest->size - start + 1);
    unsigned int dest_stride = (unsigned int)dest->stride;
    am_buffer *dest_buf = dest->buffer;
    int dest_start_offset = dest->offset + (start - 1) * dest_stride;
    uint8_t *dest_ptr = dest_buf->data + dest_start_offset;
    unsigned int src_size = (unsigned int)src->size;
    unsigned int src_stride = (unsigned int)src->stride;
    uint8_t *src_ptr = src->buffer->data + src->offset;
    unsigned int size = am_min(src_size, dest_size);

    if (size == 0) return;

    if (dest->components != src->components) {
        luaL_error(L, "views have differing number of components (%d vs %d)", dest->components, src->components);
        return;
    }

    unsigned int components = (unsigned int)dest->components;
    unsigned int dest_elem_size = ((unsigned int)am_view_type_infos[dest->type].size) * components;
    bool is_dense = dest->type == src->type && dest_elem_size == (unsigned int)src->stride && dest_elem_size == (unsigned int)dest->stride;
    if (is_dense && src->buffer != dest->buffer /* memcpy needs memory areas to not overlap*/) {
        memcpy(dest_ptr, src_ptr, size * dest_elem_size);
    } else if (dest->type == src->type) {
        for (unsigned int i = 0; i < size; ++i) {
            for (unsigned int c = 0; c < components; ++c) {
                ((CTYPE*)dest_ptr)[c] = ((CTYPE*)src_ptr)[c];
            }
            dest_ptr += dest_stride;
            src_ptr += src_stride;
        }
    } else {
        lua_Number (*read_num)(uint8_t*) = am_view_type_infos[src->type].num_reader;
        if (read_num == NULL) {
            luaL_error(L, "cannot convert a %s view to a %s view",
                am_view_type_infos[src->type].name, am_view_type_infos[dest->type].name);
            return;
        }
        int src_comp_size = am_view_type_infos[src->type].size;
        for (unsigned int i = 0; i < size; ++i) {
            CTYPE *dst2 = (CTYPE*)dest_ptr;
            uint8_t *src2 = src_ptr;
            for (unsigned int c = 0; c < components; ++c) {
                dst2[c] = FROM_LUA_NUM(read_num(src2));
                src2 += src_comp_size;
            }
            dest_ptr += dest_stride;
            src_ptr += src_stride;
        }
    }

    dest_buf->mark_dirty(dest_start_offset, (dest_ptr - dest_buf->data) - dest_stride + sizeof(CTYPE) * components);
}

static int AM_CONCAT(TNAME,_view_set)(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_buffer_view *view = am_check_buffer_view(L, 1);
    if (view->size == 0) {
        // nothing to set
        lua_pushvalue(L, 1); // for chaining
        return 1;
    }
    int sz = INT_MAX;
    int start = 1;
    if (nargs > 2) {
        start = luaL_checkinteger(L, 3);
    }
    if (nargs > 3) {
        sz = luaL_checkinteger(L, 4);
        if (sz < 0) {
            return luaL_error(L, "size can't be negative");
        }
    }
    if (start < 1) {
        return luaL_error(L, "in view:set, start must be positive",
            view->size, start);
    }
    if (start > view->size) {
        // nothing to do
        return 0;
    }
    switch (am_get_type(L, 2)) {
        case LUA_TTABLE:
            AM_CONCAT(TNAME,_view_set_from_table)(L, view, 2, start, sz);
            break;
        case LUA_TNUMBER:
        case MT_am_vec2:
        case MT_am_vec3:
        case MT_am_vec4:
        case MT_am_mat3:
        case MT_am_mat4:
            AM_CONCAT(TNAME,_view_set_from_single)(L, view, 2, start, sz);
            break;
        case MT_am_buffer_view:
            AM_CONCAT(TNAME,_view_set_from_view)(L, view, am_check_buffer_view(L, 2), start, sz);
            break;
        default:
            return luaL_error(L, "expecting a number, table or view in position 2 (got %s)",
                am_get_typename(L, 2));
    }
    return 0;
}

static int AM_CONCAT(TNAME,_view_set_component)(lua_State *L, am_buffer_view *view, int start, int count) {
    int comp_sz = am_view_type_infos[view->type].size;
    int orig_components = view->components;
    int orig_offset = view->offset;
    view->offset = view->offset + start * comp_sz;
    view->components = count;
    lua_remove(L, 2); // field name
    int rt = AM_CONCAT(TNAME,_view_set)(L);
    view->offset = orig_offset;
    view->components = orig_components;
    return rt;
}

static int AM_CONCAT(TNAME,_view_set_component_field)(lua_State *L, am_buffer_view *view) {
    if (lua_type(L, 2) == LUA_TSTRING) {
        size_t len;
        const char *str = lua_tolstring(L, 2, &len);
        switch (len) {
            case 1:
                switch (str[0]) {
                    case 'x':
                    case 'r':
                    case 's':
                        return AM_CONCAT(TNAME,_view_set_component)(L, view, 0, 1);
                    case 'y':
                    case 'g':
                    case 't':
                        return AM_CONCAT(TNAME,_view_set_component)(L, view, 1, 1);
                    case 'z':
                    case 'b':
                    case 'p':
                        return AM_CONCAT(TNAME,_view_set_component)(L, view, 2, 1);
                    case 'w':
                    case 'a':
                    case 'q':
                        return AM_CONCAT(TNAME,_view_set_component)(L, view, 3, 1);
                }
                break;
            case 2: {
                int c1 = VEC_COMPONENT_OFFSET(str[0]);
                int c2 = VEC_COMPONENT_OFFSET(str[1]);
                if (c1 >= 0 && c2 == c1 + 1 && view->components > c2) {
                    return AM_CONCAT(TNAME,_view_set_component)(L, view, c1, 2);
                }
                break;
            }
            case 3: {
                int c1 = VEC_COMPONENT_OFFSET(str[0]);
                int c2 = VEC_COMPONENT_OFFSET(str[1]);
                int c3 = VEC_COMPONENT_OFFSET(str[2]);
                if (c1 >= 0 && c2 == c1 + 1 && c3 == c2 + 1 && view->components > c3) {
                    return AM_CONCAT(TNAME,_view_set_component)(L, view, c1, 3);
                }
                break;
            }
            case 4: {
                int c1 = VEC_COMPONENT_OFFSET(str[0]);
                int c2 = VEC_COMPONENT_OFFSET(str[1]);
                int c3 = VEC_COMPONENT_OFFSET(str[2]);
                int c4 = VEC_COMPONENT_OFFSET(str[3]);
                if (c1 >= 0 && c2 == c1 + 1 && c3 == c2 + 1 && c4 == c3 + 1 && view->components > c4) {
                    return AM_CONCAT(TNAME,_view_set_component)(L, view, c1, 4);
                }
                break;
            }
        }
    }
    return luaL_error(L, "invalid view field for updating");
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
#undef FROM_LUA_NUM
#undef READ_NUM
