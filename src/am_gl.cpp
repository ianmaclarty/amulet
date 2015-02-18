/* OpenGL ES 2.0 reference: http://www.khronos.org/opengles/sdk/docs/man/ */
/* WebGL 1.0 quick reference: http://www.khronos.org/files/webgl/webgl-reference-card-1_0.pdf */

#include "amulet.h"

#if defined(AM_WIN32)
    #include <GLES2/gl2.h>
#elif defined(AM_BACKEND_SDL)
    #define GLEW_STATIC 1
    #include "GL/glew.h"
#elif defined(AM_BACKEND_EMSCRIPTEN)
    #include <GLES2/gl2.h>
#endif

#if defined(AM_USE_ANGLE)
#include "GLSLANG/ShaderLang.h"
#endif

#ifdef AM_WIN32
#define __func__ "unknown"
#endif

#define check_for_errors { if (am_conf_check_gl_errors) check_glerror(__FILE__, __LINE__, __func__); }

#define log_gl_call(fmt, ...) {if (am_conf_log_gl_calls) {am_debug("GL: " fmt, __VA_ARGS__);}}

#define check_initialized(...) {if (!am_gl_initialized) {am_log1("%s:%d: attempt to call %s without a valid gl context", __FILE__, __LINE__, __func__); return __VA_ARGS__;}}

#define ATTR_NAME_SIZE 100
#define UNI_NAME_SIZE 100

#if defined(AM_USE_ANGLE)
static void init_angle();
static void destroy_angle();
static void angle_translate_shader(am_shader_type type,
    const char *src, char **objcode, char **errmsg);
#endif

static bool am_gl_initialized = false;

void am_init_gl() {
    assert(!am_gl_initialized);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
#if defined(AM_USE_ANGLE)
    init_angle();
#endif
    am_gl_initialized = true;
}

void am_destroy_gl() {
    if (!am_gl_initialized) return;
#if defined(AM_USE_ANGLE)
    destroy_angle();
#endif
}

bool am_gl_is_initialized() {
    return am_gl_initialized;
}

static void check_glerror(const char *file, int line, const char *func);

static GLenum to_gl_blend_equation(am_blend_equation eq);
static GLenum to_gl_blend_sfactor(am_blend_sfactor f);
static GLenum to_gl_blend_dfactor(am_blend_dfactor f);
static GLenum to_gl_depth_func(am_depth_func f);
static GLenum to_gl_stencil_face_side(am_stencil_face_side fs);
static GLenum to_gl_stencil_func(am_stencil_func f);
static GLenum to_gl_stencil_op(am_stencil_op op);
static GLenum to_gl_buffer_target(am_buffer_target t);
static GLenum to_gl_buffer_usage(am_buffer_usage u);
static GLenum to_gl_face_winding(am_face_winding w);
static GLenum to_gl_cull_face_side(am_cull_face_side fs);
static GLenum to_gl_attr_client_type(am_attribute_client_type t);
static GLenum to_gl_texture_bind_target(am_texture_bind_target t);
static GLenum to_gl_texture_copy_target(am_texture_copy_target t);
static GLenum to_gl_texture_format(am_texture_format f);
static GLenum to_gl_texture_type(am_texture_type t);
static GLenum to_gl_texture_min_filter(am_texture_min_filter f);
static GLenum to_gl_texture_mag_filter(am_texture_mag_filter f);
static GLenum to_gl_texture_wrap(am_texture_wrap w);
static GLenum to_gl_renderbuffer_format(am_renderbuffer_format f);
static GLenum to_gl_framebuffer_attachment(am_framebuffer_attachment a);
static GLenum to_gl_draw_mode(am_draw_mode m);
static GLenum to_gl_element_index_type(am_element_index_type t);
static GLenum to_gl_shader_type(am_shader_type t);

static const char *gl_blend_equation_str(GLenum e);
static const char *gl_blend_factor_str(GLenum e);
static const char *gl_depth_func_str(GLenum e);
static const char *gl_face_side_str(GLenum e);
static const char *gl_stencil_func_str(GLenum e);
static const char *gl_stencil_op_str(GLenum e);
static const char *gl_buffer_target_str(GLenum e);
static const char *gl_usage_str(GLenum e);
static const char *gl_face_winding_str(GLenum e);
static const char *gl_shader_type_str(GLenum e);
static const char *gl_type_str(GLenum e);
static const char *gl_texture_target_str(GLenum e);
static const char *gl_texture_format_str(GLenum e);
static const char *gl_texture_type_str(GLenum e);
static const char *gl_texture_filter_str(GLenum e);
static const char *gl_texture_wrap_str(GLenum e);
static const char *gl_renderbuffer_format_str(GLenum e);
static const char *gl_framebuffer_status_string(GLenum e);
static const char *gl_framebuffer_attachment_str(GLenum e);
static const char *gl_draw_mode_str(GLenum e);

static am_attribute_var_type from_gl_attribute_var_type(GLenum gl_type);
static am_uniform_var_type from_gl_uniform_var_type(GLenum gl_type);
static am_framebuffer_status from_gl_framebuffer_status(GLenum gl_status);

static void get_src_error_line(char *errmsg, const char *src, int *line_no, char **line_str);

// Per-Fragment Operations

void am_set_blend_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_BLEND);
        log_gl_call("glEnable(%s)", "GL_BLEND");
    } else {
        glDisable(GL_BLEND);
        log_gl_call("glDisable(%s)", "GL_BLEND");
    }
    check_for_errors
}

void am_set_blend_color(float r, float g, float b, float a) {
    check_initialized();
    glBlendColor(r, g, b, a);
    log_gl_call("glBlendColor(%0.2f,%0.2f,%0.2f,%0.2f)", r, g, b, a);
    check_for_errors
}

void am_set_blend_equation(am_blend_equation rgb, am_blend_equation alpha) {
    check_initialized();
    GLenum gl_rgb = to_gl_blend_equation(rgb);
    GLenum gl_alpha = to_gl_blend_equation(alpha);
    glBlendEquationSeparate(gl_rgb, gl_alpha);
    log_gl_call("glBlendEquationSeparate(%s, %s)", gl_blend_equation_str(gl_rgb), gl_blend_equation_str(gl_alpha));
    check_for_errors
}

void am_set_blend_func(am_blend_sfactor src_rgb, am_blend_dfactor dst_rgb, am_blend_sfactor src_alpha, am_blend_dfactor dst_alpha) {
    check_initialized();
    GLenum gl_srgb = to_gl_blend_sfactor(src_rgb);
    GLenum gl_drgb = to_gl_blend_dfactor(dst_rgb);
    GLenum gl_salpha = to_gl_blend_sfactor(src_alpha);
    GLenum gl_dalpha = to_gl_blend_dfactor(dst_alpha);
    glBlendFuncSeparate(gl_srgb, gl_drgb, gl_salpha, gl_dalpha);
    log_gl_call("glBlendFuncSeparate(%s, %s, %s, %s)",
        gl_blend_factor_str(gl_srgb), 
        gl_blend_factor_str(gl_drgb), 
        gl_blend_factor_str(gl_salpha), 
        gl_blend_factor_str(gl_dalpha));
    check_for_errors
}

void am_set_depth_test_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
        log_gl_call("glEnable(%s)", "GL_DEPTH_TEST");
    } else {
        glDisable(GL_DEPTH_TEST);
        log_gl_call("glDisable(%s)", "GL_DEPTH_TEST");
    }
    check_for_errors
}

void am_set_depth_func(am_depth_func func) {
    check_initialized();
    GLenum gl_f = to_gl_depth_func(func);
    glDepthFunc(gl_f);
    log_gl_call("glDepthFunc(%s)", gl_depth_func_str(gl_f));
    check_for_errors
}

void am_set_stencil_test_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_STENCIL_TEST);
        log_gl_call("glEnable(%s)", "GL_STENCIL_TEST");
    } else {
        glDisable(GL_STENCIL_TEST);
        log_gl_call("glDisable(%s)", "GL_STENCIL_TEST");
    }
    check_for_errors
}

void am_set_stencil_func(am_stencil_face_side face, am_stencil_func func, am_glint ref, am_gluint mask) {
    check_initialized();
    GLenum gl_face = to_gl_stencil_face_side(face);
    GLenum gl_func = to_gl_stencil_func(func);
    glStencilFuncSeparate(gl_face, gl_func, ref, mask);
    log_gl_call("glStencilFuncSeparate(%s, %s, %d, %u)",
        gl_face_side_str(gl_face), 
        gl_stencil_func_str(gl_func), ref, mask);
    check_for_errors
}

