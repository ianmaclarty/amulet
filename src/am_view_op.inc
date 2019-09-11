#ifndef SUFFIX
#error SUFFIX undefined
#endif

#ifndef OPNAME
#error OPNAME undefined
#endif

#ifdef ARGS
#define MIN_ARGS  ARGS
#define MAX_ARGS  ARGS
#ifdef OP
    #ifndef OP2
    #define OP2 OP
    #endif
    #ifndef OP3
    #define OP3 OP
    #endif
    #ifndef OP4
    #define OP4 OP
    #endif
#endif
#endif 

#ifndef MIN_ARGS
#error MIN_ARGS undefined
#endif

#if MIN_ARGS < 1
#error MIN_ARGS must be at least 1
#endif

#ifndef MAX_ARGS
#error MAX_ARGS undefined
#endif

#if MAX_ARGS > 4
#error MAX_ARGS must be at most 4
#endif

#ifdef EXPORT
int AM_CONCAT(am_view_op_, SUFFIX)(lua_State *L) {
#else
static int AM_CONCAT(view_op_, SUFFIX)(lua_State *L) {
#endif
    int nargs = am_check_nargs(L, MIN_ARGS);
    if (nargs > MAX_ARGS) return luaL_error(L, "too many arguments to %s (expecting at most %d, got %d)", OPNAME, MAX_ARGS, nargs);
    int arg_type[MAX_ARGS];
    float *arg_data[MAX_ARGS];
    int arg_stride[MAX_ARGS];
    int arg_size[MAX_ARGS];
    int arg_components[MAX_ARGS];
    float farr[MAX_ARGS][4];
    for (int a = 0; a < nargs; ++a) {
        view_float_iter_setup(L, a+1, &arg_type[a], &arg_data[a], &arg_stride[a], &arg_size[a], &arg_components[a], &farr[a][0], OPNAME);
    }
    int size = -1;
    for (int a = 0; a < nargs; ++a) {
        if (arg_stride[a] > 0) {
            if (size == -1) {
                size = arg_size[a];
            } else if (size != arg_size[a]) {
                return luaL_error(L, "view size mismatch for op %s (%d vs %d)", OPNAME, size, arg_size[a]);
            }
        }
    }
    if (size == -1) return luaL_error(L, "no view args given for op %s", OPNAME);

#if defined(COMPONENT_WISE)
    int arg_component_move[MAX_ARGS];
    for (int a = 0; a < MAX_ARGS; a++) arg_component_move[a] = 1;
    int components = arg_components[0];
    for (int a = 1; a < nargs; ++a) {
        if (components != arg_components[a]) {
            if (components == 1) {
                components = arg_components[a];
            } else if (arg_components[a] != 1) {
                return luaL_error(L, "view components mismatch for op %s (%d vs %d)", OPNAME,
                    components, arg_components[a]);
            }
        }
    }
    if (components > 1) {
        for (int a = 0; a < nargs; ++a) {
            if (arg_components[a] == 1 && arg_stride[a] > 0) {
                arg_component_move[a] = 0;
            }
        }
    }

    am_buffer *buf = am_push_new_buffer_and_init(L, size * components * 4);
    float *data = (float*)buf->data;
    am_buffer_view_type type = AM_VIEW_TYPE_F32;

    switch (nargs) {
#if MIN_ARGS == 1
        case 1: {
            int mv1 = arg_component_move[0];
            int stride1 = arg_stride[0];
            float *data1 = arg_data[0];
            if (stride1 == components && mv1 == 1) {
                // fast path
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP(data1[i]);
                }
            } else {
                for (int i = 0; i < size * components; i += components) {
                    int j1 = 0;
                    for (int j = 0; j < components; ++j) {
                        data[i + j] = OP(data1[j1]);
                        j1 += mv1;
                    }
                    data1 += stride1;
                }
            }
            break;
        }
#endif
#if MIN_ARGS <= 2 && MAX_ARGS >= 2
        case 2: {
            int mv1 = arg_component_move[0];
            int mv2 = arg_component_move[1];
            int stride1 = arg_stride[0];
            int stride2 = arg_stride[1];
            float *data1 = arg_data[0];
            float *data2 = arg_data[1];
            if (stride1 == components && stride2 == components && mv1 == 1 && mv2 == 1) {
                // fast path 
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP2(data1[i], data2[i]);
                }
            } else if (stride1 == components && mv1 == 1 && stride2 == 0 && mv2 == 1) {
                // fast path
                int j = 0;
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP2(data1[i], data2[j]);
                    j++;
                    if (j >= components) j = 0;
                }
            } else if (stride2 == components && mv2 == 1 && stride1 == 0 && mv1 == 1) {
                // fast path
                int j = 0;
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP2(data1[j], data2[i]);
                    j++;
                    if (j >= components) j = 0;
                }
            } else {
                for (int i = 0; i < size * components; i += components) {
                    int j1 = 0;
                    int j2 = 0;
                    for (int j = 0; j < components; ++j) {
                        data[i + j] = OP2(data1[j1], data2[j2]);
                        j1 += mv1;
                        j2 += mv2;
                    }
                    data1 += stride1;
                    data2 += stride2;
                }
            }
            break;
        }
