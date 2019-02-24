#include "amulet.h"

struct t_face_component {
    int v;
    int t;
    int n;

    t_face_component() {
        v = 0;
        t = 0;
        n = 0;
    }
};

typedef std::vector<t_face_component> t_face;

char *skip_line(char *str) {
    while (*str != '\n' && *str != '\0') {
        str++;
    }
    if (*str == '\0') {
        return str;
    } else {
        return str + 1;
    }
}

static float* read_obj(const char *filename, char *str, int len,
        int *size, int *stride, int *normals_offset, int *texture_coords_offset,
        char **errmsg) {
    *errmsg = NULL;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texture_coords;
    std::vector<glm::vec3> normals;
    std::vector<t_face> faces;

    char *end = str + len;

    int line = 1;
    while (str < end) {
        switch (*str) {
            case '#':
            case 's':
                break;
            case 'v': {
                str++;
                switch (*str) {
                    case ' ': {
                        str++;
                        // t_vertex
                        glm::vec3 v;
                        v.x = strtod(str, &str);
                        v.y = strtod(str, &str);
                        v.z = strtod(str, &str);
                        vertices.push_back(v);
                        break;
                    }
                    case 't': {
                        str++;
                        // texture coord
                        glm::vec2 t;
                        t.x = strtod(str, &str);
                        t.y = strtod(str, &str);
                        texture_coords.push_back(t);
                        break;
                    }
                    case 'n': {
                        str++;
                        // t_normal
                        glm::vec3 n;
                        n.x = strtod(str, &str);
                        n.y = strtod(str, &str);
                        n.z = strtod(str, &str);
                        normals.push_back(n);
                        break;
                    }
                    case 'p': {
                        // parameter space vertices
                        am_log0("%s:%d: WARNING: ignoring parameter space vertices", filename, line);
                        break;
                    }
                    default: {
                        am_log0("%s:%d: WARNING: ignoring unrecognised definition", filename, line);
                        break;
                    }
                }
                break;
            }
            case 'f': {
                str++;
                // face
                t_face fa;
                while (*str != '\n' && *str != '\0') {
                    t_face_component fc;
                    fc.v = strtol(str, &str, 10);
                    if (*str == '/') {
                        str++;
                        if (*str == '/') {
                            fc.t = 0;
                        } else {
                            fc.t = strtol(str, &str, 10);
                        }
                        if (*str == '/') {
                            str++;
                            if (*str >= '0' && *str <= '9') {
                                fc.n = strtol(str, &str, 10);
                            } else {
                                fc.n = 0;
                            }
                        }
                    }
                    fa.push_back(fc);
                    while (*str != '\n' && *str != '\0' && *str != '/' && (*str < '0' || *str > '9')) {
                        str++;
                    }
                }
                faces.push_back(fa);
                break;
            }
            default: {
                //am_log0("%s:%d: WARNING: unrecognised charactor: %c", filename, line, *str);
                break;
            }
        }
        str = skip_line(str);
        line++;
    }

    int num_faces = faces.size();
    if (num_faces == 0) {
        *errmsg = am_format("%s contains no faces", filename);
        return NULL;
    }

    // Check all faces are triangles
    for (int i = 0; i < num_faces; i++) {
        if (faces[i].size() != 3) {
            *errmsg = am_format("%s: sorry, only triangle faces are currently supported (f%d=%d)", filename, i, faces[i].size());
            return NULL;
        }
    }

    bool has_normals = faces[0][0].n > 0;
    bool has_texture_coords = faces[0][0].t > 0;   
    *stride = 12;
    if (has_normals) {
        *normals_offset = *stride;
        *stride += 12;
    }
    if (has_texture_coords) {
        *texture_coords_offset = *stride;
        *stride += 8;
    }

    *size = *stride * 3 * num_faces;
    float *vert_data = (float*)malloc(*size);
    float *ptr = vert_data;

    for (int i = 0; i < num_faces; i++) {
        for (int j = 0; j < 3; j++) {
            int v = faces[i][j].v - 1;
            if (v < 0) {
                free(vert_data);
                *errmsg = am_format("%s: missing vertex in face %d, vertex %d", filename, i+1, j+1);
                return NULL;
            }
            int n = faces[i][j].n - 1;
            if (has_normals && n < 0) {
                free(vert_data);
                *errmsg = am_format("%s: missing normal in face %d, t_vertex %d", filename, i+1, j+1);
                return NULL;
            }
            int t = faces[i][j].t - 1;
            if (has_texture_coords && t < 0) {
                free(vert_data);
                *errmsg = am_format("%s: missing texture coords in face %d, vertex %d", filename, i+1, j+1);
                return NULL;
            }

            ptr[0] = vertices[v].x;
            ptr[1] = vertices[v].y;
            ptr[2] = vertices[v].z;
            ptr += 3;
            if (n >= 0) {
                ptr[0] = normals[n].x;
                ptr[1] = normals[n].y;
                ptr[2] = normals[n].z;
                ptr += 3;
            }
            if (t >= 0) {
                ptr[0] = texture_coords[t].x;
                ptr[1] = texture_coords[t].y;
                ptr += 2;
            }
        }
    }

    return vert_data;
}

static int load_obj(lua_State *L) {
    am_check_nargs(L, 1);
    const char *filename = luaL_checkstring(L, 1);
    char *errmsg = NULL;

    int len;
    char *str = (char*)am_read_resource(filename, &len, &errmsg);
    if (str == NULL) {
        lua_pushstring(L, errmsg);
        free(errmsg);
        return lua_error(L);
    }

    int size;
    int stride;
    int normals_offset = -1;
    int texture_coords_offset = -1;
    uint8_t *data = (uint8_t*)read_obj(filename, str, len, &size,
        &stride, &normals_offset, &texture_coords_offset, &errmsg);
    free(str);
    if (data == NULL) {
        lua_pushstring(L, errmsg);
        free(errmsg);
        return lua_error(L);
    }
    am_push_new_buffer_with_data(L, size, data);
    lua_pushinteger(L, stride);
    if (normals_offset >= 0) {
        lua_pushinteger(L, normals_offset);
    } else {
        lua_pushnil(L);
    }
    if (texture_coords_offset >= 0) {
        lua_pushinteger(L, texture_coords_offset);
    } else {
        lua_pushnil(L);
    }
    return 4;
}

void am_open_model_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"load_obj", load_obj},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