void am_set_stencil_op(am_stencil_face_side face, am_stencil_op fail, am_stencil_op zfail, am_stencil_op zpass) {
    check_initialized();
    GLenum gl_face = to_gl_stencil_face_side(face);
    GLenum gl_fail = to_gl_stencil_op(fail);
    GLenum gl_zfail = to_gl_stencil_op(zfail);
    GLenum gl_zpass = to_gl_stencil_op(zpass);
    glStencilOpSeparate(gl_face, gl_fail, gl_zfail, gl_zpass);
    log_gl_call("glStencilOpSeparate(%s, %s, %s, %s)",
        gl_face_side_str(gl_face), 
        gl_stencil_op_str(gl_fail),
        gl_stencil_op_str(gl_zfail),
        gl_stencil_op_str(gl_zpass));
    check_for_errors
}

void am_set_sample_alpha_to_coverage_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        log_gl_call("glEnable(%s)", "GL_SAMPLE_ALPHA_TO_COVERAGE");
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        log_gl_call("glDisable(%s)", "GL_SAMPLE_ALPHA_TO_COVERAGE");
    }
    check_for_errors
}

void am_set_sample_coverage_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_SAMPLE_COVERAGE);
        log_gl_call("glEnable(%s)", "GL_SAMPLE_COVERAGE");
    } else {
        glDisable(GL_SAMPLE_COVERAGE);
        log_gl_call("glDisable(%s)", "GL_SAMPLE_COVERAGE");
    }
    check_for_errors
}

void am_set_sample_coverage(float value, bool invert) {
    check_initialized();
    glSampleCoverage(value, invert);
    log_gl_call("glSampleCoverage(%f, %d)", value, (int)invert);
    check_for_errors
}

// Whole Framebuffer Operations

void am_clear_framebuffer(bool clear_color_buf, bool clear_depth_buf, bool clear_stencil_buf) {
    check_initialized();
    glClear((clear_color_buf ? GL_COLOR_BUFFER_BIT : 0) | (clear_depth_buf ? GL_DEPTH_BUFFER_BIT : 0) | (clear_stencil_buf ? GL_STENCIL_BUFFER_BIT : 0));
    log_gl_call("glClear(%d, %d, %d)", (int)clear_color_buf, (int)clear_depth_buf, (int)clear_stencil_buf);
    check_for_errors
}

void am_set_framebuffer_clear_color(float r, float g, float b, float a) {
    check_initialized();
    glClearColor(r, g, b, a);
    log_gl_call("glClearColor(%0.2f, %0.2f, %0.2f, %0.2f)", r, g, b, a);
    check_for_errors
}

void am_set_framebuffer_clear_depth(float depth) {
    check_initialized();
    glClearDepthf(depth);
    log_gl_call("glClearDepthf(%f)", depth);
    check_for_errors
}

void am_set_framebuffer_clear_stencil_val(am_glint val) {
    check_initialized();
    glClearStencil(val);
    log_gl_call("glClearStencil(%d)", val);
    check_for_errors
}

void am_set_framebuffer_color_mask(bool r, bool g, bool b, bool a) {
    check_initialized();
    glColorMask(r, g, b, a);
    log_gl_call("glColorMask(%d, %d, %d, %d)", (int)r, (int)g, (int)b, (int)a);
    check_for_errors
}

void am_set_framebuffer_depth_mask(bool flag) {
    check_initialized();
    glDepthMask(flag);
    log_gl_call("glDepthMask(%d)", (int)flag);
    check_for_errors
}

void am_set_framebuffer_stencil_mask(am_stencil_face_side face, am_gluint mask) {
    check_initialized();
    GLenum gl_face = to_gl_stencil_face_side(face);
    glStencilMaskSeparate(gl_face, mask);
    log_gl_call("glStencilMaskSeparate(%s, %u)", gl_face_side_str(gl_face), mask);
    check_for_errors
}

// Buffer Objects

am_buffer_id am_create_buffer_object() {
    check_initialized(0);
    GLuint b;
    glGenBuffers(1, &b);
    log_gl_call("glGenBuffers() = %u", b);
    check_for_errors
    return b;
}

void am_bind_buffer(am_buffer_target target, am_buffer_id buffer) {
    check_initialized();
    GLenum gl_target = to_gl_buffer_target(target);
    glBindBuffer(gl_target, buffer);
    log_gl_call("glBindBuffer(%s, %u)", gl_buffer_target_str(gl_target), buffer);
    check_for_errors
}

void am_set_buffer_data(am_buffer_target target, int size, void *data, am_buffer_usage usage) {
    check_initialized();
    GLenum gl_target = to_gl_buffer_target(target);
    GLenum gl_usage = to_gl_buffer_usage(usage);
    glBufferData(gl_target, size, data, gl_usage);
    log_gl_call("glBufferData(%s, %d, %p, %s)", gl_buffer_target_str(gl_target), size, data, gl_usage_str(gl_usage));
    check_for_errors
}

void am_set_buffer_sub_data(am_buffer_target target, int offset, int size, void *data) {
    check_initialized();
    GLenum gl_target = to_gl_buffer_target(target);
    glBufferSubData(gl_target, offset, size, data);
    log_gl_call("glBufferSubData(%s, %d, %d, %p)", gl_buffer_target_str(gl_target), offset, size, data);
    check_for_errors
}

void am_delete_buffer(am_buffer_id buffer) {
    check_initialized();
    glDeleteBuffers(1, &buffer);
    log_gl_call("glDeleteBuffers(%u)", buffer);
    check_for_errors
}

// View and Clip

void am_set_depth_range(float near, float far) {
    check_initialized();
    glDepthRangef(near, far);
    log_gl_call("glDepthRangef(%f, %f)", near, far);
    check_for_errors
}

void am_set_scissor_test_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_SCISSOR_TEST);
        log_gl_call("glEnable(%s)", "GL_SCISSOR_TEST");
    } else {
        glDisable(GL_SCISSOR_TEST);
        log_gl_call("glDisable(%s)", "GL_SCISSOR_TEST");
    }
    check_for_errors
}

void am_set_scissor(int x, int y, int w, int h) {
    check_initialized();
    glScissor(x, y, w, h);
    log_gl_call("glScissor(%d, %d, %d, %d)", x, y, w, h);
    check_for_errors
}

void am_set_viewport(int x, int y, int w, int h) {
    check_initialized();
    glViewport(x, y, w, h);
    log_gl_call("glViewport(%d, %d, %d, %d)", x, y, w, h);
    check_for_errors
}

// Rasterization

void am_set_front_face_winding(am_face_winding mode) {
    check_initialized();
    GLenum gl_mode = to_gl_face_winding(mode);
    glFrontFace(gl_mode);
    log_gl_call("glFrontFace(%s)", gl_face_winding_str(mode));
    check_for_errors
}

void am_set_cull_face_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_CULL_FACE);
        log_gl_call("glEnable(%s)", "GL_CULL_FACE");
    } else {
        glDisable(GL_CULL_FACE);
        log_gl_call("glDisable(%s)", "GL_CULL_FACE");
    }
    check_for_errors
}

void am_set_cull_face_side(am_cull_face_side face) {
    check_initialized();
    GLenum gl_face = to_gl_cull_face_side(face);
    glCullFace(gl_face);
    log_gl_call("glCullFace(%s)", gl_face_side_str(gl_face));
    check_for_errors
}

void am_set_line_width(float width) {
    check_initialized();
    glLineWidth(width);
    log_gl_call("glLineWidth(%f)", width);
    check_for_errors
}

void am_set_polygon_offset_fill_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        log_gl_call("glEnable(%s)", "GL_POLYGON_OFFSET_FILL");
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
        log_gl_call("glDisable(%s)", "GL_POLYGON_OFFSET_FILL");
    }
    check_for_errors
}

void am_set_polygon_offset(float factor, float units) {
    check_initialized();
    glPolygonOffset(factor, units);
    log_gl_call("glPolygonOffset(%f, %f)", factor, units);
    check_for_errors
}

// Dithering

void am_set_dither_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_DITHER);
        log_gl_call("glEnable(%s)", "GL_DITHER");
    } else {
        glDisable(GL_DITHER);
        log_gl_call("glDisable(%s)", "GL_DITHER");
    }
    check_for_errors
}

// Programs and Shaders

am_program_id am_create_program() {
    check_initialized(0);
    GLuint p = glCreateProgram();
    log_gl_call("glCreateProgram() = %u", p);
    check_for_errors
    return p;
}

am_shader_id am_create_shader(am_shader_type type) {
    check_initialized(0);
    GLenum gl_type = to_gl_shader_type(type);
    GLuint s = glCreateShader(gl_type);
    log_gl_call("glCreateShader(%s) = %u", gl_shader_type_str(gl_type), s);
    check_for_errors
    return s;
}