#endif
#if MIN_ARGS <= 3 && MAX_ARGS >= 3
        case 3: {
            int mv1 = arg_component_move[0];
            int mv2 = arg_component_move[1];
            int mv3 = arg_component_move[2];
            int stride1 = arg_stride[0];
            int stride2 = arg_stride[1];
            int stride3 = arg_stride[2];
            float *data1 = arg_data[0];
            float *data2 = arg_data[1];
            float *data3 = arg_data[2];
            if (stride1 == components && stride2 == components && stride3 == components && 
                mv1 == 1 && mv2 == 1 && mv3 == 1)
            {
                // fast path
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP3(data1[i], data2[i], data3[i]);
                }
            } else if (stride1 == components && mv1 == 1 && stride2 == 0 && mv2 == 1 && stride3 == 0 && mv3 == 1) {
                // fast path
                int j = 0;
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP3(data1[i], data2[j], data3[j]);
                    j++;
                    if (j >= components) j = 0;
                }
            } else if (stride1 == components && mv1 == 1 && stride2 == components && mv2 == 1 && stride3 == 0 && mv3 == 1) {
                // fast path
                int j = 0;
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP3(data1[i], data2[i], data3[j]);
                    j++;
                    if (j >= components) j = 0;
                }
            } else if (stride1 == 0 && mv1 == 1 && stride2 == components && mv2 == 1 && stride3 == components && mv3 == 1) {
                // fast path
                int j = 0;
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP3(data1[j], data2[i], data3[i]);
                    j++;
                    if (j >= components) j = 0;
                }
            } else if (stride1 == 0 && mv1 == 1 && stride2 == components && mv2 == 1 && stride3 == 0 && mv3 == 1) {
                // fast path
                int j = 0;
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP3(data1[j], data2[i], data3[j]);
                    j++;
                    if (j >= components) j = 0;
                }
            } else if (stride1 == 0 && mv1 == 1 && stride2 == 0 && mv2 == 1 && stride3 == components && mv3 == 1) {
                // fast path
                int j = 0;
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP3(data1[j], data2[j], data3[i]);
                    j++;
                    if (j >= components) j = 0;
                }
            } else if (stride1 == components && mv1 == 1 && stride2 == 0 && mv2 == 1 && stride3 == components && mv3 == 1) {
                // fast path
                int j = 0;
                for (int i = 0; i < size * components; ++i) {
                    data[i] = OP3(data1[i], data2[j], data3[i]);
                    j++;
                    if (j >= components) j = 0;
                }
            } else {
                for (int i = 0; i < size * components; i += components) {
                    int j1 = 0;
                    int j2 = 0;
                    int j3 = 0;
                    for (int j = 0; j < components; ++j) {
                        data[i + j] = OP3(data1[j1], data2[j2], data3[j3]);
                        j1 += mv1;
                        j2 += mv2;
                        j3 += mv3;
                    }
                    data1 += stride1;
                    data2 += stride2;
                    data3 += stride3;
                }
            }
            break;
        }
