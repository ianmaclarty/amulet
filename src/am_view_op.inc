#ifndef SUFFIX
#error SUFFIX undefined
#endif

#ifndef OPNAME
#error OPNAME undefined
#endif

#ifndef OP
#error OP undefined
#endif

#ifndef ARGS
#error ARGS undefined
#endif

#if ARGS > 3
#error ARGS must be <= 3
#endif

static int AM_CONCAT(view_op_, SUFFIX)(lua_State *L) {
    int type1;
    float *buf1;
    int stride1;
    int size1;
    int components1;
    float farr1[4];
    view_float_iter_setup(L, 1, &type1, &buf1, &stride1, &size1, &components1, farr1, OPNAME);
#if ARGS > 1
    int type2;
    float *buf2;
    int stride2;
    int size2;
    int components2;
    float farr2[4];
    view_float_iter_setup(L, 2, &type2, &buf2, &stride2, &size2, &components2, farr2, OPNAME);
#endif
#if ARGS > 2
    int type3;
    float *buf3;
    int stride3;
    int size3;
    int components3;
    float farr3[4];
    view_float_iter_setup(L, 3, &type3, &buf3, &stride3, &size3, &components3, farr3, OPNAME);
#endif

#ifdef COMPONENT_WISE
    int size = size1;
    #if ARGS > 1
        if (size != size2) {
            if ((size1 == 1 && stride1 == 0) || (size2 == 1 && stride2 == 0)) {
                size = am_max(size1, size2);
            } else {
                return luaL_error(L, "view size mismatch for op %s (%d vs %d)", OPNAME, size1, size2);
            }
        }
    #endif
    #if ARGS > 2
        if (size != size3) {
            if ((size1 == 1 && stride1 == 0) || (size2 == 1 && stride2 == 0) || (size3 == 1 && stride3 == 0)) {
                size = am_max(size1, am_max(size2, size3));
            } else {
                return luaL_error(L, "view size mismatch for op %s (%d vs %d vs %d)", OPNAME, size1, size2, size3);
            }
        }
    #endif
    int components = components1;
    #if ARGS > 1
        if (components != components2) {
            if (components1 == 1 || components2 == 1) {
                components = am_max(components1, components2);
            } else {
                return luaL_error(L, "view components mismatch for op %s (%d vs %d)", OPNAME,
                    components1, components2);
            }
        }
    #endif
    #if ARGS > 2
        if (components != components3) {
            if (components1 == 1 || components2 == 1 || components3 == 1) {
                components = am_max(components1, am_max(components2, components3));
            } else {
                return luaL_error(L, "view components mismatch for op %s (%d vs %d vs %d)", OPNAME,
                    components1, components2, components3);
            }
        }
    #endif

    am_buffer *result_buffer = am_push_new_buffer_and_init(L, size * components * 4);
    float *result_data = (float*)result_buffer->data;
    am_buffer_view_type result_type = (am_buffer_view_type)(AM_VIEW_TYPE_FLOAT + (components - 1));

    for (int i = 0; i < size * components; i += components) {
        for (int j = 0; j < components; ++j) {
            #if ARGS == 1
                result_data[i + j] = OP(buf1[j]);
            #elif ARGS == 2
                result_data[i + j] = OP(buf1[j], buf2[j]);
            #elif ARGS == 3
                result_data[i + j] = OP(buf1[j], buf2[j], buf3[j]);
            #endif
        }
        buf1 += stride1;
        #if ARGS > 1
            buf2 += stride2;
        #endif
        #if ARGS > 2
            buf3 += stride3;
        #endif
    }
#else
    #error no op style given
#endif

    am_buffer_view *view = am_new_buffer_view(L, result_type);

    view->buffer = result_buffer;
    view->buffer_ref = view->ref(L, -2);
    view->offset = 0;
    view->stride = components * 4;
    view->size = size;
    view->type = result_type;
    view->type_size = components * 4;
    view->normalized = false;
    view->last_max_elem_version = 0;
    view->max_elem = 0;

    lua_remove(L, -2); // remove result_buffer

    return 1;
}

#undef ARGS
#undef COMPONENT_WISE
#undef OPNAME
#undef OP
#undef SUFFIX
