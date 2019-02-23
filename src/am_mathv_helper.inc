static bool read_arg(lua_State *L, int arg, int *arg_type, am_buffer_view_type *view_type, uint8_t **buf, 
    int *stride, int *count, int *components, double *singleton_vals)
{
    *arg_type = am_get_type(L, arg);

    switch (*arg_type) {
        case MT_am_buffer_view:  {
            am_buffer_view *view = am_check_buffer_view(L, arg);
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
    }

    return false;
}
