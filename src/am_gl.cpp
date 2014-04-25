#include "amulet.h"

GLuint load_shader(lua_State *L, GLenum type, const char *src) {
    GLuint shader;
    GLint compiled;
   
    shader = glCreateShader ( type );

    if (shader == 0) {
        lua_pushstring(L, "Unable to create new shader");
        return 0;
    }

    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        const char *type_str = "<unknown>";
        if (type == GL_VERTEX_SHADER) {
            type_str = "Vertex";
        } else if (type == GL_FRAGMENT_SHADER) {
            type_str = "Fragment";
        }
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) {
            char* msg = (char*)malloc(sizeof(char) * len);
            glGetShaderInfoLog(shader, len, NULL, msg);
            lua_pushfstring(L, "%s shader compilation error:\n%s", type_str, msg);
            free(msg);
        } else {
            lua_pushfstring(L, "%s shader compilation error (no error messages available)", type_str);
        }
        glDeleteShader(shader);
        return 0;
   }

   return shader;
}

static int create_shader_program(lua_State *L) {
    GLint linked;
    const char *vertex_shader_source = lua_tostring(L, 1);
    const char *fragment_shader_source = lua_tostring(L, 2);
    if (vertex_shader_source == NULL) {
        return luaL_error(L, "expecting a string in argument 1");
    }
    if (fragment_shader_source == NULL) {
        return luaL_error(L, "expecting a string in argument 2");
    }

    GLuint vertex_shader = load_shader(L, GL_VERTEX_SHADER, vertex_shader_source);
    if (vertex_shader == 0) {
        return lua_error(L);
    }
    GLuint fragment_shader = load_shader(L, GL_FRAGMENT_SHADER, fragment_shader_source);
    if (fragment_shader == 0) {
        glDeleteShader(vertex_shader);
        return lua_error(L);
    }

    GLuint program = glCreateProgram();
    if (program == 0) {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return luaL_error(L, "unable to create new shader program");
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked) {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) {
            char* msg = (char*)malloc(sizeof(char) * len);
            glGetProgramInfoLog(program, len, NULL, msg);
            lua_pushfstring(L, "Shader program link error:\n%s", msg);
            free(msg);
            glDeleteProgram(program);
            return lua_error(L);
        } else {
            glDeleteProgram(program);
            return luaL_error(L, "shader program link error (no error messages available");
        }
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    GLuint *program_ud = (GLuint*)lua_newuserdata(L, sizeof(GLuint));
    *program_ud = program;

    am_set_metatable(L, AM_MT_SHADER_PROGRAM, -1);

    return 1;
}

static int delete_shader_program(lua_State *L) {
    GLuint *program = (GLuint*)am_check_metatable_id(L, AM_MT_SHADER_PROGRAM, 1);
    glDeleteProgram(*program);
    return 0;
}

static int use_shader_program(lua_State *L) {
    GLuint *program = (GLuint*)am_check_metatable_id(L, AM_MT_SHADER_PROGRAM, 1);
    glUseProgram(*program);
    return 0;
}

static int clear(lua_State *L) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}

void register_program_shader_metatable(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, delete_shader_program, 0);
    lua_setfield(L, -2, "__gc");
    am_register_metatable(L, AM_MT_SHADER_PROGRAM);
}

void am_open_gl_module(lua_State *L) {
    register_program_shader_metatable(L);
    luaL_Reg funcs[] = {
        {"create_shader_program",    create_shader_program},
        {"use_shader_program",       use_shader_program},
        {"clear",                    clear},
        {NULL, NULL}
    };
    am_open_module(L, "amulet", funcs);
}
