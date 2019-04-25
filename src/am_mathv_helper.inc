#define ADD_OP(x, y) ((x) + (y))
#define SUB_OP(x, y) ((x) - (y))
#define MUL_OP(x, y) ((x) * (y))
#define DIV_OP(x, y) ((x) / (y))
#define UNM_OP(x) (-(x))
#define IMOD_OP(x, y) ((x) % (y))
#define F32MOD_OP(a, b) ((a) - floorf((a) / (b)) * (b))
#define F64MOD_OP(a, b) ((a) - floor((a) / (b)) * (b))

#define MAT_MUL(x, y) ((x) * (y))

#define PERLIN1_F32(x) (glm::perlin(glm::vec2(x, 0.0f)))
#define PERLIN2_F32(x, y) (glm::perlin(glm::vec2(x, 0.0f), glm::vec2(y, 1.0f)))
#define SIMPLEX1_F32(x) (glm::simplex(glm::vec2(x, 0.0f)))

#define PERLIN1_F64(x) (glm::perlin(glm::dvec2(x, 0.0)))
#define PERLIN2_F64(x, y) (glm::perlin(glm::dvec2(x, 0.0), glm::dvec2(y, 1.0)))
#define SIMPLEX1_F64(x) ((double)glm::simplex(glm::vec2((float)x, 0.0f)))
#define SIMPLEX2_F64(x) ((double)glm::simplex(glm::vec2(x)))
#define SIMPLEX3_F64(x) ((double)glm::simplex(glm::vec3(x)))
#define SIMPLEX4_F64(x) ((double)glm::simplex(glm::vec4(x)))

static bool read_arg(lua_State *L, int arg, int *arg_type, am_buffer_view_type *view_type, uint8_t **buf, 
    int *stride, int *count, int *components, double *singleton_vals)
{
    *arg_type = am_get_type(L, arg);

    switch (*arg_type) {
        case MT_am_buffer_view:  {
            am_buffer_view *view = am_check_buffer_view(L, arg);
            *view_type = view->type;
            *buf = view->buffer->data + view->offset;
            *stride = view->stride;
            *count = view->size;
            *components = view->components;
            return true;
        }
        case LUA_TNUMBER: {
            singleton_vals[0] = lua_tonumber(L, arg);
            *stride = 0;
            *count = 1;
            *components = 1;
            return true;
        }
        case MT_am_vec2: {
            am_vec2 *v = am_get_userdata(L, am_vec2, arg);
            singleton_vals[0] = v->v.x;
            singleton_vals[1] = v->v.y;
            *stride = 0;
            *count = 1;
            *components = 2;
            return true;
        }
        case MT_am_vec3: {
            am_vec3 *v = am_get_userdata(L, am_vec3, arg);
            singleton_vals[0] = v->v.x;
            singleton_vals[1] = v->v.y;
            singleton_vals[2] = v->v.z;
            *stride = 0;
            *count = 1;
            *components = 3;
            return true;
        }
        case MT_am_vec4: {
            am_vec4 *v = am_get_userdata(L, am_vec4, arg);
            singleton_vals[0] = v->v.x;
            singleton_vals[1] = v->v.y;
            singleton_vals[2] = v->v.z;
            singleton_vals[3] = v->v.w;
            *stride = 0;
            *count = 1;
            *components = 4;
            return true;
        }
        case MT_am_mat3: {
            am_mat3 *m = am_get_userdata(L, am_mat3, arg);
            singleton_vals[0] = m->m[0][0];
            singleton_vals[1] = m->m[0][1];
            singleton_vals[2] = m->m[0][2];
            singleton_vals[3] = m->m[1][0];
            singleton_vals[4] = m->m[1][1];
            singleton_vals[5] = m->m[1][2];
            singleton_vals[6] = m->m[2][0];
            singleton_vals[7] = m->m[2][1];
            singleton_vals[8] = m->m[2][2];
            *stride = 0;
            *count = 1;
            *components = 9;
            return true;
        }
        case MT_am_mat4: {
            am_mat4 *m = am_get_userdata(L, am_mat4, arg);
            singleton_vals[0] = m->m[0][0];
            singleton_vals[1] = m->m[0][1];
            singleton_vals[2] = m->m[0][2];
            singleton_vals[3] = m->m[0][3];
            singleton_vals[4] = m->m[1][0];
            singleton_vals[5] = m->m[1][1];
            singleton_vals[6] = m->m[1][2];
            singleton_vals[7] = m->m[1][3];
            singleton_vals[8] = m->m[2][0];
            singleton_vals[9] = m->m[2][1];
            singleton_vals[10] = m->m[2][2];
            singleton_vals[11] = m->m[2][3];
            singleton_vals[12] = m->m[3][0];
            singleton_vals[13] = m->m[3][1];
            singleton_vals[14] = m->m[3][2];
            singleton_vals[15] = m->m[3][3];
            *stride = 0;
            *count = 1;
            *components = 16;
            return true;
        }
    }

    return false;
}