bool am_compile_shader(am_shader_id shader, am_shader_type type, const char *src, char **msg, int *line_no, char **line_str) {
    GLint compiled;
    *msg = NULL;
    *line_str = NULL;
    *line_no = -1;
    if (!am_gl_initialized) {
        const char *m = "gl not initialized";
        *msg = (char*)malloc(strlen(m) + 1);
        strcpy(*msg, m);
        compiled = 0;
        goto end;
    }
#if defined(AM_USE_ANGLE)
    {
        char *translate_objcode = NULL;
        char *translate_errmsg = NULL;
        angle_translate_shader(type, src, &translate_objcode, &translate_errmsg);
        if (translate_errmsg != NULL) {
            *msg = translate_errmsg;
            get_src_error_line(*msg, src, line_no, line_str);
            compiled = 0;
            goto end;
        }
        assert(translate_objcode != NULL);
        glShaderSource(shader, 1, (const char**)&translate_objcode, NULL);
        free(translate_objcode);
    }
#else
    glShaderSource(shader, 1, &src, NULL);
#endif
    check_for_errors

    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    check_for_errors
    if (!compiled) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) {
            *msg = (char*)malloc(sizeof(char) * len);
            glGetShaderInfoLog(shader, len, NULL, *msg);
            get_src_error_line(*msg, src, line_no, line_str);
        } else {
            const char *cmsg = "unknown error";
            *msg = (char*)malloc(strlen(cmsg) + 1);
            strcpy(*msg, cmsg);
        }
    }
end:
    log_gl_call("glCompileShader(%s, %u) = %d\n%s",
        gl_shader_type_str(to_gl_shader_type(type)), shader, (int)compiled, src);
    return compiled;
}

void am_attach_shader(am_program_id program, am_shader_id shader) {
    check_initialized();
    glAttachShader(program, shader);
    log_gl_call("glAttachShader(%u, %u)", program, shader);
    check_for_errors
}

bool am_link_program(am_program_id program) {
    check_initialized(false);
    GLint linked;
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    log_gl_call("glLinkProgram(%u) = %d", program, linked);
    check_for_errors
    return linked;
}

char *am_get_program_info_log(am_program_id program) {
    check_initialized(NULL);
    GLint len = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        char* msg = (char*)malloc(sizeof(char) * len);
        glGetProgramInfoLog(program, len, NULL, msg);
        return msg;
    } else {
        const char *cmsg = "unknown error";
        char* msg = (char*)malloc(strlen(cmsg) + 1);
        strcpy(msg, cmsg);
        return msg;
    }
}

int am_get_program_active_attributes(am_program_id program) {
    check_initialized(0);
    GLint val;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &val);
    log_gl_call("glGetProgramiv(%u, GL_ACTIVE_ATTRIBUTES) = %d", program, val);
    check_for_errors
    return val;
}

int am_get_program_active_uniforms(am_program_id program) {
    check_initialized(0);
    GLint val;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &val);
    log_gl_call("glGetProgramiv(%u, GL_ACTIVE_UNIFORMS) = %d", program, val);
    check_for_errors
    return val;
}

bool am_validate_program(am_program_id program) {
    check_initialized(false);
    glValidateProgram(program);
    GLint valid;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &valid);
    log_gl_call("glValidateProgram(%u) = %d", program, valid);
    return valid;
}

void am_use_program(am_program_id program) {
    check_initialized();
    glUseProgram(program);
    log_gl_call("glUseProgram(%u)", program);
    check_for_errors
}

void am_detach_shader(am_program_id program, am_shader_id shader) {
    check_initialized();
    glDetachShader(program, shader);
    log_gl_call("glDetachShader(%u, %u)", program, shader);
    check_for_errors
}

void am_delete_shader(am_shader_id shader) {
    check_initialized();
    glDeleteShader(shader);
    log_gl_call("glDeleteShader(%u)", shader);
    check_for_errors
}

void am_delete_program(am_program_id program) {
    check_initialized();
    glDeleteProgram(program);
    log_gl_call("glDeleteProgram(%u)", program);
    check_for_errors
}

// Uniforms and Attributes 

int am_attribute_client_type_size(am_attribute_client_type t) {
    switch (t) {
        case AM_ATTRIBUTE_CLIENT_TYPE_BYTE: return 1;
        case AM_ATTRIBUTE_CLIENT_TYPE_SHORT: return 2;
        case AM_ATTRIBUTE_CLIENT_TYPE_UBYTE: return 1;
        case AM_ATTRIBUTE_CLIENT_TYPE_USHORT: return 2;
        case AM_ATTRIBUTE_CLIENT_TYPE_FLOAT: return 4;
    }
    return 0;
}

void am_set_attribute_array_enabled(am_gluint location, bool enabled) {
    check_initialized();
    if (enabled) {
        glEnableVertexAttribArray(location);
        log_gl_call("glEnableVertexAttribArray(%u)", location);
    } else {
        glDisableVertexAttribArray(location);
        log_gl_call("glDisableVertexAttribArray(%u)", location);
    }
    check_for_errors
}

am_gluint am_get_attribute_location(am_program_id program, const char *name) {
    check_initialized(0);
    am_gluint l = glGetAttribLocation(program, name);
    log_gl_call("glGetAttribLocation(%u, \"%s\") = %u", program, name, l);
    check_for_errors
    return l;
}

am_gluint am_get_uniform_location(am_program_id program, const char *name) {
    check_initialized(0);
    am_gluint l = glGetUniformLocation(program, name);
    log_gl_call("glGetUniformLocation(%u, \"%s\") = %u", program, name, l);
    check_for_errors
    return l;
}

void am_get_active_attribute(am_program_id program, am_gluint index,
    char **name, am_attribute_var_type *type, int *size)
{
    check_initialized();
    GLchar gl_name[ATTR_NAME_SIZE];
    GLint gl_size;
    GLenum gl_type;
    glGetActiveAttrib(program, index, ATTR_NAME_SIZE, NULL, &gl_size, &gl_type, gl_name);
    log_gl_call("glGetActiveAttrib(%u, %u) = %d, %s, \"%s\"", program, index, gl_size, gl_type_str(gl_type), gl_name);
    check_for_errors
    *name = (char*)malloc(strlen(gl_name) + 1);
    strcpy(*name, gl_name);
    *size = gl_size;
    *type = from_gl_attribute_var_type(gl_type);
}

void am_get_active_uniform(am_program_id program, am_gluint index,
    char **name, am_uniform_var_type *type, int *size)
{
    check_initialized();
    GLchar gl_name[UNI_NAME_SIZE];
    GLint gl_size;
    GLenum gl_type;
    glGetActiveUniform(program, index, UNI_NAME_SIZE, NULL, &gl_size, &gl_type, gl_name);
    log_gl_call("glGetActiveUniform(%u, %u) = %d, %s, \"%s\"", program, index, gl_size, gl_type_str(gl_type), gl_name);
    check_for_errors
    *name = (char*)malloc(strlen(gl_name) + 1);
    strcpy(*name, gl_name);
    *size = gl_size;
    *type = from_gl_uniform_var_type(gl_type);
}

void am_set_uniform1f(am_gluint location, float value) {
    check_initialized();
    glUniform1fv(location, 1, &value);
    log_gl_call("glUniform1fv(%u, 1, %f)", location, value);
    check_for_errors
}

void am_set_uniform2f(am_gluint location, const float *value) {
    check_initialized();
    glUniform2fv(location, 1, value);
    log_gl_call("glUniform2fv(%u, 1, {%f, %f})", location, value[0], value[1]);
    check_for_errors
}

void am_set_uniform3f(am_gluint location, const float *value) {
    check_initialized();
    glUniform3fv(location, 1, value);
    log_gl_call("glUniform3fv(%u, 1, {%f, %f, %f})", location, value[0], value[1], value[2]);
    check_for_errors
}

void am_set_uniform4f(am_gluint location, const float *value) {
    check_initialized();
    glUniform4fv(location, 1, value);
    log_gl_call("glUniform4fv(%u, 1, {%f, %f, %f, %f})", location, value[0], value[1], value[2], value[3]);
    check_for_errors
}

void am_set_uniform1i(am_gluint location, am_glint value) {
    check_initialized();
    glUniform1iv(location, 1, &value);
    log_gl_call("glUniform1iv(%u, 1, %d)", location, value);
    check_for_errors
}

void am_set_uniform2i(am_gluint location, const am_glint *value) {
    check_initialized();
    glUniform2iv(location, 1, value);
    log_gl_call("glUniform2iv(%u, 1, {%d, %d})", location, value[0], value[1]);
    check_for_errors
}

void am_set_uniform3i(am_gluint location, const am_glint *value) {
    check_initialized();
    glUniform3iv(location, 1, value);
    log_gl_call("glUniform3iv(%u, 1, {%d, %d, %d})", location, value[0], value[1], value[2]);
    check_for_errors
}

void am_set_uniform4i(am_gluint location, const am_glint *value) {
    check_initialized();
    glUniform4iv(location, 1, value);
    log_gl_call("glUniform4iv(%u, 1, {%d, %d, %d, %d})", location, value[0], value[1], value[2], value[3]);
    check_for_errors
}

void am_set_uniform_mat2(am_gluint location, const float *value) {
    check_initialized();
    glUniformMatrix2fv(location, 1, GL_FALSE, value);
    log_gl_call("glUniformMatrix2fv(%u, 1, GL_FALSE, {\n%f, %f, \n%f, %f})", location, value[0], value[1], value[2], value[3]);
    check_for_errors
}

