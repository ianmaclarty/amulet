#define ADD_F32(x, y) ((x) + (y))
#define SUB_F32(x, y) ((x) - (y))
#define MUL_F32(x, y) ((x) * (y))
#define DIV_F32(x, y) ((x) / (y))
#define UNM_F32(x) (-(x))

#define PERLIN1_F32(x) (glm::perlin(glm::vec2(x, 0.0f)))
#define PERLIN2_F32(x, y) (glm::perlin(glm::vec2(x, 0.0f), glm::vec2(y, 1.0f)))
#define SIMPLEX1_F32(x) (glm::simplex(glm::vec2(x, 0.0f)))

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
        case LUA_TBOOLEAN: {
            singleton_vals[0] = (double)lua_toboolean(L, arg);
            *stride = 0;
            *count = 1;
            *components = 1;
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

int am_mathv_range(lua_State *L) {
    am_check_nargs(L, 3);
    int n = luaL_checkinteger(L, 1);
    if (n < 2) {
        return luaL_error(L, "range count must be at least 2");
    }
    float start = (float)luaL_checknumber(L, 2);
    float end = (float)luaL_checknumber(L, 3);
    float inc = (end - start) / (float)(n-1);

    am_buffer *res_buf = am_push_new_buffer_and_init(L, 4 * n);
    float *data = (float*)res_buf->data;
    float x = start;
    for (int i = 0; i < n - 1; i++) {
        data[i] = x;
        x += inc;
    }
    data[n-1] = end;
    am_buffer_view *res_view = am_new_buffer_view(L, AM_VIEW_TYPE_F32, 1);
    res_view->buffer = res_buf;
    res_view->buffer_ref = res_view->ref(L, -2);
    lua_remove(L, -2); // buffer
    res_view->stride = 4;
    res_view->size = n;
    return 1;
}

int am_mathv_random(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_rand *rand;
    int arg0 = 1;
    if (am_get_type(L, 1) == MT_am_rand) {
        rand = am_get_userdata(L, am_rand, 1);
        arg0 = 2;
        nargs--;
    } else {
        rand = am_get_default_rand(L);
    }
    if (nargs == 0) {
        return luaL_error(L, "expecting more arguments");
    }
    int n = luaL_checkinteger(L, arg0);
    if (n < 1) {
        return luaL_error(L, "number of random numbers to generate must be positive");
    }
    float lo = 0.0;
    float hi = 1.0;
    if (nargs > 1) {
        lo = (float)luaL_checknumber(L, arg0 + 1);
    }
    if (nargs > 2) {
        hi = (float)luaL_checknumber(L, arg0 + 2);
    }
    float range = hi - lo;

    am_buffer *res_buf = am_push_new_buffer_and_init(L, 4 * n);
    float *data = (float*)res_buf->data;
    for (int i = 0; i < n; i++) {
        data[i] = rand->get_randf() * range + lo;
    }
    am_buffer_view *res_view = am_new_buffer_view(L, AM_VIEW_TYPE_F32, 1);
    res_view->buffer = res_buf;
    res_view->buffer_ref = res_view->ref(L, -2);
    lua_remove(L, -2); // buffer
    res_view->stride = 4;
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
    if (info1.base_type != AM_VIEW_TYPE_F32 || info2.base_type != AM_VIEW_TYPE_F32) {
        return luaL_error(L, "cart not supported for views of type %s and %s", info1.name, info2.name);
    }
    if (view1->offset & 3 || view1->stride & 3) {
        return luaL_error(L, "view must be 4-byte aligned");
    }
    if (view2->offset & 3 || view2->stride & 3) {
        return luaL_error(L, "view must be 4-byte aligned");
    }
    int components1 = view1->components;
    int components2 = view2->components;
    int components = components1 + components2;
    if (components > 4) {
        return luaL_error(L, "sum of cart view components must be <= 4");
    }
    am_buffer_view_type type = AM_VIEW_TYPE_F32;
    am_buffer *buf = am_push_new_buffer_and_init(L, 4 * components * view1->size * view2->size);
    float *data1 = (float*)view1->buffer->data;
    float *data2 = (float*)view2->buffer->data;
    int stride1 = view1->stride >> 2;
    int stride2 = view2->stride >> 2;
    float *data = (float*)buf->data;
    int d = 0;
    for (int i = 0; i < view2->size; ++i) {
        for (int j = 0; j < view1->size; ++j) {
            for (int c = 0; c < components1; ++c) {
                data[d++] = data1[c];
            }
            for (int c = 0; c < components2; ++c) {
                data[d++] = data2[c];
            }
            data1 += stride1;
        }
        data1 = (float*)view1->buffer->data;
        data2 += stride2;
    }
    am_buffer_view *view = am_new_buffer_view(L, type, components);
    view->buffer = buf;
    view->buffer_ref = view->ref(L, -2);
    lua_remove(L, -2); // buffer
    view->stride = 4 * components;
    view->size = view1->size * view2->size;
    return 1;
}
