#include "amulet.h"

//static const int field_types[] = {
//    GL_FLOAT,
//    GL_UNSIGNED_BYTE,
//    GL_BYTE,
//    GL_UNSIGNED_SHORT,
//    GL_SHORT,
//};
//
//#define MAX_OFFSET 4000
//#define get_flen(t)   ((((t) & 0x0F000000) >> 24)+1)
//#define get_falign(t)  (((t) & 0x00F00000) >> 20)
//#define get_fsize(t)   (((t) & 0x000F0000) >> 16)
//#define get_foffset(t) (((t) & 0x0000FFF0) >> 4)
//#define get_ftype(t)   (((t) & 0x0000000F))
//#define set_foffset(t, os) (((t) | (os << 4)))
//#define mk_type_info(type, size, align, len) ((type) | ((size) << 16) | ((align) << 20) | (((len)-1) << 24))
//
///*
// * create_mesh(num_rows, {
// *    {"position", "float2"},
// *    {"normal",   "byte3"},
// *    {"uv",       "ushort2"},
// * })
// */
//
//struct am_mesh_data {
//    int stride;
//    int size;
//    unsigned char data[];
//};

//static int parse_layout_def(lua_State *L) {
//    int num_fields;
//    int i;
//    int offset = 0;
//    if (!lua_istable(L, 2)) {
//        return luaL_error(L, "expecting a table in argument 2");
//    }
//    lua_newtable(L);
//    num_fields = lua_rawlen(L, 2);
//    for (i = 1; i <= num_fields; i++) {
//        int type_info;
//        lua_rawgeti(L, 2, i); /* push row */
//
//        lua_rawgeti(L, -1, 1); /* lookup field name */
//        if (!lua_isstring(L, -1)) {
//            return luaL_error(L, "missing field name in layout def row %d", i);
//        }
//
//        am_push_metatable(L, AM_MT_MESH);
//        lua_rawgeti(L, -1, 2); /* push field type name */
//        lua_rawget(L, -2);     /* look up type info */
//        if (!lua_isnumber(L, -1)) {
//            return luaL_error(L, "invalid type for field '%s'", lua_tostring(L, -2));
//        }
//        type_info = lua_tointeger(L, -1);
//        while (offset & get_falign(type_info)) {
//            offset++;
//        }
//        if (offset > MAX_OFFSET) {
//            return luaL_error(L, "too many fields :(");
//        }
//
//        /* update type_info with offset */
//        type_info = set_foffset(type_info, offset);
//        lua_pop(L, 1);
//        lua_pushinteger(L, type_info);
//
//        lua_rawset(L, -4); /* env[field name] = type info */
//        lua_pop(L, 1); /* pop row */
//
//        offset += get_flen(type_info);
//    }
//    while (offset & 0x3) { /* ensure rows 4-byte aligned */
//        offset++;
//    }
//    return offset;
//}
//
//static int l_create_mesh(lua_State *L) {
//    int num_rows = luaL_checkinteger(L, 1);
//    if (num_rows <= 0) {
//        return luaL_error(L, "number of rows must be positive");
//    }
//    int stride = parse_layout_def(L); // pushes env table
//    am_mesh_data *mesh = (am_mesh_data*)lua_newuserdata(L, sizeof(am_mesh_data) + num_rows * stride);
//    lua_pushvalue(L, -2);
//    lua_setuservalue(L, -2);
//    lua_remove(L, -2); // remove env table
//    am_set_metatable(L, AM_MT_MESH, -1);
//    return 1;
//}