void am_set_uniform_mat3(am_gluint location, const float *value) {
    check_initialized();
    glUniformMatrix3fv(location, 1, GL_FALSE, value);
    log_gl_call("glUniformMatrix3fv(%u, 1, GL_FALSE, {\n%f, %f, %f,\n%f, %f, %f,\n%f, %f, %f})", location,
        value[0], value[1], value[2],
        value[3], value[4], value[5],
        value[6], value[7], value[8]);
    check_for_errors
}

void am_set_uniform_mat4(am_gluint location, const float *value) {
    check_initialized();
    glUniformMatrix4fv(location, 1, GL_FALSE, value);
    log_gl_call("glUniformMatrix4fv(%u, 1, GL_FALSE, {\n%f, %f, %f, %f,\n%f, %f, %f, %f,\n%f, %f, %f, %f,\n%f, %f, %f, %f})",
        location,
        value[0], value[1], value[2], value[3],
        value[4], value[5], value[6], value[7],
        value[8], value[9], value[10], value[11],
        value[12], value[13], value[14], value[15]);
    check_for_errors
}

void am_set_attribute1f(am_gluint location, const float value) {
    check_initialized();
    glVertexAttrib1f(location, value);
    log_gl_call("glVertexAttrib1f(%u, %f)", location, value);
    check_for_errors
}

void am_set_attribute2f(am_gluint location, const float *value) {
    check_initialized();
    glVertexAttrib2fv(location, value);
    log_gl_call("glVertexAttrib2fv(%u, {%f, %f})", location, value[0], value[1]);
    check_for_errors
}

void am_set_attribute3f(am_gluint location, const float *value) {
    check_initialized();
    glVertexAttrib3fv(location, value);
    log_gl_call("glVertexAttrib3fv(%u, {%f, %f, %f})", location, value[0], value[1], value[2]);
    check_for_errors
}

void am_set_attribute4f(am_gluint location, const float *value) {
    check_initialized();
    glVertexAttrib4fv(location, value);
    log_gl_call("glVertexAttrib4fv(%u, {%f, %f, %f, %f})", location, value[0], value[1], value[2], value[3]);
    check_for_errors
}

void am_set_attribute_pointer(am_gluint location, int size, am_attribute_client_type type, bool normalized, int stride, int offset) {
    check_initialized();
    GLenum gl_type = to_gl_attr_client_type(type);
    glVertexAttribPointer(location, size, gl_type, normalized, stride, (void*)((uintptr_t)offset));
    log_gl_call("glVertexAttribPointer(%u, %d, %s, %d, %d, %d)", location, size, gl_type_str(gl_type), (int)normalized, stride, offset);
    check_for_errors
}

// Texture Objects

int am_get_max_texture_units() {
    return GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS;
}

void am_set_active_texture_unit(int texture_unit) {
    check_initialized();
    if (texture_unit < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) {
        glActiveTexture(GL_TEXTURE0 + texture_unit);
        log_gl_call("glActiveTexture(GL_TEXTURE%d)", texture_unit);
    } else {
        am_log1("WARNING: too many active texture units (max %d)", GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    }
    check_for_errors
}

am_texture_id am_create_texture() {
    check_initialized(0);
    GLuint tex;
    glGenTextures(1, &tex);
    log_gl_call("glGenTextures() = %u", tex);
    check_for_errors
    return tex;
}

void am_delete_texture(am_texture_id texture) {
    check_initialized();
    glDeleteTextures(1, &texture);
    log_gl_call("glDeleteTextures(%u)", texture);
    check_for_errors
}

void am_bind_texture(am_texture_bind_target target, am_texture_id texture) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    glBindTexture(gl_target, texture);
    log_gl_call("glBindTexture(%s, %u)", gl_texture_target_str(gl_target), texture);
    check_for_errors
}

void am_copy_texture_image_2d(am_texture_copy_target target, int level, am_texture_format format, int x, int y, int w, int h) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    GLenum gl_format = to_gl_texture_format(format);
    glCopyTexImage2D(gl_target, level, gl_format, x, y, w, h, 0);
    log_gl_call("glCopyTexImage2D(%s, %d, %s, %d, %d, %d, %d, 0)",
        gl_texture_target_str(gl_target), level, gl_texture_format_str(gl_format),
        x, y, w, h);
    check_for_errors
}

void am_copy_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int x, int y, int w, int h) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    glCopyTexSubImage2D(gl_target, level, xoffset, yoffset, x, y, w, h);
    log_gl_call("glCopyTexSubImage2D(%s, %d, %d, %d, %d, %d, %d, %d)",
        gl_texture_target_str(gl_target), level,
        xoffset, yoffset, x, y, w, h);
    check_for_errors
}

void am_generate_mipmap(am_texture_bind_target target) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    glGenerateMipmap(gl_target);
    log_gl_call("glGenerateMipmap(%s)", gl_texture_target_str(gl_target));
    check_for_errors
}

int am_compute_pixel_size(am_texture_format format, am_texture_type type) {
    switch (type) {
        case AM_PIXEL_TYPE_UBYTE:
            switch (format) {
                case AM_TEXTURE_FORMAT_ALPHA:
                case AM_TEXTURE_FORMAT_LUMINANCE:
                    return 1;
                case AM_TEXTURE_FORMAT_LUMINANCE_ALPHA:
                    return 2;
                case AM_TEXTURE_FORMAT_RGB:
                    return 3;
                case AM_TEXTURE_FORMAT_RGBA:
                    return 4;
            }
        case AM_PIXEL_TYPE_USHORT_5_6_5:
        case AM_PIXEL_TYPE_USHORT_4_4_4_4:
        case AM_PIXEL_TYPE_USHORT_5_5_5_1:
            return 2;
    }
    assert(false);
    return 0;
}

void am_set_texture_image_2d(am_texture_copy_target target, int level, am_texture_format format, int w, int h, am_texture_type type, void *data) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    GLenum gl_format = to_gl_texture_format(format);
    GLenum gl_type = to_gl_texture_type(type);
    glTexImage2D(gl_target, level, gl_format, w, h, 0, gl_format, gl_type, data);
    log_gl_call("glTexImage2D(%s, %d, %s, %d, %d, 0, %s, %s, %p)",
        gl_texture_target_str(gl_target), level,
        gl_texture_format_str(gl_format),
        w, h,
        gl_texture_format_str(gl_format),
        gl_texture_type_str(gl_type),
        data);
    check_for_errors
}

void am_set_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int w, int h, am_texture_format format, am_texture_type type, void *data) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    GLenum gl_format = to_gl_texture_format(format);
    GLenum gl_type = to_gl_texture_type(type);
    glTexSubImage2D(gl_target, level, xoffset, yoffset, w, h, gl_format, gl_type, data);
    log_gl_call("glTexSubImage2D(%s, %d, %d, %d, %d, %d, %s, %s, %p)",
        gl_texture_target_str(gl_target), level, xoffset, yoffset, w, h,
        gl_texture_format_str(gl_format),
        gl_texture_type_str(gl_type),
        data);
    check_for_errors
}

void am_set_texture_min_filter(am_texture_bind_target target, am_texture_min_filter filter) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    GLenum gl_filter = to_gl_texture_min_filter(filter);
    glTexParameteri(gl_target, GL_TEXTURE_MIN_FILTER, gl_filter);
    log_gl_call("glTexParameteri(%s, GL_TEXTURE_MIN_FILTER, %s)",
        gl_texture_target_str(gl_target),
        gl_texture_filter_str(gl_filter));
    check_for_errors
}

void am_set_texture_mag_filter(am_texture_bind_target target, am_texture_mag_filter filter) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    GLenum gl_filter = to_gl_texture_mag_filter(filter);
    glTexParameteri(gl_target, GL_TEXTURE_MAG_FILTER, gl_filter);
    log_gl_call("glTexParameteri(%s, GL_TEXTURE_MAG_FILTER, %s)",
        gl_texture_target_str(gl_target),
        gl_texture_filter_str(gl_filter));
    check_for_errors
}

void am_set_texture_wrap(am_texture_bind_target target, am_texture_wrap s_wrap, am_texture_wrap t_wrap) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    GLenum gl_swrap = to_gl_texture_wrap(s_wrap);
    GLenum gl_twrap = to_gl_texture_wrap(t_wrap);
    glTexParameteri(gl_target, GL_TEXTURE_WRAP_S, gl_swrap);
    log_gl_call("glTexParameteri(%s, GL_TEXTURE_WRAP_S, %s)",
        gl_texture_target_str(gl_target),
        gl_texture_wrap_str(gl_swrap));
    check_for_errors
    glTexParameteri(gl_target, GL_TEXTURE_WRAP_T, gl_twrap);
    log_gl_call("glTexParameteri(%s, GL_TEXTURE_WRAP_T, %s)",
        gl_texture_target_str(gl_target),
        gl_texture_wrap_str(gl_twrap));
    check_for_errors
}

