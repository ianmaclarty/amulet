#include "amulet.h"

//struct t_face_component {
//    long v;
//    long t;
//    long n;
//};
//
//typedef std::vector<t_face_component> t_face;
//
//static read_obj(const char *filename, char *str, char **errmsg) {
//    *errmsg = NULL;
//    std::vector<glm::vec3> vertices;
//    std::vector<glm::vec2> texture_coords;
//    std::vector<glm::vec3> normals;
//    std::vector<t_face> faces;
//
//    int line = 1;
//    while (*str != 0) {
//        switch (*str) {
//            case '#':
//            case 's':
//                break;
//            case 'v': {
//                str++;
//                switch (*str) {
//                    case ' ': {
//                        str++;
//                        // t_vertex
//                        glm::vec3 v;
//                        v.x = strtof(str, &str);
//                        v.y = strtof(str, &str);
//                        v.z = strtof(str, &str);
//                        vertices.push_back(v);
//                        break;
//                    }
//                    case 't': {
//                        str++;
//                        // texture coord
//                        glm::vec2 t;
//                        t.u = strtof(str, &str);
//                        t.v = strtof(str, &str); // XXX assuming v component is present
//                        texture_coords.push_back(t);
//                        break;
//                    }
//                    case 'n': {
//                        str++;
//                        // t_normal
//                        glm::vec3 n;
//                        n.x = strtof(str, &str);
//                        n.y = strtof(str, &str);
//                        n.z = strtof(str, &str);
//                        normals.push_back(n);
//                        break;
//                    }
//                    case 'p': {
//                        // parameter space vertices
//                        am_log0("%s:%d: WARNING: ignoring parameter space vertices", filename, line);
//                        break;
//                    }
//                    default: {
//                        *errmsg = am_format("%s:%d: WARNING: unrecognised definition", filename, line);
//                        return false;
//                    }
//                }
//                break;
//            }
//            case 'f': {
//                str++;
//                // face
//                t_face fa;
//                while (*str != '\n') {
//                    t_face_component fc;
//                    fc.v = strtol(str, &str, 10);
//                    if (*str == '/') {
//                        str++;
//                        if (*str == '/') {
//                            fc.t = 0;
//                            str++;
//                        } else {
//                            fc.t = strtol(str, &str, 10);
//                        }
//                        if (*str >= '0' && *str <= '9') {
//                            fc.n = strtol(str, &str, 10);
//                        } else {
//                            fc.n = 0;
//                        }
//                    }
//                    fa.push_back(fc);
//                    while (*str == ' ') {
//                        str++;
//                    }
//                }
//                faces.push_back(fa);
//                break;
//            }
//            default: {
//                am_log0("%s:%d: WARNING: unrecognised definition", filename, line);
//                break;
//            }
//        }
//        str = skip_line(str);
//        line++;
//    }
//
//    int num_faces = faces.size();
//    if (num_faces == 0) {
//        am_log0("%s contains no faces", filename);
//        return false;
//    }
//
//    // Check all faces are triangles
//    for (int i = 0; i < num_faces; i++) {
//        if (faces[i].size() != 3) {
//            am_log0("%s: sorry, only triangle faces are currently supported", filename);
//            return false;
//        }
//    }
//
//    bool has_normals = faces[0][0].n > 0;
//    bool has_texture_coords = faces[0][0].t > 0;   
//    int stride = 12;
//    if (has_normals) {
//        stride += 12;
//    }
//    if (has_texture_coords) {
//        stride += 8;
//    }
//
//    float *vert_data = (float*)malloc(stride * 3 * num_faces);
//
//    int k = 0;
//    for (int i = 0; i < num_faces; i++) {
//        for (int j = 0; j < 3; j++) {
//            int v = faces[i][j].v - 1;
//            if (v < 0) {
//                am_log0("%s: missing vertex in face %d, vertex %d", filename, i, j);
//                return false;
//            }
//            int n = faces[i][j].n - 1;
//            if (has_normals && n < 0) {
//                ltLog("%s: Error: missing normal in face %d, t_vertex %d", filename, i, j);
//                return false;
//            }
//            int t = faces[i][j].t - 1;
//            if (has_texture_coords && t < 0) {
//                ltLog("%s: Error: missing texture coords in face %d, vertex %d", filename, i, j);
//                return false;
//            }
//
//            vdata[k].xyz = vertices[v];
//            if (n >= 0) {
//                vdata[k].normal = normals[n];
//            }
//            if (t >= 0) {
//                vdata[k].uv = texture_coords[t];
//            }
//            k++;
//        }
//    }
//    assert(k == num_faces * 3);
//
//static int load_obj(lua_State *L) {
//    am_check_nargs(L, 1);
//    am_buffer *buffer = am_get_userdata(L, am_buffer, 1);
//    glm::vec3 *verts;
//    glm::vec3 *normals;
//    glm::vec2 *uvs;
//
//    read_obj(buffer->data, buffer->size
//}

void am_open_model_module(lua_State *L) {
    /*
    luaL_Reg funcs[] = {
        {"load_obj", load_obj},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    */
}
