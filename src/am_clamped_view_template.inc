#ifndef TNAME
#error TNAME undefined
#endif
#ifndef CTYPE
#error CTYPE undefined
#endif
#ifndef MINVAL
#error MINVAL undefined
#endif
#ifndef MAXVAL
#error MAXVAL undefined
#endif

#define LUA_TYPE LUA_TNUMBER
#define GET_CTYPE(L, idx) ( \
    (MINVAL == 0 && (uint32_t)MAXVAL > INT32_MAX) \
    ? \
    ((CTYPE)am_clamp(lua_tonumber(L, idx), (lua_Number)MINVAL, (lua_Number)MAXVAL)) \
    : \
    ((CTYPE)am_clamp(am_lua_num2int(L, idx), (int)MINVAL, (int)MAXVAL)) \
)
#define PUSH_CTYPE(L, x) { \
    if (MINVAL == 0 && (uint32_t)MAXVAL > INT32_MAX && (uint32_t)x > INT32_MAX) { \
        lua_pushnumber(L, x); \
    } else { \
        lua_pushinteger(L, x); \
    } \
}

#include "am_view_template.inc"

#undef MINVAL
#undef MAXVAL