// Renderbuffer Objects

am_renderbuffer_id am_create_renderbuffer() {
    check_initialized(0);
    GLuint rb;
    glGenRenderbuffers(1, &rb);
    log_gl_call("glGenRenderbuffers() = %u", rb);
    check_for_errors
    return rb;
}

void am_delete_renderbuffer(am_renderbuffer_id rb) {
    check_initialized();
    glDeleteRenderbuffers(1, &rb);
    log_gl_call("glDeleteRenderbuffers(%u)", rb);
    check_for_errors
}

void am_bind_renderbuffer(am_renderbuffer_id rb) {
    check_initialized();
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    log_gl_call("glBindRenderbuffer(GL_RENDERBUFFER, %u)", rb);
    check_for_errors
}

void am_set_renderbuffer_storage(am_renderbuffer_format format, int w, int h) {
    check_initialized();
    GLenum gl_format = to_gl_renderbuffer_format(format);
    glRenderbufferStorage(GL_RENDERBUFFER, gl_format, w, h);
    log_gl_call("glRenderbufferStorage(GL_RENDERBUFFER, %s, %d, %d)",
        gl_renderbuffer_format_str(gl_format), w, h);
    check_for_errors
}

// Read Back Pixels 

void am_read_pixels(int x, int y, int w, int h, void *data) {
    check_initialized();
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
    log_gl_call("glReadPixels(%d, %d, %d, %d, GL_RGBA, GL_UNSIGNED_BYTE, %p)",
        x, y, w, h, data);
    check_for_errors
}

// Framebuffer Objects

am_framebuffer_id am_create_framebuffer() {
    check_initialized(0);
    GLuint fb;
    glGenFramebuffers(1, &fb);
    log_gl_call("glGenFramebuffers() = %u", fb);
    check_for_errors
    return fb;
}

void am_delete_framebuffer(am_framebuffer_id fb) {
    check_initialized();
    glDeleteFramebuffers(1, &fb);
    log_gl_call("glDeleteFramebuffers(%u)", fb);
    check_for_errors
}

void am_bind_framebuffer(am_framebuffer_id fb) {
    check_initialized();
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    log_gl_call("glBindFramebuffer(GL_FRAMEBUFFER, %u)", fb);
    check_for_errors
}

am_framebuffer_status am_check_framebuffer_status() {
    check_initialized(AM_FRAMEBUFFER_STATUS_UNKNOWN);
    GLenum gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    log_gl_call("glCheckFramebufferStatus(GL_FRAMEBUFFER) = %s", gl_framebuffer_status_string(gl_status));
    return from_gl_framebuffer_status(gl_status);
}

void am_set_framebuffer_renderbuffer(am_framebuffer_attachment attachment, am_renderbuffer_id rb) {
    check_initialized();
    GLenum gl_attachment = to_gl_framebuffer_attachment(attachment);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, gl_attachment, GL_RENDERBUFFER, rb);
    log_gl_call("glFramebufferRenderbuffer(GL_FRAMEBUFFER, %s, GL_RENDERBUFFER, %u)",
        gl_framebuffer_attachment_str(gl_attachment), rb);
    check_for_errors
}

void am_set_framebuffer_texture2d(am_framebuffer_attachment attachment, am_texture_copy_target target, am_texture_id texture) {
    check_initialized();
    GLenum gl_attachment = to_gl_framebuffer_attachment(attachment);
    GLenum gl_target = to_gl_texture_copy_target(target);
    glFramebufferTexture2D(GL_FRAMEBUFFER, gl_attachment, gl_target, texture, 0);
    log_gl_call("glFramebufferTexture2D(GL_FRAMEBUFFER, %s, %s, %u, 0)",
        gl_framebuffer_attachment_str(gl_attachment), gl_texture_target_str(gl_target),
        texture);
    check_for_errors
}

// Writing to the Draw Buffer

void am_draw_arrays(am_draw_mode mode, int first, int count) {
    check_initialized();
    GLenum gl_mode = to_gl_draw_mode(mode);
    glDrawArrays(gl_mode, first, count);
    log_gl_call("glDrawArrays(%s, %d, %d)",
        gl_draw_mode_str(gl_mode), first, count);
    check_for_errors
}

void am_draw_elements(am_draw_mode mode, int count, am_element_index_type type, int offset) {
    check_initialized();
    GLenum gl_mode = to_gl_draw_mode(mode);
    GLenum gl_type = to_gl_element_index_type(type);
    glDrawElements(gl_mode, count, gl_type, (void*)((uintptr_t)offset));
    log_gl_call("glDrawElements(%s, %d, %s, %d)",
        gl_draw_mode_str(gl_mode), count, gl_type_str(gl_type), offset);
    check_for_errors
}

static GLenum to_gl_blend_equation(am_blend_equation eq) {
    switch (eq) {
        case AM_BLEND_EQUATION_ADD: return GL_FUNC_ADD;
        case AM_BLEND_EQUATION_SUBTRACT: return GL_FUNC_SUBTRACT;
        case AM_BLEND_EQUATION_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
    }
    return 0;
}

static GLenum to_gl_blend_sfactor(am_blend_sfactor f) {
    switch (f) {
        case AM_BLEND_SFACTOR_ZERO: return GL_ZERO;
        case AM_BLEND_SFACTOR_ONE: return GL_ONE;
        case AM_BLEND_SFACTOR_SRC_COLOR: return GL_SRC_COLOR;
        case AM_BLEND_SFACTOR_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
        case AM_BLEND_SFACTOR_DST_COLOR: return GL_DST_COLOR;
        case AM_BLEND_SFACTOR_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
        case AM_BLEND_SFACTOR_SRC_ALPHA: return GL_SRC_ALPHA;
        case AM_BLEND_SFACTOR_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
        case AM_BLEND_SFACTOR_DST_ALPHA: return GL_DST_ALPHA;
        case AM_BLEND_SFACTOR_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
        case AM_BLEND_SFACTOR_CONSTANT_COLOR: return GL_CONSTANT_COLOR;
        case AM_BLEND_SFACTOR_ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
        case AM_BLEND_SFACTOR_CONSTANT_ALPHA: return GL_CONSTANT_ALPHA;
        case AM_BLEND_SFACTOR_ONE_MINUS_CONSTANT_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
        case AM_BLEND_SFACTOR_SRC_ALPHA_SATURATE: return GL_SRC_ALPHA_SATURATE;
    }
    return 0;
}

static GLenum to_gl_blend_dfactor(am_blend_dfactor f) {
    switch (f) {
        case AM_BLEND_DFACTOR_ZERO: return GL_ZERO;
        case AM_BLEND_DFACTOR_ONE: return GL_ONE;
        case AM_BLEND_DFACTOR_SRC_COLOR: return GL_SRC_COLOR;
        case AM_BLEND_DFACTOR_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
        case AM_BLEND_DFACTOR_DST_COLOR: return GL_DST_COLOR;
        case AM_BLEND_DFACTOR_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
        case AM_BLEND_DFACTOR_SRC_ALPHA: return GL_SRC_ALPHA;
        case AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
        case AM_BLEND_DFACTOR_DST_ALPHA: return GL_DST_ALPHA;
        case AM_BLEND_DFACTOR_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
        case AM_BLEND_DFACTOR_CONSTANT_COLOR: return GL_CONSTANT_COLOR;
        case AM_BLEND_DFACTOR_ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
        case AM_BLEND_DFACTOR_CONSTANT_ALPHA: return GL_CONSTANT_ALPHA;
        case AM_BLEND_DFACTOR_ONE_MINUS_CONSTANT_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
    }
    return 0;
}

static GLenum to_gl_depth_func(am_depth_func f) {
    switch (f) {
        case AM_DEPTH_FUNC_NEVER: return GL_NEVER;
        case AM_DEPTH_FUNC_ALWAYS: return GL_ALWAYS;
        case AM_DEPTH_FUNC_EQUAL: return GL_EQUAL;
        case AM_DEPTH_FUNC_NOTEQUAL: return GL_NOTEQUAL;
        case AM_DEPTH_FUNC_LESS: return GL_LESS;
        case AM_DEPTH_FUNC_LEQUAL: return GL_LEQUAL;
        case AM_DEPTH_FUNC_GREATER: return GL_GREATER;
        case AM_DEPTH_FUNC_GEQUAL: return GL_GEQUAL;
    }
    return 0;
}

static GLenum to_gl_stencil_face_side(am_stencil_face_side fs) {
    switch (fs) {
        case AM_STENCIL_FACE_FRONT: return GL_FRONT;
        case AM_STENCIL_FACE_BACK: return GL_BACK;
        case AM_STENCIL_FACE_FRONT_AND_BACK: return GL_FRONT_AND_BACK;
    }
    return 0;
}