static void component_wise_setup(lua_State *L, am_buffer_view *target, const char *name, int nargs, 
    int *arg_type, am_buffer_view_type *arg_view_type, uint8_t **arg_data, 
    int *arg_stride, int *arg_count, int *arg_components, uint8_t **arg_singleton_bufs,
    am_buffer_view_type *output_view_type, int *output_count, int *output_components, int *output_stride, uint8_t **output_data,
    bool *is_dense)
{
    for (int i = 0; i < nargs; i++) {
        if (!read_arg(L, i+1, &arg_type[i], &arg_view_type[i], &arg_data[i], &arg_stride[i], &arg_count[i], &arg_components[i], (double*)arg_singleton_bufs[i])) {
            luaL_error(L, "invalid argument types for function %s", name);
            return;
        }
    }

    // code below depends on there being at least one arg
    if (nargs == 0) {
        luaL_error(L, "no arguments given for %s", name);
        return;
    }

    // compute output count and view type
    *output_count = -1;
    *output_view_type = AM_NUM_VIEW_TYPES;
    for (int i = 0; i < nargs; i++) {
        if (arg_type[i] == MT_am_buffer_view) {
            if (*output_count != -1 && arg_count[i] != *output_count) {
                luaL_error(L, "in call to %s argument %d has size %d, but previous arguments have size %d", name, (i+1), arg_count[i], *output_count);
                return;
            } else {
                *output_count = arg_count[i];
            }
            if (*output_view_type != AM_NUM_VIEW_TYPES && arg_view_type[i] != *output_view_type) {
                luaL_error(L, "in call to %s argument %d has base type %s, but previous arguments have base type %s", 
                    name, (i+1), am_view_type_infos[arg_view_type[i]].name, am_view_type_infos[arg_view_type[*output_view_type]].name);
                return;
            } else {
                *output_view_type = arg_view_type[i];
            }
        }
    }
    if (*output_count == -1) {
        luaL_error(L, "in call to %s at least one argument must be a view", name);
        return;
    }
    am_view_type_info output_view_type_info = am_view_type_infos[*output_view_type];

    // compute output components
    *output_components = arg_components[0];
    for (int i = 1; i < nargs; i++) {
        if (*output_components != arg_components[i]) {
            if (*output_components == 1) {
                *output_components = arg_components[i];
            } else if (arg_components[i] != 1) {
                luaL_error(L, "in call to %s argument %d has %d components, but previous arguments have %d components",
                    name, i+1, arg_components[i], *output_components);
                return;
            }
        }
    }
    // setup non-view args
    for (int i = 0; i < nargs; i++) {
        if (arg_type[i] != MT_am_buffer_view) {
            arg_view_type[i] = *output_view_type;
            double *f64s = (double*)arg_singleton_bufs[i];
            arg_data[i] = arg_singleton_bufs[i];
            switch (*output_view_type) {
                case AM_VIEW_TYPE_F32: {
                    float *conv = (float*)f64s;
                    for (int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (float)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_F64:
                    // nothing to do
                    break;
                case AM_VIEW_TYPE_U8: {
                    uint8_t *conv = (uint8_t*)f64s;
                    for (int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (uint8_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_I8: {
                    int8_t *conv = (int8_t*)f64s;
                    for (int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (int8_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_U16: {
                    uint16_t *conv = (uint16_t*)f64s;
                    for (int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (uint16_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_I16: {
                    int16_t *conv = (int16_t*)f64s;
                    for (int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (int16_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_U32: {
                    uint32_t *conv = (uint32_t*)f64s;
                    for (int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (uint32_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_I32: {
                    int32_t *conv = (int32_t*)f64s;
                    for (int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (int32_t)f64s[c];
                    }
                    break;
                }
                default:
                    luaL_error(L, "%s does not support arguments with base type %s", name, output_view_type_info.name);
                    return;
            }
        }
    }

    if (target == NULL) {
        // create a new buffer for the output
        *output_stride = *output_components * output_view_type_info.size;
        am_buffer *output_buffer = am_push_new_buffer_and_init(L, (*output_count) * (*output_stride));
        *output_data = output_buffer->data;
        am_buffer_view *output_view = am_new_buffer_view(L, *output_view_type, *output_components);
        output_view->buffer = output_buffer;
        output_view->buffer_ref = output_view->ref(L, -2);
        output_view->offset = 0;
        output_view->stride = *output_stride;
        output_view->size = *output_count;
        output_view->last_max_elem_version = 0;
        output_view->max_elem = 0;
        lua_remove(L, -2); // remove output_buffer
    } else {
        // overwrite provided buffer
        if (target->type != *output_view_type) {
            luaL_error(L, "target view has incorrect type (expecting %s, got %s)",
                output_view_type_info.name, am_view_type_infos[target->type].name);
            return;
        }
        if (target->components != *output_components) {
            luaL_error(L, "target view has incorrect number of components (expecting %d, got %d)",
                *output_components, target->components);
            return;
        }
        *output_count = am_min(*output_count, target->size);
        target->mark_dirty(0, *output_count);
        *output_stride = target->stride;
        *output_data = target->buffer->data + target->offset;
    }

    // is_dense is true if all the arguments and output are tightly packed.
    // This allows us to treat them as simple arrays of 1-component values,
    // making for a more efficient update loop.
    if (*output_stride == *output_components * output_view_type_info.size) {
        *is_dense = true;
        for (int i = 0; i < nargs; i++) {
            if (arg_stride[i] != *output_stride) {
                *is_dense = false;
                break;
            }
        }
    } else {
        *is_dense = false;
    }
}

int am_mathv_range(lua_State *L) {
    am_check_nargs(L, 4);
    am_buffer_view_type type = am_get_enum(L, am_buffer_view_type, 1);
    am_view_type_info info = am_view_type_infos[type];
    int n = luaL_checkinteger(L, 2);
    if (n < 2) {
        return luaL_error(L, "range count must be at least 2");
    }
    double start = luaL_checknumber(L, 3);
    double end = luaL_checknumber(L, 4);
    double inc = (end - start) / (double)(n-1);

    am_buffer *res_buf = am_push_new_buffer_and_init(L, info.size * n);
    switch (type) {
        case AM_VIEW_TYPE_F32: {
            float *data = (float*)res_buf->data;
            float x = (float)start;
            float dx = (float)inc;
            for (int i = 0; i < n - 1; ++i) {
                data[i] = x;
                x += dx;
            }
            data[n-1] = (float)end;
            break;
        }
        case AM_VIEW_TYPE_F64: {
            double *data = (double*)res_buf->data;
            double x = start;
            for (int i = 0; i < n - 1; ++i) {
                data[i] = x;
                x += inc;
            }
            data[n-1] = end;
            break;
        }
        case AM_VIEW_TYPE_U8: {
            uint8_t *data = res_buf->data;
            double x = start;
            for (int i = 0; i < n - 1; ++i) {
                data[i] = (uint8_t)x;
                x += inc;
            }
            data[n-1] = (uint8_t)end;
            break;
        }
        case AM_VIEW_TYPE_I8: {
            int8_t *data = (int8_t*)res_buf->data;
            double x = start;
            for (int i = 0; i < n - 1; ++i) {
                data[i] = (int8_t)x;
                x += inc;
            }
            data[n-1] = (int8_t)end;
            break;
        }
        case AM_VIEW_TYPE_U16: {
            uint16_t *data = (uint16_t*)res_buf->data;
            double x = start;
            for (int i = 0; i < n - 1; ++i) {
                data[i] = (uint16_t)x;
                x += inc;
            }
            data[n-1] = (uint16_t)end;
            break;
        }
        case AM_VIEW_TYPE_I16: {
            int16_t *data = (int16_t*)res_buf->data;
            double x = start;
            for (int i = 0; i < n - 1; ++i) {
                data[i] = (int16_t)x;
                x += inc;
            }
            data[n-1] = (int16_t)end;
            break;
        }
        case AM_VIEW_TYPE_U32: {
            uint32_t *data = (uint32_t*)res_buf->data;
            double x = start;
            for (int i = 0; i < n - 1; ++i) {
                data[i] = (uint32_t)x;
                x += inc;
            }
            data[n-1] = (uint32_t)end;
            break;
        }
        case AM_VIEW_TYPE_I32: {
            int32_t *data = (int32_t*)res_buf->data;
            double x = start;
            for (int i = 0; i < n - 1; ++i) {
                data[i] = (int32_t)x;
                x += inc;
            }
            data[n-1] = (int32_t)end;
            break;
        }
        default:
            return luaL_error(L, "sorry, range does not support type %s", info.name);
    }
    am_buffer_view *res_view = am_new_buffer_view(L, type, 1);
    res_view->buffer = res_buf;
    res_view->buffer_ref = res_view->ref(L, -2);
    lua_remove(L, -2); // buffer
    res_view->stride = info.size;
    res_view->size = n;
    return 1;
}

int am_mathv_random(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_rand *rand;
    if (am_get_type(L, 1) == MT_am_rand) {
        rand = am_get_userdata(L, am_rand, 1);
        lua_remove(L, 1);
        nargs--;
    } else {
        rand = am_get_default_rand(L);
    }
    if (nargs < 2) {
        return luaL_error(L, "random: expecting more arguments");
    }

    am_buffer_view_type type = am_get_enum(L, am_buffer_view_type, 1);
    am_view_type_info info = am_view_type_infos[type];
    int n = luaL_checkinteger(L, 2);
    if (n < 0) {
        return luaL_error(L, "number of random numbers to generate must be non-negative");
    }
    double lo = 0.0;
    double hi = 1.0;
    if (nargs > 2) {
        lo = luaL_checknumber(L, 3);
    }
    if (nargs > 3) {
        hi = luaL_checknumber(L, 4);
    }
    double range = hi - lo;

    am_buffer *res_buf = am_push_new_buffer_and_init(L, info.size * n);

    switch (type) {
        case AM_VIEW_TYPE_F32: {
            float *data = (float*)res_buf->data;
            float frange = (float)range;
            float flo = (float)lo;
            for (int i = 0; i < n; ++i) {
                data[i] = rand->get_randf() * frange + flo;
            }
            break;
        }
        case AM_VIEW_TYPE_F64: {
            double *data = (double*)res_buf->data;
            for (int i = 0; i < n; ++i) {
                data[i] = rand->get_rand() * range + lo;
            }
            break;
        }
        default:
            return luaL_error(L, "sorry, random does not support type %s", info.name);
    }

    am_buffer_view *res_view = am_new_buffer_view(L, type, 1);
    res_view->buffer = res_buf;
    res_view->buffer_ref = res_view->ref(L, -2);
    lua_remove(L, -2); // buffer
    res_view->stride = info.size;
    res_view->size = n;
    return 1;
}

int am_mathv_cart(lua_State *L) {
    am_check_nargs(L, 2);
    am_buffer_view *view1 = am_check_buffer_view(L, 1);
    am_buffer_view *view2 = am_check_buffer_view(L, 2);
    int type1 = view1->type;
    int type2 = view2->type;
    am_view_type_info info1 = am_view_type_infos[type1];
    am_view_type_info info2 = am_view_type_infos[type2];
    if (info1.base_type != info2.base_type) {
        return luaL_error(L, "cart: views must have same base type (%s vs %s)", info1.name, info2.name);
    }
    int components1 = view1->components;
    int components2 = view2->components;
    int result_components = components1 + components2;
    if (result_components > 4) {
        return luaL_error(L, "cart: views must have combined components <= 4");
    }
    int base_size = info1.size;
    am_buffer *result_buf = am_push_new_buffer_and_init(L, base_size * result_components * view1->size * view2->size);
    uint8_t *data1 = view1->buffer->data;
    uint8_t *data2 = view2->buffer->data;
    int stride1 = view1->stride;
    int stride2 = view2->stride;
    uint8_t *result_data = result_buf->data;
    for (int i = 0; i < view2->size; ++i) {
        for (int j = 0; j < view1->size; ++j) {
            memcpy(result_data, data1, base_size * components1);
            result_data += base_size * components1;
            memcpy(result_data, data2, base_size * components2);
            result_data += base_size * components2;
            data1 += stride1;
        }
        data1 = view1->buffer->data;
        data2 += stride2;
    }
    am_buffer_view *result_view = am_new_buffer_view(L, view1->type, result_components);
    result_view->buffer = result_buf;
    result_view->buffer_ref = result_view->ref(L, -2);
    lua_remove(L, -2); // result_buf
    result_view->stride = base_size * result_components;
    result_view->size = view1->size * view2->size;
    return 1;
}

static int am_mathv_vec_mul(lua_State *L);
static int am_mathv_mat_mul(lua_State *L);

int am_mathv_mul(lua_State *L) {
    // decide if we need to do matrix multiplication or not
    int t1 = am_get_type(L, 1);
    switch (t1) {
        case MT_am_mat3:
        case MT_am_mat4:
            return am_mathv_mat_mul(L);
        case MT_am_buffer_view: {
            am_buffer_view *view = am_get_userdata(L, am_buffer_view, 1);
            if (view->components >= 9) {
                return am_mathv_mat_mul(L);
            }
            // fall through
        }
        default: {
            int t2 = am_get_type(L, 2);
            switch (t2) {
                case MT_am_mat3:
                case MT_am_mat4:
                    return am_mathv_mat_mul(L);
                case MT_am_buffer_view: {
                    am_buffer_view *view = am_get_userdata(L, am_buffer_view, 2);
                    if (view->components >= 9) {
                        return am_mathv_mat_mul(L);
                    }
                    // fall through
                }
            }
        }
    }
    return am_mathv_vec_mul(L);
}