#endif
#if MAX_ARGS == 4
        case 4: {
            int mv1 = arg_component_move[0];
            int mv2 = arg_component_move[1];
            int mv3 = arg_component_move[2];
            int mv4 = arg_component_move[3];
            int stride1 = arg_stride[0];
            int stride2 = arg_stride[1];
            int stride3 = arg_stride[2];
            int stride4 = arg_stride[3];
            float *data1 = arg_data[0];
            float *data2 = arg_data[1];
            float *data3 = arg_data[2];
            float *data4 = arg_data[3];
            for (int i = 0; i < size * components; i += components) {
                int j1 = 0;
                int j2 = 0;
                int j3 = 0;
                int j4 = 0;
                for (int j = 0; j < components; ++j) {
                    data[i + j] = OP4(data2[j1], data2[j2], data3[j3], data4[j4]);
                    j1 += mv1;
                    j2 += mv2;
                    j3 += mv3;
                    j4 += mv4;
                }
                data1 += stride1;
                data2 += stride2;
                data3 += stride3;
                data4 += stride4;
            }
            break;
        }
#endif
    }
    int result_comps = components;
#elif defined(ELEMENT_WISE)
    int components = arg_components[0];
    for (int a = 1; a < nargs; ++a) {
        if (components != arg_components[a]) {
            return luaL_error(L, "view components mismatch for op %s (%d vs %d)", OPNAME,
                components, arg_components[a]);
        }
    }

    am_buffer *buf = am_push_new_buffer_and_init(L, size * RESULT_COMPONENTS * 4);
    float *data = (float*)buf->data;
    am_buffer_view_type type = AM_VIEW_TYPE_F32;

    switch (nargs) {
#if MIN_ARGS == 1
        case 1: {
            int stride1 = arg_stride[0];
            float *data1 = arg_data[0];
            switch (components) {
            #if defined(OP1_1)
                case 1:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP1_1(&data[i], data1);
                        data1 += stride1;
                    }
                    break;
            #endif
            #if defined(OP1_2)
                case 2:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP1_2(&data[i], data1);
                        data1 += stride1;
                    }
                    break;
            #endif
            #if defined(OP1_3)
                case 3:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP1_3(&data[i], data1);
                        data1 += stride1;
                    }
                    break;
            #endif
            #if defined(OP1_4)
                case 4:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP1_4(&data[i], data1);
                        data1 += stride1;
                    }
                    break;
            #endif
                default:
                    return luaL_error(L, "op %s does not support input with %d components", OPNAME, components);
            }
            break;
        }
#endif
#if MIN_ARGS <= 2 && MAX_ARGS >= 2
        case 2: {
            int stride1 = arg_stride[0];
            int stride2 = arg_stride[1];
            float *data1 = arg_data[0];
            float *data2 = arg_data[1];
            switch (components) {
            #if defined(OP2_1)
                case 1:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP2_1(&data[i], data1, data2);
                        data1 += stride1;
                        data2 += stride2;
                    }
                    break;
            #endif
            #if defined(OP2_2)
                case 2:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP2_2(&data[i], data1, data2);
                        data1 += stride1;
                        data2 += stride2;
                    }
                    break;
            #endif
            #if defined(OP2_3)
                case 3:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP2_3(&data[i], data1, data2);
                        data1 += stride1;
                        data2 += stride2;
                    }
                    break;
            #endif
            #if defined(OP2_4)
                case 4:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP2_4(&data[i], data1, data2);
                        data1 += stride1;
                        data2 += stride2;
                    }
                    break;
            #endif
                default:
                    return luaL_error(L, "op %s does not support input with %d components", OPNAME, components);
            }
            break;
        }
#endif
#if MIN_ARGS <= 3 && MAX_ARGS >= 3
        case 3: {
            int stride1 = arg_stride[0];
            int stride2 = arg_stride[1];
            int stride3 = arg_stride[2];
            float *data1 = arg_data[0];
            float *data2 = arg_data[1];
            float *data3 = arg_data[2];
            switch (components) {
            #if defined(OP3_1)
                case 1:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP3_1(&data[i], data1, data2, data3);
                        data1 += stride1;
                        data2 += stride2;
                        data3 += stride3;
                    }
                    break;
            #endif
            #if defined(OP3_2)
                case 2:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP3_2(&data[i], data1, data2, data3);
                        data1 += stride1;
                        data2 += stride2;
                        data3 += stride3;
                    }
                    break;
            #endif
            #if defined(OP3_3)
                case 3:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP3_3(&data[i], data1, data2, data3);
                        data1 += stride1;
                        data2 += stride2;
                        data3 += stride3;
                    }
                    break;
            #endif
            #if defined(OP3_4)
                case 4:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP3_4(&data[i], data1, data2, data3);
                        data1 += stride1;
                        data2 += stride2;
                        data3 += stride3;
                    }
                    break;
            #endif
                default:
                    return luaL_error(L, "op %s does not support input with %d components", OPNAME, components);
            }
            break;
        }