static GLenum to_gl_stencil_func(am_stencil_func f) {
    switch (f) {
        case AM_STENCIL_FUNC_NEVER: return GL_NEVER;
        case AM_STENCIL_FUNC_ALWAYS: return GL_ALWAYS;
        case AM_STENCIL_FUNC_EQUAL: return GL_EQUAL;
        case AM_STENCIL_FUNC_NOTEQUAL: return GL_NOTEQUAL;
        case AM_STENCIL_FUNC_LESS: return GL_LESS;
        case AM_STENCIL_FUNC_LEQUAL: return GL_LEQUAL;
        case AM_STENCIL_FUNC_GREATER: return GL_GREATER;
        case AM_STENCIL_FUNC_GEQUAL: return GL_GEQUAL;
    }
    return 0;
}

static GLenum to_gl_stencil_op(am_stencil_op op) {
    switch (op) {
        case AM_STENCIL_OP_KEEP: return GL_KEEP;
        case AM_STENCIL_OP_ZERO: return GL_ZERO;
        case AM_STENCIL_OP_REPLACE: return GL_REPLACE;
        case AM_STENCIL_OP_INCR: return GL_INCR;
        case AM_STENCIL_OP_DECR: return GL_DECR;
        case AM_STENCIL_OP_INVERT: return GL_INVERT;
        case AM_STENCIL_OP_INCR_WRAP: return GL_INCR_WRAP;
        case AM_STENCIL_OP_DECR_WRAP: return GL_DECR_WRAP;
    }
    return 0;
}

static GLenum to_gl_buffer_target(am_buffer_target t) {
    switch (t) {
        case AM_ARRAY_BUFFER: return GL_ARRAY_BUFFER;
        case AM_ELEMENT_ARRAY_BUFFER: return GL_ELEMENT_ARRAY_BUFFER;
    }
    return 0;
}

static GLenum to_gl_buffer_usage(am_buffer_usage u) {
    switch (u) {
        case AM_BUFFER_USAGE_STREAM_DRAW: return GL_STREAM_DRAW;
        case AM_BUFFER_USAGE_STATIC_DRAW: return GL_STATIC_DRAW;
        case AM_BUFFER_USAGE_DYNAMIC_DRAW: return GL_DYNAMIC_DRAW;
    }
    return 0;
}

static GLenum to_gl_face_winding(am_face_winding w) {
    switch (w) {
        case AM_FACE_WIND_CW: return GL_CW;
        case AM_FACE_WIND_CCW: return GL_CCW;
    }
    return 0;
}

static GLenum to_gl_cull_face_side(am_cull_face_side fs) {
    switch (fs) {
        case AM_CULL_FACE_FRONT: return GL_FRONT;
        case AM_CULL_FACE_BACK: return GL_BACK;
        case AM_CULL_FACE_FRONT_AND_BACK: return GL_FRONT_AND_BACK;
    }
    return 0;
}

static GLenum to_gl_attr_client_type(am_attribute_client_type t) {
    switch (t) {
        case AM_ATTRIBUTE_CLIENT_TYPE_BYTE: return GL_BYTE;
        case AM_ATTRIBUTE_CLIENT_TYPE_SHORT: return GL_SHORT;
        case AM_ATTRIBUTE_CLIENT_TYPE_UBYTE: return GL_UNSIGNED_BYTE;
        case AM_ATTRIBUTE_CLIENT_TYPE_USHORT: return GL_UNSIGNED_SHORT;
        case AM_ATTRIBUTE_CLIENT_TYPE_FLOAT: return GL_FLOAT;
    }
    return 0;
}

static GLenum to_gl_texture_bind_target(am_texture_bind_target t) {
    switch (t) {
        case AM_TEXTURE_BIND_TARGET_2D: return GL_TEXTURE_2D;
        case AM_TEXTURE_BIND_TARGET_CUBE_MAP: return GL_TEXTURE_CUBE_MAP;
    }
    return 0;
}

static GLenum to_gl_texture_copy_target(am_texture_copy_target t) {
    switch (t) {
        case AM_TEXTURE_COPY_TARGET_2D: return GL_TEXTURE_2D;
        case AM_TEXTURE_COPY_TARGET_CUBE_MAP_POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case AM_TEXTURE_COPY_TARGET_CUBE_MAP_NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case AM_TEXTURE_COPY_TARGET_CUBE_MAP_POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case AM_TEXTURE_COPY_TARGET_CUBE_MAP_NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        case AM_TEXTURE_COPY_TARGET_CUBE_MAP_POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        case AM_TEXTURE_COPY_TARGET_CUBE_MAP_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    }
    return 0;
}

static GLenum to_gl_texture_format(am_texture_format f) {
    switch (f) {
        case AM_TEXTURE_FORMAT_ALPHA: return GL_ALPHA;
        case AM_TEXTURE_FORMAT_LUMINANCE: return GL_LUMINANCE;
        case AM_TEXTURE_FORMAT_LUMINANCE_ALPHA: return GL_LUMINANCE_ALPHA;
        case AM_TEXTURE_FORMAT_RGB: return GL_RGB;
        case AM_TEXTURE_FORMAT_RGBA: return GL_RGBA;
    }
    return 0;
}

static GLenum to_gl_texture_type(am_texture_type t) {
    switch (t) {
        case AM_PIXEL_TYPE_UBYTE: return GL_UNSIGNED_BYTE;
        case AM_PIXEL_TYPE_USHORT_5_6_5: return GL_UNSIGNED_SHORT_5_6_5;
        case AM_PIXEL_TYPE_USHORT_4_4_4_4: return GL_UNSIGNED_SHORT_4_4_4_4;
        case AM_PIXEL_TYPE_USHORT_5_5_5_1: return GL_UNSIGNED_SHORT_5_5_5_1;
    }
    return 0;
}

static GLenum to_gl_texture_min_filter(am_texture_min_filter f) {
    switch (f) {
        case AM_MIN_FILTER_NEAREST: return GL_NEAREST;
        case AM_MIN_FILTER_LINEAR: return GL_LINEAR;
        case AM_MIN_FILTER_NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
        case AM_MIN_FILTER_LINEAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
        case AM_MIN_FILTER_NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
        case AM_MIN_FILTER_LINEAR_MIPMAP_LINEAR: return GL_LINEAR_MIPMAP_LINEAR;
    }
    return 0;
}

static GLenum to_gl_texture_mag_filter(am_texture_mag_filter f) {
    switch (f) {
        case AM_MAG_FILTER_NEAREST: return GL_NEAREST;
        case AM_MAG_FILTER_LINEAR: return GL_LINEAR;
    }
    return 0;
}

static GLenum to_gl_texture_wrap(am_texture_wrap w) {
    switch (w) {
        case AM_TEXTURE_WRAP_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
        case AM_TEXTURE_WRAP_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
        case AM_TEXTURE_WRAP_REPEAT: return GL_REPEAT;
    }
    return 0;
}

static GLenum to_gl_renderbuffer_format(am_renderbuffer_format f) {
    switch (f) {
        case AM_RENDERBUFFER_FORMAT_RGBA4: return GL_RGBA4;
        case AM_RENDERBUFFER_FORMAT_RGB565: return GL_RGB565;
        case AM_RENDERBUFFER_FORMAT_RGB5_A1: return GL_RGB5_A1;
        case AM_RENDERBUFFER_FORMAT_DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT16;
        case AM_RENDERBUFFER_FORMAT_STENCIL_INDEX8: return GL_STENCIL_INDEX8;
    }
    return 0;
}

static GLenum to_gl_framebuffer_attachment(am_framebuffer_attachment a) {
    switch (a) {
        case AM_FRAMEBUFFER_COLOR_ATTACHMENT0: return GL_COLOR_ATTACHMENT0;
        case AM_FRAMEBUFFER_DEPTH_ATTACHMENT: return GL_DEPTH_ATTACHMENT;
        case AM_FRAMEBUFFER_STENCIL_ATTACHMENT: return GL_STENCIL_ATTACHMENT;
    }
    return 0;
}

static GLenum to_gl_draw_mode(am_draw_mode m) {
    switch (m) {
        case AM_DRAWMODE_POINTS: return GL_POINTS;
        case AM_DRAWMODE_LINES: return GL_LINES;
        case AM_DRAWMODE_LINE_STRIP: return GL_LINE_STRIP;
        case AM_DRAWMODE_LINE_LOOP: return GL_LINE_LOOP;
        case AM_DRAWMODE_TRIANGLES: return GL_TRIANGLES;
        case AM_DRAWMODE_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        case AM_DRAWMODE_TRIANGLE_FAN: return GL_TRIANGLE_FAN;
    }
    return 0;
}

static GLenum to_gl_element_index_type(am_element_index_type t) {
    switch (t) {
        case AM_ELEMENT_TYPE_UBYTE: return GL_UNSIGNED_BYTE;
        case AM_ELEMENT_TYPE_USHORT: return GL_UNSIGNED_SHORT;
        case AM_ELEMENT_TYPE_UINT: return GL_UNSIGNED_INT;
    }
    return 0;
}

