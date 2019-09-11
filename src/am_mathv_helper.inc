#define MAX_ARGS 16

#define ADD_OP(x, y) ((x) + (y))
#define SUB_OP(x, y) ((x) - (y))
#define MUL_OP(x, y) ((x) * (y))
#define DIV_OP(x, y) ((x) / (y))
#define UNM_OP(x) (-(x))
#define IMOD_OP(x, y) ((x) % (y))
#define F32MOD_OP(a, b) ((a) - floorf((a) / (b)) * (b))
#define F64MOD_OP(a, b) ((a) - floor((a) / (b)) * (b))
#define LT_OP(a, b) (((a) < (b)) ? 1 : 0)
#define LTE_OP(a, b) (((a) <= (b)) ? 1 : 0)
#define GT_OP(a, b) (((a) > (b)) ? 1 : 0)
#define GTE_OP(a, b) (((a) >= (b)) ? 1 : 0)
#define AND_OP(a, b) (((a) && (b)) ? 1 : 0)
#define OR_OP(a, b) (((a) || (b)) ? 1 : 0)
#define NOT_OP(a) ((a) ? 0 : 1)

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
    unsigned int *stride, unsigned int *count, unsigned int *components, double *singleton_vals)
{
    *arg_type = am_get_type(L, arg);
    *view_type = AM_NUM_VIEW_TYPES;

    switch (*arg_type) {
        case MT_am_buffer_view:  {
            am_buffer_view *view = am_check_buffer_view(L, arg);
            *view_type = view->type;
            *buf = view->buffer->data + view->offset;
            *stride = (unsigned int)view->stride;
            *count = (unsigned int)view->size;
            *components = (unsigned int)view->components;
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

static void component_wise_setup(lua_State *L, const char *name, int nargs, 
    int *arg_type, am_buffer_view_type *arg_view_type, unsigned int *arg_count,
    uint8_t **arg_data, unsigned int *arg_stride, unsigned int *arg_components, uint8_t **arg_singleton_bufs,
    unsigned int *output_count, unsigned int *output_components, bool *is_dense)
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

    // compute output count
    *output_count = 0;
    bool was_view_arg = false;
    for (int i = 0; i < nargs; i++) {
        if (arg_type[i] == MT_am_buffer_view) {
            if (was_view_arg && arg_count[i] != *output_count) {
                luaL_error(L, "in call to %s argument %d has size %d, but previous arguments have size %d", name, (i+1), arg_count[i], *output_count);
                return;
            } else {
                *output_count = arg_count[i];
            }
            was_view_arg = true;
        }
    }
    if (!was_view_arg) {
        luaL_error(L, "in call to %s at least one argument must be a view", name);
        return;
    }

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

    // is_dense is true if all the arguments are tightly packed views,
    // which allows for optimisations in the update loop.
    *is_dense = true;
    for (int i = 0; i < nargs; i++) {
        if (arg_type[i] != MT_am_buffer_view || (arg_stride[i] != arg_components[i] * am_view_type_infos[arg_view_type[i]].size)) {
            *is_dense = false;
            break;
        }
    }
}

static void element_wise_setup(lua_State *L, const char *name, int nargs, 
    int *arg_type, am_buffer_view_type *arg_view_type, unsigned int *arg_count,
    uint8_t **arg_data, unsigned int *arg_stride, unsigned int *arg_components, uint8_t **arg_singleton_bufs,
    unsigned int *output_count)
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

    // compute output count
    *output_count = 0;
    bool was_view_arg = false;
    for (int i = 0; i < nargs; i++) {
        if (arg_type[i] == MT_am_buffer_view) {
            if (was_view_arg && arg_count[i] != *output_count) {
                luaL_error(L, "in call to %s argument %d has size %d, but previous arguments have size %d", name, (i+1), arg_count[i], *output_count);
                return;
            } else {
                *output_count = arg_count[i];
            }
            was_view_arg = true;
        }
    }
    if (!was_view_arg) {
        luaL_error(L, "in call to %s at least one argument must be a view", name);
        return;
    }
}

static void setup_non_view_args(lua_State *L, const char *name, int nargs, int *arg_type, am_buffer_view_type *arg_view_type,
    unsigned int *arg_components, uint8_t **arg_singleton_bufs, uint8_t **arg_data) 
{
    for (int i = 0; i < nargs; i++) {
        if (arg_type[i] != MT_am_buffer_view) {
            double *f64s = (double*)arg_singleton_bufs[i];
            arg_data[i] = arg_singleton_bufs[i];
            switch (arg_view_type[i]) {
                case AM_VIEW_TYPE_F32: {
                    float *conv = (float*)f64s;
                    for (unsigned int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (float)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_F64:
                    // nothing to do
                    break;
                case AM_VIEW_TYPE_U8: {
                    uint8_t *conv = (uint8_t*)f64s;
                    for (unsigned int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (uint8_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_I8: {
                    int8_t *conv = (int8_t*)f64s;
                    for (unsigned int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (int8_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_U16: {
                    uint16_t *conv = (uint16_t*)f64s;
                    for (unsigned int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (uint16_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_I16: {
                    int16_t *conv = (int16_t*)f64s;
                    for (unsigned int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (int16_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_U32: {
                    uint32_t *conv = (uint32_t*)f64s;
                    for (unsigned int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (uint32_t)f64s[c];
                    }
                    break;
                }
                case AM_VIEW_TYPE_I32: {
                    int32_t *conv = (int32_t*)f64s;
                    for (unsigned int c = 0; c < arg_components[i]; c++) {
                        conv[c] = (int32_t)f64s[c];
                    }
                    break;
                }
                default:
                    luaL_error(L, "%s does not support arguments with base type %s", name, am_view_type_infos[arg_view_type[i]].name);
                    return;
            }
        }
    }
}

static void create_output_view(lua_State *L, am_buffer_view *target, am_buffer_view_type output_view_type,
    unsigned int *output_count, unsigned int output_components, unsigned int *output_stride, uint8_t **output_data, bool *is_dense)
{
    am_view_type_info output_view_type_info = am_view_type_infos[output_view_type];
    if (target == NULL) {
        // create a new buffer for the output
        *output_stride = (unsigned int)(output_components * output_view_type_info.size);
        am_buffer *output_buffer = am_push_new_buffer_and_init(L, *output_count * (*output_stride));
        *output_data = output_buffer->data;
        am_buffer_view *output_view = am_new_buffer_view(L, output_view_type, output_components);
        output_view->buffer = output_buffer;
        output_view->buffer_ref = output_view->ref(L, -2);
        output_view->offset = 0;
        output_view->stride = *output_stride;
        output_view->size = *output_count;
        lua_remove(L, -2); // remove output_buffer
        *is_dense = true;
    } else {
        // overwrite provided buffer
        if (target->type != output_view_type) {
            luaL_error(L, "target view has incorrect type (expecting %s, got %s)",
                output_view_type_info.name, am_view_type_infos[target->type].name);
            return;
        }
        if ((unsigned int)target->components != output_components) {
            luaL_error(L, "target view has incorrect number of components (expecting %u, got %d)",
                output_components, target->components);
            return;
        }
        *output_count = am_min(*output_count, (unsigned int)target->size);
        target->mark_dirty(0, *output_count);
        *output_stride = (unsigned int)target->stride;
        *output_data = target->buffer->data + target->offset;
        *is_dense = output_components * output_view_type_info.size == *output_stride;
    }
}

int am_mathv_range(lua_State *L) {
    am_check_nargs(L, 4);
    am_buffer_view_type type = am_get_enum(L, am_buffer_view_type, 1);
    am_view_type_info info = am_view_type_infos[type];
    int ns = luaL_checkinteger(L, 2);
    if (ns < 2) {
        return luaL_error(L, "range count must be at least 2");
    }
    unsigned int n = (unsigned int)ns;
    double start = luaL_checknumber(L, 3);
    double end = luaL_checknumber(L, 4);
    double inc = (end - start) / (double)(n-1);

    am_buffer *res_buf = am_push_new_buffer_and_init(L, info.size * n);
    switch (type) {
        case AM_VIEW_TYPE_F32: {
            float *data = (float*)res_buf->data;
            float x = (float)start;
            float dx = (float)inc;
            for (unsigned int i = 0; i < n - 1; ++i) {
                data[i] = x;
                x += dx;
            }
            data[n-1] = (float)end;
            break;
        }
        case AM_VIEW_TYPE_F64: {
            double *data = (double*)res_buf->data;
            double x = start;
            for (unsigned int i = 0; i < n - 1; ++i) {
                data[i] = x;
                x += inc;
            }
            data[n-1] = end;
            break;
        }
        case AM_VIEW_TYPE_U8: {
            uint8_t *data = res_buf->data;
            double x = start;
            for (unsigned int i = 0; i < n - 1; ++i) {
                data[i] = (uint8_t)x;
                x += inc;
            }
            data[n-1] = (uint8_t)end;
            break;
        }
        case AM_VIEW_TYPE_I8: {
            int8_t *data = (int8_t*)res_buf->data;
            double x = start;
            for (unsigned int i = 0; i < n - 1; ++i) {
                data[i] = (int8_t)x;
                x += inc;
            }
            data[n-1] = (int8_t)end;
            break;
        }
        case AM_VIEW_TYPE_U16: {
            uint16_t *data = (uint16_t*)res_buf->data;
            double x = start;
            for (unsigned int i = 0; i < n - 1; ++i) {
                data[i] = (uint16_t)x;
                x += inc;
            }
            data[n-1] = (uint16_t)end;
            break;
        }
        case AM_VIEW_TYPE_I16: {
            int16_t *data = (int16_t*)res_buf->data;
            double x = start;
            for (unsigned int i = 0; i < n - 1; ++i) {
                data[i] = (int16_t)x;
                x += inc;
            }
            data[n-1] = (int16_t)end;
            break;
        }
        case AM_VIEW_TYPE_U32: {
            uint32_t *data = (uint32_t*)res_buf->data;
            double x = start;
            for (unsigned int i = 0; i < n - 1; ++i) {
                data[i] = (uint32_t)x;
                x += inc;
            }
            data[n-1] = (uint32_t)end;
            break;
        }
        case AM_VIEW_TYPE_I32: {
            int32_t *data = (int32_t*)res_buf->data;
            double x = start;
            for (unsigned int i = 0; i < n - 1; ++i) {
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
    int ns = luaL_checkinteger(L, 2);
    if (ns < 0) {
        return luaL_error(L, "number of random numbers to generate must be non-negative");
    }
    unsigned int n = (unsigned int)ns;
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
            for (unsigned int i = 0; i < n; ++i) {
                data[i] = rand->get_randf() * frange + flo;
            }
            break;
        }
        case AM_VIEW_TYPE_F64: {
            double *data = (double*)res_buf->data;
            for (unsigned int i = 0; i < n; ++i) {
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
    unsigned int n1 = (unsigned int)view1->size;
    unsigned int n2 = (unsigned int)view2->size;
    for (unsigned int i = 0; i < n2; ++i) {
        for (unsigned int j = 0; j < n1; ++j) {
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
            am_buffer_view *view = am_check_buffer_view(L, 1);
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
                    am_buffer_view *view = am_check_buffer_view(L, 2);
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

int filter_impl(lua_State *L, am_buffer_view *target) {
    int nargs = lua_gettop(L) - (target == NULL ? 0 : 1);
    if (nargs == 0) {
        return luaL_error(L, "mathv.filter expects at least 1 arg");
    }
    am_buffer_view *data_view;
    am_buffer_view *filter_view;
    if (target != NULL && nargs == 1) {
        data_view = target;
        filter_view = am_check_buffer_view(L, 1);
    } else if (nargs == 2) {
        data_view = am_check_buffer_view(L, 1);
        filter_view = am_check_buffer_view(L, 2);
    } else {
        return luaL_error(L, "mathv.filter expects 1 or 2 arguments");
    }
    if (filter_view->type != AM_VIEW_TYPE_U8 || filter_view->components != 1) {
        return luaL_error(L, "mathv.filter predicate view must have element type ubyte");
    }
    if (filter_view->size != data_view->size) {
        return luaL_error(L, "mathv.filter arguments must have same size");
    }

    unsigned int output_count = 0;
    unsigned int output_stride;
    uint8_t *output_data;
    bool is_dense;
    unsigned int filter_size = (unsigned int)filter_view->size;
    unsigned int filter_stride = (unsigned int)filter_view->stride;
    uint8_t *filter_data = filter_view->buffer->data + filter_view->offset;
    for (unsigned int i = 0; i < filter_size; ++i) {
        output_count += *filter_data ? 1 : 0;
        filter_data += filter_stride;
    }

    create_output_view(L, target, data_view->type, &output_count, data_view->components, &output_stride, &output_data, &is_dense);

    filter_data = filter_view->buffer->data + filter_view->offset;
    uint8_t *in_data = data_view->buffer->data + data_view->offset;
    unsigned int in_stride = (unsigned int)data_view->stride;
    unsigned int output_elem_size = (unsigned int)(data_view->components * am_view_type_infos[data_view->type].size);

    for (unsigned int i = 0; i < filter_size; ++i) {
        if (*filter_data) {
            memcpy(output_data, in_data, output_elem_size);
            output_data += output_stride;
        }
        in_data += in_stride;
        filter_data += filter_stride;
    }

    if (target != NULL) {
        // if overwriting target, return number of matches instead of target
        lua_pop(L, 1);
        lua_pushnumber(L, output_count);
    }
    return 1;
}

int eq_impl(lua_State *L, am_buffer_view *target) {
    int nargs = lua_gettop(L) - (target == NULL ? 0 : 1);
    if (nargs != 2 && nargs != 3) {
        return luaL_error(L, "mathv.eq expects 2 or 3 args");
    }
    uint8_t arg_singleton_scratch[2][16*8];
    uint8_t *arg_singleton_bufs[2];
    for (int i = 0; i < 2; i++) {
        arg_singleton_bufs[i] = &arg_singleton_scratch[i][0];
    }
    uint8_t *arg_data[2];
    unsigned int arg_stride[2];
    unsigned int arg_count[2];
    int arg_type[2];
    am_buffer_view_type arg_view_type[2];
    unsigned int arg_components[2];
    am_buffer_view_type view_type = AM_NUM_VIEW_TYPES;
    unsigned int output_count;
    unsigned int output_components;
    unsigned int output_stride;
    uint8_t *output_data;
    bool output_is_dense;
    element_wise_setup(L, "mathv.eq", 2, 
        arg_type, arg_view_type, arg_count, arg_data, arg_stride, arg_components, arg_singleton_bufs, 
        &output_count);
    for (int i = 0; i < 2; i++) {
        if (arg_type[i] == MT_am_buffer_view) {
            if (view_type != AM_NUM_VIEW_TYPES && arg_view_type[i] != view_type) {
                return luaL_error(L, "mathv.eq arguments must have the same type");
            }
            view_type = arg_view_type[i];
        }
    }
    assert(view_type != AM_NUM_VIEW_TYPES);
    if (arg_components[0] != arg_components[1]) {
        return luaL_error(L, "mathv.eq arguments must have the same number of components");
    }
    arg_view_type[0] = view_type;
    arg_view_type[1] = view_type;
    setup_non_view_args(L, "mathv.eq", 2, arg_type, arg_view_type, arg_components, arg_singleton_bufs, arg_data); 

    double epsilon = 0.0;
    if (nargs > 2) {
        epsilon = luaL_checknumber(L, 3);
        if (view_type != AM_VIEW_TYPE_F32 && view_type != AM_VIEW_TYPE_F64) {
            return luaL_error(L, "mathv.eq epsilon only allowed for float and double views");
        }
    }

    output_components = arg_components[0];
    create_output_view(L, target, AM_VIEW_TYPE_U8, &output_count, output_components, &output_stride, &output_data, &output_is_dense);

    unsigned int stride1 = arg_stride[0];
    unsigned int stride2 = arg_stride[1];
    uint8_t *ptr1 = arg_data[0];
    uint8_t *ptr2 = arg_data[1];

    switch (view_type) {
        case AM_VIEW_TYPE_F32: {
            for (unsigned int i = 0; i < output_count; ++i) {
                uint8_t eq = 1;
                float ep = (float)epsilon;
                for (unsigned int c = 0; c < output_components; c++) {
                    if (fabsf(((float*)ptr1)[c] - ((float*)ptr2)[c]) > ep) {
                        eq = 0;
                        break;
                    }
                }
                *output_data = eq;
                output_data += output_stride;
                ptr1 += stride1;
                ptr2 += stride2;
            }
            break;
        }
        case AM_VIEW_TYPE_F64: {
            for (unsigned int i = 0; i < output_count; ++i) {
                uint8_t eq = 1;
                double ep = epsilon;
                for (unsigned int c = 0; c < output_components; c++) {
                    if (fabs(((double*)ptr1)[c] - ((double*)ptr2)[c]) > ep) {
                        eq = 0;
                        break;
                    }
                }
                *output_data = eq;
                output_data += output_stride;
                ptr1 += stride1;
                ptr2 += stride2;
            }
            break;
        }
        default: {
            // do bit comparison
            int elemsz = output_components * am_view_type_infos[view_type].size;
            for (unsigned int i = 0; i < output_count; ++i) {
                *output_data = (memcmp(ptr1, ptr2, elemsz) == 0) ? 1 : 0;
                output_data += output_stride;
                ptr1 += stride1;
                ptr2 += stride2;
            }
            break;
        }
    }

    return 1;
}

static int am_mathv_sum(lua_State *L) {
    am_check_nargs(L, 1);
    am_buffer_view *view = am_check_buffer_view(L, 1);
    if (view->components > 4) {
        return luaL_error(L, "mathv.sum only supports elements with up to 4 components");
    }
    unsigned int stride = (unsigned int)view->stride;
    unsigned int components = (unsigned int)view->components;
    uint8_t *data = view->buffer->data + view->offset;
    unsigned int size = (unsigned int)view->size;
    double component_sums[16];
    for (unsigned int i = 0; i < 16; i++) component_sums[i] = 0.0;
    switch (view->type) {
        case AM_VIEW_TYPE_F32: {
            for (unsigned int i = 0; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_sums[c] += (double)((float*)data)[c];
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_F64: {
            for (unsigned int i = 0; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_sums[c] += (double)((double*)data)[c];
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_I8: {
            for (unsigned int i = 0; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_sums[c] += (double)((int8_t*)data)[c];
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_U8: {
            for (unsigned int i = 0; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_sums[c] += (double)data[c];
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_I16: {
            for (unsigned int i = 0; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_sums[c] += (double)((int16_t*)data)[c];
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_U16: {
            for (unsigned int i = 0; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_sums[c] += (double)((uint16_t*)data)[c];
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_I32: {
            for (unsigned int i = 0; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_sums[c] += (double)((int32_t*)data)[c];
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_U32: {
            for (unsigned int i = 0; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_sums[c] += (double)((uint32_t*)data)[c];
                }
                data += stride;
            }
            break;
        }
        default:
            return luaL_error(L, "mathv.sum not supported for views with base type %s", am_view_type_infos[view->type].name);
    }
    switch (components) {
        case 1: {
            lua_pushnumber(L, component_sums[0]);
            break;
        }
        case 2: {
            am_vec2 *v = am_new_userdata(L, am_vec2);
            v->v.x = component_sums[0];
            v->v.y = component_sums[1];
            break;
        }
        case 3: {
            am_vec3 *v = am_new_userdata(L, am_vec3);
            v->v.x = component_sums[0];
            v->v.y = component_sums[1];
            v->v.z = component_sums[2];
            break;
        }
        case 4: {
            am_vec4 *v = am_new_userdata(L, am_vec4);
            v->v.x = component_sums[0];
            v->v.y = component_sums[1];
            v->v.z = component_sums[2];
            v->v.w = component_sums[3];
            break;
        }
        default:
            return luaL_error(L, "mathv.sum: unexpected number of components: %d", components);
    }
    return 1;
}

static int am_mathv_greatest(lua_State *L) {
    am_check_nargs(L, 1);
    am_buffer_view *view = am_check_buffer_view(L, 1);
    if (view->components > 4) {
        return luaL_error(L, "mathv.greatest only supports elements with up to 4 components");
    }
    unsigned int stride = (unsigned int)view->stride;
    unsigned int components = (unsigned int)view->components;
    uint8_t *data = view->buffer->data + view->offset;
    unsigned int size = (unsigned int)view->size;
    if (size == 0) {
        lua_pushnil(L);
        return 1;
    }
    double component_max[16];
    switch (view->type) {
        case AM_VIEW_TYPE_F32:
            for (unsigned int c = 0; c < components; ++c) {
                component_max[c] = (double)((float*)data)[c];
            }
            data += stride;
            break;
        case AM_VIEW_TYPE_F64: {
            for (unsigned int c = 0; c < components; ++c) {
                component_max[c] = (double)((double*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_I8: {
            for (unsigned int c = 0; c < components; ++c) {
                component_max[c] = (double)((int8_t*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_U8: {
            for (unsigned int c = 0; c < components; ++c) {
                component_max[c] = (double)data[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_I16: {
            for (unsigned int c = 0; c < components; ++c) {
                component_max[c] = (double)((int16_t*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_U16: {
            for (unsigned int c = 0; c < components; ++c) {
                component_max[c] = (double)((uint16_t*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_I32: {
            for (unsigned int c = 0; c < components; ++c) {
                component_max[c] = (double)((int32_t*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_U32: {
            for (unsigned int c = 0; c < components; ++c) {
                component_max[c] = (double)((uint32_t*)data)[c];
            }
            data += stride;
            break;
        }
        default:
            return luaL_error(L, "mathv.greatest not supported for views with base type %s", am_view_type_infos[view->type].name);
    }
    switch (view->type) {
        case AM_VIEW_TYPE_F32: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_max[c] = am_max(component_max[c], (double)((float*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_F64: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_max[c] = am_max(component_max[c], (double)((double*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_I8: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_max[c] = am_max(component_max[c], (double)((int8_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_U8: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_max[c] = am_max(component_max[c], (double)data[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_I16: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_max[c] = am_max(component_max[c], (double)((int16_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_U16: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_max[c] = am_max(component_max[c], (double)((uint16_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_I32: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_max[c] = am_max(component_max[c], (double)((int32_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_U32: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_max[c] = am_max(component_max[c], (double)((uint32_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        default:
            return luaL_error(L, "mathv.greatest not supported for views with base type %s", am_view_type_infos[view->type].name);
    }
    switch (components) {
        case 1: {
            lua_pushnumber(L, component_max[0]);
            break;
        }
        case 2: {
            am_vec2 *v = am_new_userdata(L, am_vec2);
            v->v.x = component_max[0];
            v->v.y = component_max[1];
            break;
        }
        case 3: {
            am_vec3 *v = am_new_userdata(L, am_vec3);
            v->v.x = component_max[0];
            v->v.y = component_max[1];
            v->v.z = component_max[2];
            break;
        }
        case 4: {
            am_vec4 *v = am_new_userdata(L, am_vec4);
            v->v.x = component_max[0];
            v->v.y = component_max[1];
            v->v.z = component_max[2];
            v->v.w = component_max[3];
            break;
        }
        default:
            return luaL_error(L, "mathv.greatest: unexpected number of components: %d", components);
    }
    return 1;
}

static int am_mathv_least(lua_State *L) {
    am_check_nargs(L, 1);
    am_buffer_view *view = am_check_buffer_view(L, 1);
    if (view->components > 4) {
        return luaL_error(L, "mathv.least only supports elements with up to 4 components");
    }
    unsigned int stride = (unsigned int)view->stride;
    unsigned int components = (unsigned int)view->components;
    uint8_t *data = view->buffer->data + view->offset;
    unsigned int size = (unsigned int)view->size;
    if (size == 0) {
        lua_pushnil(L);
        return 1;
    }
    double component_min[16];
    switch (view->type) {
        case AM_VIEW_TYPE_F32:
            for (unsigned int c = 0; c < components; ++c) {
                component_min[c] = (double)((float*)data)[c];
            }
            data += stride;
            break;
        case AM_VIEW_TYPE_F64: {
            for (unsigned int c = 0; c < components; ++c) {
                component_min[c] = (double)((double*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_I8: {
            for (unsigned int c = 0; c < components; ++c) {
                component_min[c] = (double)((int8_t*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_U8: {
            for (unsigned int c = 0; c < components; ++c) {
                component_min[c] = (double)data[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_I16: {
            for (unsigned int c = 0; c < components; ++c) {
                component_min[c] = (double)((int16_t*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_U16: {
            for (unsigned int c = 0; c < components; ++c) {
                component_min[c] = (double)((uint16_t*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_I32: {
            for (unsigned int c = 0; c < components; ++c) {
                component_min[c] = (double)((int32_t*)data)[c];
            }
            data += stride;
            break;
        }
        case AM_VIEW_TYPE_U32: {
            for (unsigned int c = 0; c < components; ++c) {
                component_min[c] = (double)((uint32_t*)data)[c];
            }
            data += stride;
            break;
        }
        default:
            return luaL_error(L, "mathv.least not supported for views with base type %s", am_view_type_infos[view->type].name);
    }
    switch (view->type) {
        case AM_VIEW_TYPE_F32: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_min[c] = am_min(component_min[c], (double)((float*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_F64: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_min[c] = am_min(component_min[c], (double)((double*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_I8: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_min[c] = am_min(component_min[c], (double)((int8_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_U8: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_min[c] = am_min(component_min[c], (double)data[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_I16: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_min[c] = am_min(component_min[c], (double)((int16_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_U16: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_min[c] = am_min(component_min[c], (double)((uint16_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_I32: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_min[c] = am_min(component_min[c], (double)((int32_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_U32: {
            for (unsigned int i = 1; i < size; ++i) {
                for (unsigned int c = 0; c < components; ++c) {
                    component_min[c] = am_min(component_min[c], (double)((uint32_t*)data)[c]);
                }
                data += stride;
            }
            break;
        }
        default:
            return luaL_error(L, "mathv.least not supported for views with base type %s", am_view_type_infos[view->type].name);
    }
    switch (components) {
        case 1: {
            lua_pushnumber(L, component_min[0]);
            break;
        }
        case 2: {
            am_vec2 *v = am_new_userdata(L, am_vec2);
            v->v.x = component_min[0];
            v->v.y = component_min[1];
            break;
        }
        case 3: {
            am_vec3 *v = am_new_userdata(L, am_vec3);
            v->v.x = component_min[0];
            v->v.y = component_min[1];
            v->v.z = component_min[2];
            break;
        }
        case 4: {
            am_vec4 *v = am_new_userdata(L, am_vec4);
            v->v.x = component_min[0];
            v->v.y = component_min[1];
            v->v.z = component_min[2];
            v->v.w = component_min[3];
            break;
        }
        default:
            return luaL_error(L, "mathv.least: unexpected number of components: %d", components);
    }
    return 1;
}
