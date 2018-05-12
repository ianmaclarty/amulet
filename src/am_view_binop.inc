#ifndef OPNAME
#error OPNAME undefined
#endif

#ifndef OP
#error OP undefined
#endif

static int AM_CONCAT(view_, OPNAME)(lua_State *L) {
    am_check_nargs(L, 2);
    int type1 = am_get_type(L, 1);
    int type2 = am_get_type(L, 2);
    float *buf1;
    float *buf2;
    int stride1;
    int stride2;
    int size1;
    int size2;
    int components1;
    int components2;
    float farr[4];

    switch (type1) {
        case MT_am_buffer_view:  {
            am_buffer_view *view1 = check_buffer_view(L, 1);
            if (view1->offset & 3 || view1->stride & 3) {
                return luaL_error(L, "view must be 4 byte aligned");
            }
            buf1 = (float*)(view1->buffer->data + view1->offset);
            stride1 = view1->stride / 4;
            size1 = view1->size;
            switch (view1->type) {
                case AM_VIEW_TYPE_FLOAT:
                    components1 = 1;
                    break;
                case AM_VIEW_TYPE_FLOAT2:
                    components1 = 2;
                    break;
                case AM_VIEW_TYPE_FLOAT3:
                    components1 = 3;
                    break;
                case AM_VIEW_TYPE_FLOAT4:
                    components1 = 4;
                    break;
                default:
                    return luaL_error(L, "view type %s not supported for op %s", view_type_name[view1->type], AM_STR(OPNAME));
            }
            break;
        }
        case LUA_TNUMBER: {
            lua_Number n = lua_tonumber(L, 1);
            farr[0] = (float)n;
            buf1 = farr;
            stride1 = 0;
            size1 = INT32_MAX;
            components1 = 1;
            break;
        }
        case MT_am_vec2: {
            am_vec2 *v = am_get_userdata(L, am_vec2, 1);
            farr[0] = v->v.x;
            farr[1] = v->v.y;
            buf1 = farr;
            stride1 = 0;
            size1 = INT32_MAX;
            components1 = 2;
            break;
        }
        case MT_am_vec3: {
            am_vec3 *v = am_get_userdata(L, am_vec3, 1);
            farr[0] = v->v.x;
            farr[1] = v->v.y;
            farr[2] = v->v.z;
            buf1 = farr;
            stride1 = 0;
            size1 = INT32_MAX;
            components1 = 3;
            break;
        }
        case MT_am_vec4: {
            am_vec4 *v = am_get_userdata(L, am_vec4, 1);
            farr[0] = v->v.x;
            farr[1] = v->v.y;
            farr[2] = v->v.z;
            farr[3] = v->v.w;
            buf1 = farr;
            stride1 = 0;
            size1 = INT32_MAX;
            components1 = 4;
            break;
        }
        default:
            return luaL_error(L, "type %s not supported for op %s", am_get_typename(L, type1), AM_STR(OPNAME));
    }

    switch (type2) {
        case MT_am_buffer_view:  {
            am_buffer_view *view2 = check_buffer_view(L, 2);
            if (view2->offset & 3 || view2->stride & 3) {
                return luaL_error(L, "view must be 4 byte aligned");
            }
            buf2 = (float*)(view2->buffer->data + view2->offset);
            stride2 = view2->stride / 4;
            size2 = view2->size;
            switch (view2->type) {
                case AM_VIEW_TYPE_FLOAT:
                    components2 = 1;
                    break;
                case AM_VIEW_TYPE_FLOAT2:
                    components2 = 2;
                    break;
                case AM_VIEW_TYPE_FLOAT3:
                    components2 = 3;
                    break;
                case AM_VIEW_TYPE_FLOAT4:
                    components2 = 4;
                    break;
                default:
                    return luaL_error(L, "view type %s not supported for op %s", view_type_name[view2->type], AM_STR(OPNAME));
            }
            break;
        }
        case LUA_TNUMBER: {
            lua_Number n = lua_tonumber(L, 2);
            farr[0] = (float)n;
            buf2 = farr;
            stride2 = 0;
            size2 = INT32_MAX;
            components2 = 1;
            break;
        }
        case MT_am_vec2: {
            am_vec2 *v = am_get_userdata(L, am_vec2, 2);
            farr[0] = v->v.x;
            farr[1] = v->v.y;
            buf2 = farr;
            stride2 = 0;
            size2 = INT32_MAX;
            components2 = 2;
            break;
        }
        case MT_am_vec3: {
            am_vec3 *v = am_get_userdata(L, am_vec3, 2);
            farr[0] = v->v.x;
            farr[1] = v->v.y;
            farr[2] = v->v.z;
            buf2 = farr;
            stride2 = 0;
            size2 = INT32_MAX;
            components2 = 3;
            break;
        }
        case MT_am_vec4: {
            am_vec4 *v = am_get_userdata(L, am_vec4, 2);
            farr[0] = v->v.x;
            farr[1] = v->v.y;
            farr[2] = v->v.z;
            farr[3] = v->v.w;
            buf2 = farr;
            stride2 = 0;
            size2 = INT32_MAX;
            components2 = 4;
            break;
        }
        default:
            return luaL_error(L, " type %s not supported for op %s", am_get_typename(L, type2), AM_STR(OPNAME));
    }

    if (components1 != components2) {
        return luaL_error(L, "inconsistent number of components for view op %s (%d vs %d)", AM_STR(OPNAME), components1, components2);
    }

    int size = am_min(size1, size2);

    am_buffer *result_buffer = am_new_userdata(L, am_buffer, size * 4 * components1);
    float *buf3 = (float*)result_buffer->data;
    am_buffer_view_type type;

    switch (components1) {
        case 1:
            type = AM_VIEW_TYPE_FLOAT;
            for (int i = 0; i < size; ++i) {
                buf3[i] = OP(*buf1, *buf2);
                buf1 += stride1;
                buf2 += stride2;
            }
            break;
        case 2:
            type = AM_VIEW_TYPE_FLOAT2;
            for (int i = 0; i < size * 2; i += 2) {
                buf3[i] = OP(*buf1, *buf2);
                buf3[i+1] = OP(*(buf1+1), *(buf2+1));
                buf1 += stride1;
                buf2 += stride2;
            }
            break;
        case 3:
            type = AM_VIEW_TYPE_FLOAT3;
            for (int i = 0; i < size * 3; i += 3) {
                buf3[i] = OP(*buf1, *buf2);
                buf3[i+1] = OP(*(buf1+1), *(buf2+1));
                buf3[i+2] = OP(*(buf1+2), *(buf2+2));
                buf1 += stride1;
                buf2 += stride2;
            }
            break;
        case 4:
            type = AM_VIEW_TYPE_FLOAT4;
            for (int i = 0; i < size * 4; i += 4) {
                buf3[i] = OP(*buf1, *buf2);
                buf3[i+1] = OP(*(buf1+1), *(buf2+1));
                buf3[i+2] = OP(*(buf1+2), *(buf2+2));
                buf3[i+3] = OP(*(buf1+3), *(buf2+3));
                buf1 += stride1;
                buf2 += stride2;
            }
            break;
        default:
            return luaL_error(L, "invalid component size %d", components1);
    }

    
    am_buffer_view *view = am_new_buffer_view(L, type);

    view->buffer = result_buffer;
    view->buffer_ref = view->ref(L, -2);
    view->offset = 0;
    view->stride = components1 * 4;
    view->size = size;
    view->type = type;
    view->type_size = components1 * 4;
    view->normalized = false;
    view->last_max_elem_version = 0;
    view->max_elem = 0;

    lua_remove(L, -2); // remove result_buffer

    return 1;
}

#undef OPNAME
#undef OP