static GLenum to_gl_shader_type(am_shader_type t) {
    switch (t) {
        case AM_VERTEX_SHADER: return GL_VERTEX_SHADER;
        case AM_FRAGMENT_SHADER: return GL_FRAGMENT_SHADER;
    }
    return 0;
}

static am_attribute_var_type from_gl_attribute_var_type(GLenum gl_type) {
    switch (gl_type) {
        case GL_FLOAT:      return AM_ATTRIBUTE_VAR_TYPE_FLOAT;
        case GL_FLOAT_VEC2: return AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC2;
        case GL_FLOAT_VEC3: return AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC3;
        case GL_FLOAT_VEC4: return AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC4;
        case GL_FLOAT_MAT2: return AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT2;
        case GL_FLOAT_MAT3: return AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT3;
        case GL_FLOAT_MAT4: return AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT4;
        default: return AM_ATTRIBUTE_VAR_TYPE_UNKNOWN;
    }
}

static am_uniform_var_type from_gl_uniform_var_type(GLenum gl_type) {
    switch (gl_type) {
        case GL_FLOAT:      return AM_UNIFORM_VAR_TYPE_FLOAT;
        case GL_FLOAT_VEC2: return AM_UNIFORM_VAR_TYPE_FLOAT_VEC2;
        case GL_FLOAT_VEC3: return AM_UNIFORM_VAR_TYPE_FLOAT_VEC3;
        case GL_FLOAT_VEC4: return AM_UNIFORM_VAR_TYPE_FLOAT_VEC4;
        case GL_FLOAT_MAT2: return AM_UNIFORM_VAR_TYPE_FLOAT_MAT2;
        case GL_FLOAT_MAT3: return AM_UNIFORM_VAR_TYPE_FLOAT_MAT3;
        case GL_FLOAT_MAT4: return AM_UNIFORM_VAR_TYPE_FLOAT_MAT4;
        case GL_INT:        return AM_UNIFORM_VAR_TYPE_INT;
        case GL_INT_VEC2:   return AM_UNIFORM_VAR_TYPE_INT_VEC2;
        case GL_INT_VEC3:   return AM_UNIFORM_VAR_TYPE_INT_VEC3;
        case GL_INT_VEC4:   return AM_UNIFORM_VAR_TYPE_INT_VEC4;
        case GL_BOOL:       return AM_UNIFORM_VAR_TYPE_BOOL;
        case GL_BOOL_VEC2:  return AM_UNIFORM_VAR_TYPE_BOOL_VEC2;
        case GL_BOOL_VEC3:  return AM_UNIFORM_VAR_TYPE_BOOL_VEC3;
        case GL_BOOL_VEC4:  return AM_UNIFORM_VAR_TYPE_BOOL_VEC4;
        case GL_SAMPLER_2D: return AM_UNIFORM_VAR_TYPE_SAMPLER_2D;
        case GL_SAMPLER_CUBE: return AM_UNIFORM_VAR_TYPE_SAMPLER_CUBE;
        default: return AM_UNIFORM_VAR_TYPE_UNKNOWN;
    }
}

static am_framebuffer_status from_gl_framebuffer_status(GLenum gl_status) {
    switch (gl_status) {
        case GL_FRAMEBUFFER_COMPLETE: return AM_FRAMEBUFFER_STATUS_COMPLETE;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return AM_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return AM_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: return AM_FRAMEBUFFER_STATUS_INCOMPLETE_DIMENSIONS;
#endif
        case GL_FRAMEBUFFER_UNSUPPORTED: return AM_FRAMEBUFFER_STATUS_UNSUPPORTED;
        default: return AM_FRAMEBUFFER_STATUS_UNKNOWN;
    }
}

static void check_glerror(const char *file, int line, const char *func) {
    {
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            const char *str = "UNKNOWN";
            switch (err) {
                case GL_INVALID_ENUM: str = "INVALID_ENUM"; break;
                case GL_INVALID_VALUE: str = "INVALID_VALUE"; break;
                case GL_INVALID_OPERATION: str = "INVALID_OPERATION"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: str = "INVALID_FRAMEBUFFER_OPERATION"; break;
                case GL_OUT_OF_MEMORY: str = "OUT_OF_MEMORY"; break;
            }
            am_log1("OpenGL error at %s:%d %s: %s", file, line, func, str);
        }
    }
}