#endif
#if MAX_ARGS == 4
        case 4: {
            int stride1 = arg_stride[0];
            int stride2 = arg_stride[1];
            int stride3 = arg_stride[2];
            int stride4 = arg_stride[3];
            float *data1 = arg_data[0];
            float *data2 = arg_data[1];
            float *data3 = arg_data[2];
            float *data4 = arg_data[3];
            switch (components) {
            #if defined(OP4_1)
                case 1:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP4_1(&data[i], data1, data2, data3, data4);
                        data1 += stride1;
                        data2 += stride2;
                        data3 += stride3;
                        data4 += stride4;
                    }
                    break;
            #endif
            #if defined(OP4_2)
                case 2:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP4_2(&data[i], data1, data2, data3, data4);
                        data1 += stride1;
                        data2 += stride2;
                        data3 += stride3;
                        data4 += stride4;
                    }
                    break;
            #endif
            #if defined(OP4_3)
                case 3:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP4_3(&data[i], data1, data2, data3, data4);
                        data1 += stride1;
                        data2 += stride2;
                        data3 += stride3;
                        data4 += stride4;
                    }
                    break;
            #endif
            #if defined(OP4_4)
                case 4:
                    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
                        OP4_4(&data[i], data1, data2, data3, data4);
                        data1 += stride1;
                        data2 += stride2;
                        data3 += stride3;
                        data4 += stride4;
                    }
                    break;
            #endif
                default:
                    return luaL_error(L, "op %s does not support input with %d components", OPNAME, components);
            }
            break;
        }
#endif
    }
    int result_comps = RESULT_COMPONENTS;
#elif defined(VEC_BUILDER)
    int components = 0;
    for (int a = 0; a < nargs; a++) {
        components += arg_components[a];
    }
    if (components < RESULT_COMPONENTS) {
        return luaL_error(L, "insufficient components for op %s", OPNAME);
    } else if (components > RESULT_COMPONENTS) {
        return luaL_error(L, "too many components for op %s", OPNAME);
    }
    float f[RESULT_COMPONENTS];
    am_buffer *buf = am_push_new_buffer_and_init(L, size * RESULT_COMPONENTS * 4);
    float *data = (float*)buf->data;
    am_buffer_view_type type = AM_VIEW_TYPE_F32;

    for (int i = 0; i < size * RESULT_COMPONENTS; i += RESULT_COMPONENTS) {
        int c = 0;
        for (int a = 0; a < nargs; ++a) {
            for (int j = 0; j < arg_components[a]; ++j) {
                f[c++] = arg_data[a][j];
            }
        }
        #if RESULT_COMPONENTS == 2
            OP(&data[i], f[0], f[1]);
        #elif RESULT_COMPONENTS == 3
            OP(&data[i], f[0], f[1], f[2]);
        #elif RESULT_COMPONENTS == 4
            OP(&data[i], f[0], f[1], f[2], f[3]);
        #endif
        for (int a = 0; a < nargs; ++a) {
            arg_data[a] += arg_stride[a];
        }
    }
    int result_comps = RESULT_COMPONENTS;
#else
    #error no op style given
#endif

    am_buffer_view *view = am_new_buffer_view(L, type, components);

    view->buffer = buf;
    view->buffer_ref = view->ref(L, -2);
    view->offset = 0;
    view->stride = result_comps * 4;
    view->size = size;
    view->last_max_elem_version = 0;
    view->max_elem = 0;

    lua_remove(L, -2); // remove buf

    return 1;
}

#undef ARGS
#undef MIN_ARGS
#undef MAX_ARGS
#undef COMPONENT_WISE
#undef ELEMENT_WISE
#undef OPNAME
#undef OP
#undef OP2
#undef OP3
#undef OP4
#undef OP1_1
#undef OP1_2
#undef OP1_3
#undef OP1_4
#undef OP2_1
#undef OP2_2
#undef OP2_3
#undef OP2_4
#undef OP3_1
#undef OP3_2
#undef OP3_3
#undef OP3_4
#undef OP4_1
#undef OP4_2
#undef OP4_3
#undef OP4_4
#undef SUFFIX
#undef EXPORT
#undef RESULT_COMPONENTS
