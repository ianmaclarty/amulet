#ifndef SUFFIX
#error SUFFIX undefined
#endif

#ifndef OPNAME
#error OPNAME undefined
#endif

#ifndef ARGS
#error ARGS undefined
#endif

#if ARGS > 3
#error ARGS must be <= 3
#endif

#ifdef EXPORT
int AM_CONCAT(am_view_op_, SUFFIX)(lua_State *L) {
#else
static int AM_CONCAT(view_op_, SUFFIX)(lua_State *L) {
#endif
    am_check_nargs(L, ARGS);
    int type1;
    float *buf1;
    int stride1;
    int size1;
    int components1;
#if defined(COMPONENT_WISE)
    int component_move1 = 1;
#endif
    float farr1[4];
    view_float_iter_setup(L, 1, &type1, &buf1, &stride1, &size1, &components1, farr1, OPNAME);
#if ARGS > 1
    int type2;
    float *buf2;
    int stride2;
    int size2;
    int components2;
#if defined(COMPONENT_WISE)
    int component_move2 = 1;
#endif
    float farr2[4];
    view_float_iter_setup(L, 2, &type2, &buf2, &stride2, &size2, &components2, farr2, OPNAME);
#endif
#if ARGS > 2
    int type3;
    float *buf3;
    int stride3;
    int size3;
    int components3;
#if defined(COMPONENT_WISE)
    int component_move3 = 1;
#endif
    float farr3[4];
    view_float_iter_setup(L, 3, &type3, &buf3, &stride3, &size3, &components3, farr3, OPNAME);
#endif

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
#if defined(COMPONENT_WISE)
    int components = components1;
    #if ARGS > 1
        if (components != components2) {
            if (components1 == 1 || components2 == 1) {
                components = am_max(components1, components2);
                if (components1 == 1) component_move1 = 0;
                if (components2 == 1) component_move2 = 0;
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
                if (components1 == 1) component_move1 = 0;
                if (components2 == 1) component_move2 = 0;
                if (components3 == 1) component_move3 = 0;
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
        int j1 = 0;
        #if ARGS > 1
        int j2 = 0;
        #endif
        #if ARGS > 2
        int j3 = 0;
        #endif
        for (int j = 0; j < components; ++j) {
            #if ARGS == 1
                result_data[i + j] = OP(buf1[j1]);
                j1 += component_move1;
            #elif ARGS == 2
                result_data[i + j] = OP(buf1[j1], buf2[j2]);
                j1 += component_move1;
                j2 += component_move2;
            #elif ARGS == 3
                result_data[i + j] = OP(buf1[j1], buf2[j2], buf3[j3]);
                j1 += component_move1;
                j2 += component_move2;
                j3 += component_move3;
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
#elif defined(ELEMENT_WISE)
    int components = components1;
    #if ARGS > 1
        if (components != components2) {
            return luaL_error(L, "view components mismatch for op %s (%d vs %d)", OPNAME,
                components1, components2);
        }
    #endif
    #if ARGS > 2
        if (components != components3) {
            return luaL_error(L, "view components mismatch for op %s (%d vs %d vs %d)", OPNAME,
                components1, components2, components3);
        }
    #endif

    am_buffer *result_buffer = am_push_new_buffer_and_init(L, size * RESULT_COMPONENTS * 4);
    float *result_data = (float*)result_buffer->data;
    am_buffer_view_type result_type = (am_buffer_view_type)(AM_VIEW_TYPE_FLOAT + (RESULT_COMPONENTS - 1));

    switch (components) {
#if defined(OP)
        case 1:
            for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                #if ARGS == 1
                    OP(&result_data[i], buf1);
                #elif ARGS == 2
                    OP(&result_data[i], buf1, buf2);
                #elif ARGS == 3
                    OP(&result_data[i], buf1, buf2, buf3);
                #endif
                buf1 += stride1;
                #if ARGS > 1
                    buf2 += stride2;
                #endif
                #if ARGS > 2
                    buf3 += stride3;
                #endif
            }
            break;
#endif
#if defined(OP2)
        case 2:
            for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                #if ARGS == 1
                    //am_debug("%0.2f %0.2f", buf1[0], buf1[1]);
                    OP2(&result_data[i], buf1);
                #elif ARGS == 2
                    OP2(&result_data[i], buf1, buf2);
                #elif ARGS == 3
                    OP2(&result_data[i], buf1, buf2, buf3);
                #endif
                buf1 += stride1;
                #if ARGS > 1
                    buf2 += stride2;
                #endif
                #if ARGS > 2
                    buf3 += stride3;
                #endif
            }
            break;
#endif
#if defined(OP3)
        case 3:
            for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                #if ARGS == 1
                    OP3(&result_data[i], buf1);
                #elif ARGS == 2
                    OP3(&result_data[i], buf1, buf2);
                #elif ARGS == 3
                    OP3(&result_data[i], buf1, buf2, buf3);
                #endif
                buf1 += stride1;
                #if ARGS > 1
                    buf2 += stride2;
                #endif
                #if ARGS > 2
                    buf3 += stride3;
                #endif
            }
            break;
#endif
#if defined(OP4)
        case 4:
            for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                #if ARGS == 1
                    OP4(&result_data[i], buf1);
                #elif ARGS == 2
                    OP4(&result_data[i], buf1, buf2);
                #elif ARGS == 3
                    OP4(&result_data[i], buf1, buf2, buf3);
                #endif
                buf1 += stride1;
                #if ARGS > 1
                    buf2 += stride2;
                #endif
                #if ARGS > 2
                    buf3 += stride3;
                #endif
            }
            break;
#endif
        default:
            return luaL_error(L, "op %s does not support input with %d", OPNAME, components1);
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
    view->last_max_elem_version = 0;
    view->max_elem = 0;

    lua_remove(L, -2); // remove result_buffer

    return 1;
}

#undef ARGS
#undef COMPONENT_WISE
#undef ELEMENT_WISE
#undef OPNAME
#undef OP
#undef OP2
#undef OP3
#undef OP4
#undef SUFFIX
#undef EXPORT
#undef RESULT_COMPONENTS