static const char *gl_blend_equation_str(GLenum e) {
    switch (e) {
        case GL_FUNC_ADD: return "GL_FUNC_ADD";
        case GL_FUNC_SUBTRACT: return "GL_FUNC_SUBTRACT";
        case GL_FUNC_REVERSE_SUBTRACT: return "GL_FUNC_REVERSE_SUBTRACT";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_blend_factor_str(GLenum e) {
    switch (e) {
        case GL_ONE: return "GL_ONE";
        case GL_ZERO: return "GL_ZERO";
        case GL_SRC_COLOR: return "GL_SRC_COLOR";
        case GL_ONE_MINUS_SRC_COLOR: return "GL_ONE_MINUS_SRC_COLOR";
        case GL_SRC_ALPHA: return "GL_SRC_ALPHA";
        case GL_ONE_MINUS_SRC_ALPHA: return "GL_ONE_MINUS_SRC_ALPHA";
        case GL_DST_ALPHA: return "GL_DST_ALPHA";
        case GL_ONE_MINUS_DST_ALPHA: return "GL_ONE_MINUS_DST_ALPHA";
        case GL_DST_COLOR: return "GL_DST_COLOR";
        case GL_ONE_MINUS_DST_COLOR: return "GL_ONE_MINUS_DST_COLOR";
        case GL_SRC_ALPHA_SATURATE: return "GL_SRC_ALPHA_SATURATE";
        case GL_CONSTANT_COLOR: return "GL_CONSTANT_COLOR";
        case GL_ONE_MINUS_CONSTANT_COLOR: return "GL_ONE_MINUS_CONSTANT_COLOR";
        case GL_CONSTANT_ALPHA: return "GL_CONSTANT_ALPHA";
        case GL_ONE_MINUS_CONSTANT_ALPHA: return "GL_ONE_MINUS_CONSTANT_ALPHA";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_depth_func_str(GLenum e) {
    switch (e) {
        case GL_NEVER: return "GL_NEVER";
        case GL_LESS: return "GL_LESS";
        case GL_EQUAL: return "GL_EQUAL";
        case GL_LEQUAL: return "GL_LEQUAL";
        case GL_GREATER: return "GL_GREATER";
        case GL_NOTEQUAL: return "GL_NOTEQUAL";
        case GL_GEQUAL: return "GL_GEQUAL";
        case GL_ALWAYS: return "GL_ALWAYS";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_face_side_str(GLenum e) {
    switch (e) {
        case GL_FRONT: return "GL_FRONT";
        case GL_BACK: return "GL_BACK";
        case GL_FRONT_AND_BACK: return "GL_FRONT_AND_BACK";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_stencil_func_str(GLenum e) {
    switch (e) {
        case GL_NEVER: return "GL_NEVER";
        case GL_LESS: return "GL_LESS";
        case GL_EQUAL: return "GL_EQUAL";
        case GL_LEQUAL: return "GL_LEQUAL";
        case GL_GREATER: return "GL_GREATER";
        case GL_NOTEQUAL: return "GL_NOTEQUAL";
        case GL_GEQUAL: return "GL_GEQUAL";
        case GL_ALWAYS: return "GL_ALWAYS";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_stencil_op_str(GLenum e) {
    switch (e) {
        case GL_KEEP: return "GL_KEEP";
        case GL_ZERO: return "GL_ZERO";
        case GL_REPLACE: return "GL_REPLACE";
        case GL_INCR: return "GL_INCR";
        case GL_DECR: return "GL_DECR";
        case GL_INVERT: return "GL_INVERT";
        case GL_INCR_WRAP: return "GL_INCR_WRAP";
        case GL_DECR_WRAP: return "GL_DECR_WRAP";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_buffer_target_str(GLenum e) {
    switch (e) {
        case GL_ARRAY_BUFFER: return "GL_ARRAY_BUFFER";
        case GL_ELEMENT_ARRAY_BUFFER: return "GL_ELEMENT_ARRAY_BUFFER";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_usage_str(GLenum e) {
    switch (e) {
        case GL_STREAM_DRAW: return "GL_STREAM_DRAW";
        case GL_STATIC_DRAW: return "GL_STATIC_DRAW";
        case GL_DYNAMIC_DRAW: return "GL_DYNAMIC_DRAW";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_face_winding_str(GLenum e) {
    switch (e) {
        case GL_CW: return "GL_CW";
        case GL_CCW: return "GL_CCW";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_shader_type_str(GLenum e) {
    switch (e) {
        case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
        case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_type_str(GLenum e) {
    switch (e) {
        case GL_FLOAT: return "GL_FLOAT";
        case GL_FLOAT_VEC2: return "GL_FLOAT_VEC2";
        case GL_FLOAT_VEC3: return "GL_FLOAT_VEC3";
        case GL_FLOAT_VEC4: return "GL_FLOAT_VEC4";
        case GL_FLOAT_MAT2: return "GL_FLOAT_MAT2";
        case GL_FLOAT_MAT3: return "GL_FLOAT_MAT3";
        case GL_FLOAT_MAT4: return "GL_FLOAT_MAT4";
        case GL_INT: return "GL_INT";
        case GL_INT_VEC2: return "GL_INT_VEC2";
        case GL_INT_VEC3: return "GL_INT_VEC3";
        case GL_INT_VEC4: return "GL_INT_VEC4";
        case GL_BOOL: return "GL_BOOL";
        case GL_BOOL_VEC2: return "GL_BOOL_VEC2";
        case GL_BOOL_VEC3: return "GL_BOOL_VEC3";
        case GL_BOOL_VEC4: return "GL_BOOL_VEC4";
        case GL_BYTE: return "GL_BYTE";
        case GL_UNSIGNED_BYTE: return "GL_UNSIGNED_BYTE";
        case GL_SHORT: return "GL_SHORT";
        case GL_UNSIGNED_SHORT: return "GL_UNSIGNED_SHORT";
        case GL_UNSIGNED_INT: return "GL_UNSIGNED_INT";
        case GL_FIXED: return "GL_FIXED";
        case GL_SAMPLER_2D: return "GL_SAMPLER_2D";
        case GL_SAMPLER_CUBE: return "GL_SAMPLER_CUBE";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_texture_target_str(GLenum e) {
    switch (e) {
        case GL_TEXTURE_2D: return "GL_TEXTURE_2D";
        case GL_TEXTURE_CUBE_MAP: return "GL_TEXTURE_CUBE_MAP";
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X: return "GL_TEXTURE_CUBE_MAP_POSITIVE_X";
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: return "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: return "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: return "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: return "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: return "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_texture_format_str(GLenum e) {
    switch (e) {
        case GL_ALPHA: return "GL_ALPHA";
        case GL_RGB: return "GL_RGB";
        case GL_RGBA: return "GL_RGBA";
        case GL_LUMINANCE: return "GL_LUMINANCE";
        case GL_LUMINANCE_ALPHA: return "GL_LUMINANCE_ALPHA";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_texture_type_str(GLenum e) {
    switch (e) {
        case GL_UNSIGNED_BYTE: return "GL_UNSIGNED_BYTE";
        case GL_UNSIGNED_SHORT_4_4_4_4: return "GL_UNSIGNED_SHORT_4_4_4_4";
        case GL_UNSIGNED_SHORT_5_5_5_1: return "GL_UNSIGNED_SHORT_5_5_5_1";
        case GL_UNSIGNED_SHORT_5_6_5: return "GL_UNSIGNED_SHORT_5_6_5";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_texture_filter_str(GLenum e) {
    switch (e) {
        case GL_NEAREST: return "GL_NEAREST";
        case GL_LINEAR: return "GL_LINEAR";
        case GL_NEAREST_MIPMAP_NEAREST: return "GL_NEAREST_MIPMAP_NEAREST";
        case GL_LINEAR_MIPMAP_NEAREST: return "GL_LINEAR_MIPMAP_NEAREST";
        case GL_NEAREST_MIPMAP_LINEAR: return "GL_NEAREST_MIPMAP_LINEAR";
        case GL_LINEAR_MIPMAP_LINEAR: return "GL_LINEAR_MIPMAP_LINEAR";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_texture_wrap_str(GLenum e) {
    switch (e) {
        case GL_REPEAT: return "GL_REPEAT";
        case GL_CLAMP_TO_EDGE: return "GL_CLAMP_TO_EDGE";
        case GL_MIRRORED_REPEAT: return "GL_MIRRORED_REPEAT";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_renderbuffer_format_str(GLenum e) {
    switch (e) {
        case GL_RGBA4: return "GL_RGBA4";
        case GL_RGB5_A1: return "GL_RGB5_A1";
        case GL_RGB565: return "GL_RGB565";
        case GL_DEPTH_COMPONENT16: return "GL_DEPTH_COMPONENT16";
        case GL_STENCIL_INDEX8: return "GL_STENCIL_INDEX8";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_framebuffer_status_string(GLenum e) {
    switch (e) {
        case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
#ifdef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
#endif
        case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_framebuffer_attachment_str(GLenum e) {
    switch (e) {
        case GL_COLOR_ATTACHMENT0: return "GL_COLOR_ATTACHMENT0";
        case GL_DEPTH_ATTACHMENT: return "GL_DEPTH_ATTACHMENT";
        case GL_STENCIL_ATTACHMENT: return "GL_STENCIL_ATTACHMENT";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static const char *gl_draw_mode_str(GLenum e) {
    switch (e) {
        case GL_POINTS: return "GL_POINTS";
        case GL_LINES: return "GL_LINES";
        case GL_LINE_LOOP: return "GL_LINE_LOOP";
        case GL_LINE_STRIP: return "GL_LINE_STRIP";
        case GL_TRIANGLES: return "GL_TRIANGLES";
        case GL_TRIANGLE_STRIP: return "GL_TRIANGLE_STRIP";
        case GL_TRIANGLE_FAN: return "GL_TRIANGLE_FAN";
    }
    return "<UNKNOWN GL CONSTANT>";
}

static void get_src_error_line(char *errmsg, const char *src, int *line_no, char **line_str) {
    *line_no = -1;
    *line_str = NULL;
    int res = sscanf(errmsg, "ERROR: 0:%d:", line_no);
    if (res == 0 || *line_no < 1) return;
    const char *ptr = src;
    int l = 1;
    while (*ptr != '\0') {   
        if (l == *line_no) {
            const char *start = ptr;
            while (*ptr != '\0' && *ptr != '\n') ptr++;
            const char *end = ptr;
            size_t len = end - start;
            *line_str = (char*)malloc(len+1);
            memcpy(*line_str, start, len);
            (*line_str)[len] = '\0';
            return;
        }
        if (*ptr == '\n') l++;
        ptr++;
    }
}

//-------------------------------------------------------------------------------------------------
// Angle shader translater
#if defined(AM_USE_ANGLE)

static ShBuiltInResources angle_resources;
static ShHandle angle_vcompiler;
static ShHandle angle_fcompiler;
static int angle_comp_options =
    SH_OBJECT_CODE | SH_INIT_GL_POSITION |
    SH_UNFOLD_SHORT_CIRCUIT | SH_INIT_VARYINGS_WITHOUT_STATIC_USE;

static void init_angle() {
    ShInitialize();

    ShInitBuiltInResources(&angle_resources);
    angle_resources.MaxVertexAttribs = 16;
    angle_resources.MaxVertexUniformVectors = 128;
    angle_resources.MaxVaryingVectors = 16;
    angle_resources.MaxVertexTextureImageUnits = 8;
    angle_resources.MaxCombinedTextureImageUnits = 16;
    angle_resources.MaxTextureImageUnits = 16;
    angle_resources.MaxFragmentUniformVectors = 32;
    angle_resources.MaxDrawBuffers = 4;

    ShShaderSpec spec = SH_WEBGL_SPEC;
    ShShaderOutput output = SH_GLSL_OUTPUT;

    angle_vcompiler = ShConstructCompiler(SH_VERTEX_SHADER, spec, output, &angle_resources);
    angle_fcompiler = ShConstructCompiler(SH_FRAGMENT_SHADER, spec, output, &angle_resources);
}

static void destroy_angle() {
    ShDestruct(angle_vcompiler);
    ShDestruct(angle_fcompiler);
}

static void angle_translate_shader(am_shader_type type,
    const char *src, char **objcode, char **errmsg)
{
    size_t bufferLen = 0;

    *objcode = NULL;
    *errmsg = NULL;
    ShHandle compiler = NULL;

    switch (type) {
        case AM_VERTEX_SHADER: compiler = angle_vcompiler; break;
        case AM_FRAGMENT_SHADER: compiler = angle_fcompiler; break;
    }
    int ret = ShCompile(compiler, &src, 1, angle_comp_options);
    if (ret) {
        ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, &bufferLen);
        *objcode = (char*) malloc(bufferLen + 1);
        memset(*objcode, 0, bufferLen + 1);
        ShGetObjectCode(compiler, *objcode);
        if (am_conf_dump_translated_shaders) {
            am_debug("translated shader:\n%s", *objcode);
        }
    } else {
        ShGetInfo(compiler, SH_INFO_LOG_LENGTH, &bufferLen);
        *errmsg = (char*) malloc(bufferLen + 1);
        memset(*errmsg, 0, bufferLen + 1);
        ShGetInfoLog(compiler, *errmsg);
    }
}
#endif
