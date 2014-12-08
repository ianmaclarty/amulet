/* OpenGL ES 2.0 reference: http://www.khronos.org/opengles/sdk/docs/man/ */
/* WebGL 1.0 quick reference: http://www.khronos.org/files/webgl/webgl-reference-card-1_0.pdf */

#include "amulet.h"

#if defined(AM_BACKEND_SDL)
    #define GLEW_STATIC 1
    #include "GL/glew.h"
#elif defined(AM_BACKEND_EMSCRIPTEN)
    #include <GLES2/gl2.h>
#endif

#define check_for_errors { if (am_conf_check_gl_errors) check_glerror(__FILE__, __LINE__, __func__); }

#define check_initialized(...) {if (!am_gl_initialized) {am_report_error("%s:%d: attempt to call %s without a valid gl context", __FILE__, __LINE__, __func__); return __VA_ARGS__;}}

#define ATTR_NAME_SIZE 100
#define UNI_NAME_SIZE 100

bool am_gl_initialized = false;

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
static GLenum to_gl_pixel_type(am_pixel_type t);
static GLenum to_gl_texture_min_filter(am_texture_min_filter f);
static GLenum to_gl_texture_mag_filter(am_texture_mag_filter f);
static GLenum to_gl_texture_wrap(am_texture_wrap w);
static GLenum to_gl_renderbuffer_format(am_renderbuffer_format f);
static GLenum to_gl_framebuffer_attachment(am_framebuffer_attachment a);
static GLenum to_gl_draw_mode(am_draw_mode m);
static GLenum to_gl_element_index_type(am_element_index_type t);

static am_attribute_var_type from_gl_attribute_var_type(GLenum gl_type);
static am_uniform_var_type from_gl_uniform_var_type(GLenum gl_type);
static am_framebuffer_status from_gl_framebuffer_status(GLenum gl_status);

// Per-Fragment Operations

void am_set_blend_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    check_for_errors
}

void am_set_blend_color(float r, float g, float b, float a) {
    check_initialized();
    glBlendColor(r, g, b, a);
    check_for_errors
}

void am_set_blend_equation(am_blend_equation rgb, am_blend_equation alpha) {
    check_initialized();
    GLenum gl_rgb = to_gl_blend_equation(rgb);
    GLenum gl_alpha = to_gl_blend_equation(alpha);
    glBlendEquationSeparate(gl_rgb, gl_alpha);
    check_for_errors
}

void am_set_blend_func(am_blend_sfactor src_rgb, am_blend_dfactor dst_rgb, am_blend_sfactor src_alpha, am_blend_dfactor dst_alpha) {
    check_initialized();
    GLenum gl_srgb = to_gl_blend_sfactor(src_rgb);
    GLenum gl_drgb = to_gl_blend_dfactor(dst_rgb);
    GLenum gl_salpha = to_gl_blend_sfactor(src_alpha);
    GLenum gl_dalpha = to_gl_blend_dfactor(dst_alpha);
    glBlendFuncSeparate(gl_srgb, gl_drgb, gl_salpha, gl_dalpha);
    check_for_errors
}

void am_set_depth_test_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    check_for_errors
}

void am_set_depth_func(am_depth_func func) {
    check_initialized();
    GLenum gl_f = to_gl_depth_func(func);
    glDepthFunc(gl_f);
    check_for_errors
}

void am_set_stencil_test_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_STENCIL_TEST);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    check_for_errors
}

void am_set_stencil_func(am_stencil_face_side face, am_stencil_func func, am_glint ref, am_gluint mask) {
    check_initialized();
    GLenum gl_face = to_gl_stencil_face_side(face);
    GLenum gl_func = to_gl_stencil_func(func);
    glStencilFuncSeparate(gl_face, gl_func, ref, mask);
    check_for_errors
}

void am_set_stencil_op(am_stencil_face_side face, am_stencil_op fail, am_stencil_op zfail, am_stencil_op zpass) {
    check_initialized();
    GLenum gl_face = to_gl_stencil_face_side(face);
    GLenum gl_fail = to_gl_stencil_op(fail);
    GLenum gl_zfail = to_gl_stencil_op(zfail);
    GLenum gl_zpass = to_gl_stencil_op(zpass);
    glStencilOpSeparate(gl_face, gl_fail, gl_zfail, gl_zpass);
}

void am_set_sample_alpha_to_coverage_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    check_for_errors
}

void am_set_sample_coverage_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_SAMPLE_COVERAGE);
    } else {
        glDisable(GL_SAMPLE_COVERAGE);
    }
    check_for_errors
}

void am_set_sample_coverage(float value, bool invert) {
    check_initialized();
    glSampleCoverage(value, invert);
    check_for_errors
}

// Whole Framebuffer Operations

void am_clear_framebuffer(bool clear_color_buf, bool clear_depth_buf, bool clear_stencil_buf) {
    check_initialized();
    glClear((clear_color_buf ? GL_COLOR_BUFFER_BIT : 0) | (clear_depth_buf ? GL_DEPTH_BUFFER_BIT : 0) | (clear_stencil_buf ? GL_STENCIL_BUFFER_BIT : 0));
    check_for_errors
}

void am_set_framebuffer_clear_color(float r, float g, float b, float a) {
    check_initialized();
    glClearColor(r, g, b, a);
    check_for_errors
}

void am_set_framebuffer_clear_depth(float depth) {
    check_initialized();
    glClearDepthf(depth);
    check_for_errors
}

void am_set_framebuffer_clear_stencil_val(am_glint val) {
    check_initialized();
    glClearStencil(val);
    check_for_errors
}

void am_set_framebuffer_color_mask(bool r, bool g, bool b, bool a) {
    check_initialized();
    glColorMask(r, g, b, a);
    check_for_errors
}

void am_set_framebuffer_depth_mask(bool flag) {
    check_initialized();
    glDepthMask(flag);
    check_for_errors
}

void am_set_framebuffer_stencil_mask(am_stencil_face_side face, am_gluint mask) {
    check_initialized();
    GLenum gl_face = to_gl_stencil_face_side(face);
    glStencilMaskSeparate(gl_face, mask);
    check_for_errors
}

// Buffer Objects

am_buffer_id am_create_buffer() {
    check_initialized(0);
    GLuint b;
    glGenBuffers(1, &b);
    check_for_errors
    return b;
}

void am_bind_buffer(am_buffer_target target, am_buffer_id buffer) {
    check_initialized();
    GLenum gl_target = to_gl_buffer_target(target);
    glBindBuffer(gl_target, buffer);
    check_for_errors
}

void am_set_buffer_data(am_buffer_target target, int size, void *data, am_buffer_usage usage) {
    check_initialized();
    GLenum gl_target = to_gl_buffer_target(target);
    GLenum gl_usage = to_gl_buffer_usage(usage);
    glBufferData(gl_target, size, data, gl_usage);
    check_for_errors
}

void am_set_buffer_sub_data(am_buffer_target target, int offset, int size, void *data) {
    check_initialized();
    GLenum gl_target = to_gl_buffer_target(target);
    glBufferSubData(gl_target, offset, size, data);
    check_for_errors
}

void am_delete_buffer(am_buffer_id buffer) {
    check_initialized();
    glDeleteBuffers(1, &buffer);
    check_for_errors
}

// View and Clip

void am_set_depth_range(float near, float far) {
    check_initialized();
    glDepthRangef(near, far);
    check_for_errors
}

void am_set_scissor_test_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    check_for_errors
}

void am_set_scissor(int x, int y, int w, int h) {
    check_initialized();
    glScissor(x, y, w, h);
    check_for_errors
}

void am_set_viewport(int x, int y, int w, int h) {
    check_initialized();
    glViewport(x, y, w, h);
    check_for_errors
}

// Rasterization

void am_set_front_face_winding(am_face_winding mode) {
    check_initialized();
    GLenum gl_mode = to_gl_face_winding(mode);
    glFrontFace(gl_mode);
    check_for_errors
}

void am_set_cull_face_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
    check_for_errors
}

void am_set_cull_face_side(am_cull_face_side face) {
    check_initialized();
    GLenum gl_face = to_gl_cull_face_side(face);
    glCullFace(gl_face);
    check_for_errors
}

void am_set_line_width(float width) {
    check_initialized();
    glLineWidth(width);
    check_for_errors
}

void am_set_polygon_offset_fill_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_POLYGON_OFFSET_FILL);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    check_for_errors
}

void am_set_polygon_offset(float factor, float units) {
    check_initialized();
    glPolygonOffset(factor, units);
    check_for_errors
}

// Dithering

void am_set_dither_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        glEnable(GL_DITHER);
    } else {
        glDisable(GL_DITHER);
    }
    check_for_errors
}

// Programs and Shaders

am_program_id am_create_program() {
    check_initialized(0);
    GLuint p = glCreateProgram();
    check_for_errors
    return p;
}

am_shader_id am_create_vertex_shader() {
    check_initialized(0);
    GLuint s = glCreateShader(GL_VERTEX_SHADER);
    check_for_errors
    return s;
}

am_shader_id am_create_fragment_shader() {
    check_initialized(0);
    GLuint s = glCreateShader(GL_FRAGMENT_SHADER);
    check_for_errors
    return s;
}

void am_set_shader_source(am_shader_id shader, const char *src) {
    check_initialized();
    glShaderSource(shader, 1, &src, NULL);
    check_for_errors
}

bool am_compile_shader(am_shader_id shader) {
    check_initialized(false);
    GLint compiled;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    check_for_errors
    return compiled;
}

const char *am_get_shader_info_log(am_shader_id shader) {
    check_initialized("gl not initialized");
    GLint len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        char* msg = (char*)malloc(sizeof(char) * len);
        glGetShaderInfoLog(shader, len, NULL, msg);
        return msg;
    } else {
        const char *cmsg = "unknown error";
        char* msg = (char*)malloc(strlen(cmsg) + 1);
        strcpy(msg, cmsg);
        return msg;
    }
}

void am_attach_shader(am_program_id program, am_shader_id shader) {
    check_initialized();
    glAttachShader(program, shader);
    check_for_errors
}

bool am_link_program(am_program_id program) {
    check_initialized(false);
    GLint linked;
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
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
    check_for_errors
    return val;
}

int am_get_program_active_uniforms(am_program_id program) {
    check_initialized(0);
    GLint val;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &val);
    check_for_errors
    return val;
}

bool am_validate_program(am_program_id program) {
    check_initialized(false);
    glValidateProgram(program);
    GLint valid;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &valid);
    return valid;
}

void am_use_program(am_program_id program) {
    check_initialized();
    glUseProgram(program);
    check_for_errors
}

void am_detach_shader(am_program_id program, am_shader_id shader) {
    check_initialized();
    glDetachShader(program, shader);
    check_for_errors
}

void am_delete_shader(am_shader_id shader) {
    check_initialized();
    glDeleteShader(shader);
    check_for_errors
}

void am_delete_program(am_program_id program) {
    check_initialized();
    glDeleteProgram(program);
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
        default: am_abort("INTERNAL ERROR: unknown am_attribute_client_type %d", t); return 0;
    }
}

void am_set_attribute_array_enabled(am_gluint location, bool enabled) {
    check_initialized();
    if (enabled) {
        glEnableVertexAttribArray(location);
    } else {
        glDisableVertexAttribArray(location);
    }
    check_for_errors
}

am_gluint am_get_attribute_location(am_program_id program, const char *name) {
    check_initialized(0);
    am_gluint l = glGetAttribLocation(program, name);
    check_for_errors
    return l;
}

am_gluint am_get_uniform_location(am_program_id program, const char *name) {
    check_initialized(0);
    am_gluint l = glGetUniformLocation(program, name);
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
    check_for_errors
    *name = (char*)malloc(strlen(gl_name) + 1);
    strcpy(*name, gl_name);
    *size = gl_size;
    *type = from_gl_uniform_var_type(gl_type);
}

void am_set_uniform1f(am_gluint location, float value) {
    check_initialized();
    glUniform1fv(location, 1, &value);
    check_for_errors
}

void am_set_uniform2f(am_gluint location, const float *value) {
    check_initialized();
    glUniform2fv(location, 1, value);
    check_for_errors
}

void am_set_uniform3f(am_gluint location, const float *value) {
    check_initialized();
    glUniform3fv(location, 1, value);
    check_for_errors
}

void am_set_uniform4f(am_gluint location, const float *value) {
    check_initialized();
    glUniform4fv(location, 1, value);
    check_for_errors
}

void am_set_uniform1i(am_gluint location, am_glint value) {
    check_initialized();
    glUniform1iv(location, 1, &value);
    check_for_errors
}

void am_set_uniform2i(am_gluint location, const am_glint *value) {
    check_initialized();
    glUniform2iv(location, 1, value);
    check_for_errors
}

void am_set_uniform3i(am_gluint location, const am_glint *value) {
    check_initialized();
    glUniform3iv(location, 1, value);
    check_for_errors
}

void am_set_uniform4i(am_gluint location, const am_glint *value) {
    check_initialized();
    glUniform4iv(location, 1, value);
    check_for_errors
}

void am_set_uniform_mat2(am_gluint location, const float *value) {
    check_initialized();
    glUniformMatrix2fv(location, 1, GL_FALSE, value);
    check_for_errors
}

void am_set_uniform_mat3(am_gluint location, const float *value) {
    check_initialized();
    glUniformMatrix3fv(location, 1, GL_FALSE, value);
    check_for_errors
}

void am_set_uniform_mat4(am_gluint location, const float *value) {
    check_initialized();
    glUniformMatrix4fv(location, 1, GL_FALSE, value);
    check_for_errors
}

void am_set_attribute1f(am_gluint location, const float value) {
    check_initialized();
    glVertexAttrib1f(location, value);
    check_for_errors
}

void am_set_attribute2f(am_gluint location, const float *value) {
    check_initialized();
    glVertexAttrib2fv(location, value);
    check_for_errors
}

void am_set_attribute3f(am_gluint location, const float *value) {
    check_initialized();
    glVertexAttrib3fv(location, value);
    check_for_errors
}

void am_set_attribute4f(am_gluint location, const float *value) {
    check_initialized();
    glVertexAttrib4fv(location, value);
    check_for_errors
}

void am_set_attribute_pointer(am_gluint location, int size, am_attribute_client_type type, bool normalized, int stride, int offset) {
    check_initialized();
    GLenum gl_type = to_gl_attr_client_type(type);
    glVertexAttribPointer(location, size, gl_type, normalized, stride, (void*)((uintptr_t)offset));
    check_for_errors
}

// Texture Objects

void am_set_active_texture_unit(int texture_unit) {
    check_initialized();
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    check_for_errors
}

am_texture_id am_create_texture() {
    check_initialized(0);
    GLuint tex;
    glGenTextures(1, &tex);
    check_for_errors
    return tex;
}

void am_delete_texture(am_texture_id texture) {
    check_initialized();
    glDeleteTextures(1, &texture);
    check_for_errors
}

void am_bind_texture(am_texture_bind_target target, am_texture_id texture) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    glBindTexture(gl_target, texture);
    check_for_errors
}

void am_copy_texture_image_2d(am_texture_copy_target target, int level, am_texture_format format, int x, int y, int w, int h) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    GLenum gl_format = to_gl_texture_format(format);
    glCopyTexImage2D(gl_target, level, gl_format, x, y, w, h, 0);
    check_for_errors
}

void am_copy_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int x, int y, int w, int h) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    glCopyTexSubImage2D(gl_target, level, xoffset, yoffset, x, y, w, h);
    check_for_errors
}

void am_generate_mipmap(am_texture_bind_target target) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    glGenerateMipmap(gl_target);
    check_for_errors
}

void am_set_texture_image_2d(am_texture_copy_target target, int level, am_texture_format format, int w, int h, am_pixel_type type, void *data) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    GLenum gl_format = to_gl_texture_format(format);
    GLenum gl_type = to_gl_pixel_type(type);
    glTexImage2D(gl_target, level, gl_format, w, h, 0, gl_format, gl_type, data);
    check_for_errors
}

void am_set_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int w, int h, am_texture_format format, am_pixel_type type, void *data) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    GLenum gl_format = to_gl_texture_format(format);
    GLenum gl_type = to_gl_pixel_type(type);
    glTexSubImage2D(gl_target, level, xoffset, yoffset, w, h, gl_format, gl_type, data);
    check_for_errors
}

void am_set_texture_min_filter(am_texture_bind_target target, am_texture_min_filter filter) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    GLenum gl_filter = to_gl_texture_min_filter(filter);
    glTexParameteri(gl_target, GL_TEXTURE_MIN_FILTER, gl_filter);
    check_for_errors
}

void am_set_texture_mag_filter(am_texture_bind_target target, am_texture_mag_filter filter) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    GLenum gl_filter = to_gl_texture_mag_filter(filter);
    glTexParameteri(gl_target, GL_TEXTURE_MAG_FILTER, gl_filter);
    check_for_errors
}

void am_set_texture_wrap(am_texture_bind_target target, am_texture_wrap s_wrap, am_texture_wrap t_wrap) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    GLenum gl_swrap = to_gl_texture_wrap(s_wrap);
    GLenum gl_twrap = to_gl_texture_wrap(t_wrap);
    glTexParameteri(gl_target, GL_TEXTURE_WRAP_S, gl_swrap);
    check_for_errors
    glTexParameteri(gl_target, GL_TEXTURE_WRAP_T, gl_twrap);
    check_for_errors
}

// Renderbuffer Objects

am_renderbuffer_id am_create_renderbuffer() {
    check_initialized(0);
    GLuint rb;
    glGenRenderbuffers(1, &rb);
    check_for_errors
    return rb;
}

void am_delete_renderbuffer(am_renderbuffer_id rb) {
    check_initialized();
    glDeleteRenderbuffers(1, &rb);
    check_for_errors
}

void am_bind_renderbuffer(am_renderbuffer_id rb) {
    check_initialized();
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    check_for_errors
}

void am_set_renderbuffer_storage(am_renderbuffer_format format, int w, int h) {
    check_initialized();
    GLenum gl_format = to_gl_renderbuffer_format(format);
    glRenderbufferStorage(GL_RENDERBUFFER, gl_format, w, h);
    check_for_errors
}

// Read Back Pixels 

void am_read_pixels(int x, int y, int w, int h, void *data) {
    check_initialized();
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
    check_for_errors
}

// Framebuffer Objects

am_framebuffer_id am_create_framebuffer() {
    check_initialized(0);
    GLuint fb;
    glGenFramebuffers(1, &fb);
    check_for_errors
    return fb;
}

void am_delete_framebuffer(am_framebuffer_id fb) {
    check_initialized();
    glDeleteFramebuffers(1, &fb);
    check_for_errors
}

void am_bind_framebuffer(am_framebuffer_id fb) {
    check_initialized();
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    check_for_errors
}

am_framebuffer_status am_check_framebuffer_status() {
    check_initialized(AM_FRAMEBUFFER_STATUS_UNKNOWN);
    GLenum gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    return from_gl_framebuffer_status(gl_status);
}

void am_set_framebuffer_renderbuffer(am_framebuffer_attachment attachment, am_renderbuffer_id rb) {
    check_initialized();
    GLenum gl_attachment = to_gl_framebuffer_attachment(attachment);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, gl_attachment, GL_RENDERBUFFER, rb);
    check_for_errors
}

void am_set_framebuffer_texture2d(am_framebuffer_attachment attachment, am_texture_copy_target target, am_texture_id texture) {
    check_initialized();
    GLenum gl_attachment = to_gl_framebuffer_attachment(attachment);
    GLenum gl_target = to_gl_texture_copy_target(target);
    glFramebufferTexture2D(GL_FRAMEBUFFER, gl_attachment, gl_target, texture, 0);
    check_for_errors
}

// Writing to the Draw Buffer

void am_draw_arrays(am_draw_mode mode, int first, int count) {
    check_initialized();
    GLenum gl_mode = to_gl_draw_mode(mode);
    glDrawArrays(gl_mode, first, count);
    check_for_errors
}

void am_draw_elements(am_draw_mode mode, int count, am_element_index_type type, int offset) {
    check_initialized();
    GLenum gl_mode = to_gl_draw_mode(mode);
    GLenum gl_type = to_gl_element_index_type(type);
    glDrawElements(gl_mode, count, gl_type, (void*)((uintptr_t)offset));
    check_for_errors
}

static GLenum to_gl_blend_equation(am_blend_equation eq) {
    switch (eq) {
        case AM_BLEND_EQUATION_ADD: return GL_FUNC_ADD;
        case AM_BLEND_EQUATION_SUBTRACT: return GL_FUNC_SUBTRACT;
        case AM_BLEND_EQUATION_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
        default: am_abort("INTERNAL ERROR: unknown am_blend_equation %d", eq); return 0;
    }
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
        default: am_abort("INTERNAL ERROR: unknown am_blend_sfactor %d", f); return 0;
    }
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
        default: am_abort("INTERNAL ERROR: unknown am_blend_dfactor %d", f); return 0;
    }
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
        default: am_abort("INTERNAL ERROR: unknown am_depth_func %d", f); return 0;
    }
}

static GLenum to_gl_stencil_face_side(am_stencil_face_side fs) {
    switch (fs) {
        case AM_STENCIL_FACE_FRONT: return GL_FRONT;
        case AM_STENCIL_FACE_BACK: return GL_BACK;
        case AM_STENCIL_FACE_FRONT_AND_BACK: return GL_FRONT_AND_BACK;
        default: am_abort("INTERNAL ERROR: unknown am_stencil_face_side %d", fs); return 0;
    }
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
        default: am_abort("INTERNAL ERROR: unknown am_stencil_func %d", f); return 0;
    }
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
        default: am_abort("INTERNAL ERROR: unknown am_stencil_op %d", op); return 0;
    }
}

static GLenum to_gl_buffer_target(am_buffer_target t) {
    switch (t) {
        case AM_ARRAY_BUFFER: return GL_ARRAY_BUFFER;
        case AM_ELEMENT_ARRAY_BUFFER: return GL_ELEMENT_ARRAY_BUFFER;
        default: am_abort("INTERNAL ERROR: unknown am_buffer_target %d", t); return 0;
    }
}

static GLenum to_gl_buffer_usage(am_buffer_usage u) {
    switch (u) {
        case AM_BUFFER_USAGE_STREAM_DRAW: return GL_STREAM_DRAW;
        case AM_BUFFER_USAGE_STATIC_DRAW: return GL_STATIC_DRAW;
        case AM_BUFFER_USAGE_DYNAMIC_DRAW: return GL_DYNAMIC_DRAW;
        default: am_abort("INTERNAL ERROR: unknown am_buffer_usage %d", u); return 0;
    }
}

static GLenum to_gl_face_winding(am_face_winding w) {
    switch (w) {
        case AM_FACE_WIND_CW: return GL_CW;
        case AM_FACE_WIND_CCW: return GL_CCW;
        default: am_abort("INTERNAL ERROR: unknown am_face_winding %d", w); return 0;
    }
}

static GLenum to_gl_cull_face_side(am_cull_face_side fs) {
    switch (fs) {
        case AM_CULL_FACE_FRONT: return GL_FRONT;
        case AM_CULL_FACE_BACK: return GL_BACK;
        case AM_CULL_FACE_FRONT_AND_BACK: return GL_FRONT_AND_BACK;
        default: am_abort("INTERNAL ERROR: unknown am_cull_face_side %d", fs); return 0;
    }
}

static GLenum to_gl_attr_client_type(am_attribute_client_type t) {
    switch (t) {
        case AM_ATTRIBUTE_CLIENT_TYPE_BYTE: return GL_BYTE;
        case AM_ATTRIBUTE_CLIENT_TYPE_SHORT: return GL_SHORT;
        case AM_ATTRIBUTE_CLIENT_TYPE_UBYTE: return GL_UNSIGNED_BYTE;
        case AM_ATTRIBUTE_CLIENT_TYPE_USHORT: return GL_UNSIGNED_SHORT;
        case AM_ATTRIBUTE_CLIENT_TYPE_FLOAT: return GL_FLOAT;
        default: am_abort("INTERNAL ERROR: unknown am_attribute_client_type %d", t); return 0;
    }
}

static GLenum to_gl_texture_bind_target(am_texture_bind_target t) {
    switch (t) {
        case AM_TEXTURE_BIND_TARGET_2D: return GL_TEXTURE_2D;
        case AM_TEXTURE_BIND_TARGET_CUBE_MAP: return GL_TEXTURE_CUBE_MAP;
        default: am_abort("INTERNAL ERROR: unknown am_texture_bind_target %d", t); return 0;
    }
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
        default: am_abort("INTERNAL ERROR: unknown am_texture_copy_target %d", t); return 0;
    }
}

static GLenum to_gl_texture_format(am_texture_format f) {
    switch (f) {
        case AM_TEXTURE_FORMAT_ALPHA: return GL_ALPHA;
        case AM_TEXTURE_FORMAT_LUMINANCE: return GL_LUMINANCE;
        case AM_TEXTURE_FORMAT_LUMINANCE_ALPHA: return GL_LUMINANCE_ALPHA;
        case AM_TEXTURE_FORMAT_RGB: return GL_RGB;
        case AM_TEXTURE_FORMAT_RGBA: return GL_RGBA;
        default: am_abort("INTERNAL ERROR: unknown am_texture_format %d", f); return 0;
    }
}

static GLenum to_gl_pixel_type(am_pixel_type t) {
    switch (t) {
        case AM_PIXEL_FORMAT_UBYTE: return GL_UNSIGNED_BYTE;
        case AM_PIXEL_FORMAT_USHORT_5_6_5: return GL_UNSIGNED_SHORT_5_6_5;
        case AM_PIXEL_FORMAT_USHORT_4_4_4_4: return GL_UNSIGNED_SHORT_4_4_4_4;
        case AM_PIXEL_FORMAT_USHORT_5_5_5_1: return GL_UNSIGNED_SHORT_5_5_5_1;
        default: am_abort("INTERNAL ERROR: unknown am_pixel_type %d", t); return 0;
    }
}

static GLenum to_gl_texture_min_filter(am_texture_min_filter f) {
    switch (f) {
        case AM_MIN_FILTER_NEAREST: return GL_NEAREST;
        case AM_MIN_FILTER_LINEAR: return GL_LINEAR;
        case AM_MIN_FILTER_NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
        case AM_MIN_FILTER_LINEAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
        case AM_MIN_FILTER_NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
        case AM_MIN_FILTER_LINEAR_MIPMAP_LINEAR: return GL_LINEAR_MIPMAP_LINEAR;
        default: am_abort("INTERNAL ERROR: unknown am_texture_min_filter %d", f); return 0;
    }
}

static GLenum to_gl_texture_mag_filter(am_texture_mag_filter f) {
    switch (f) {
        case AM_MAG_FILTER_NEAREST: return GL_NEAREST;
        case AM_MAG_FILTER_LINEAR: return GL_LINEAR;
        default: am_abort("INTERNAL ERROR: unknown am_texture_mag_filter %d", f); return 0;
    }
}

static GLenum to_gl_texture_wrap(am_texture_wrap w) {
    switch (w) {
        case AM_TEXTURE_WRAP_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
        case AM_TEXTURE_WRAP_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
        case AM_TEXTURE_WRAP_REPEAT: return GL_REPEAT;
        default: am_abort("INTERNAL ERROR: unknown am_texture_wrap %d", w); return 0;
    }
}

static GLenum to_gl_renderbuffer_format(am_renderbuffer_format f) {
    switch (f) {
        case AM_RENDERBUFFER_FORMAT_RGBA4: return GL_RGBA4;
        case AM_RENDERBUFFER_FORMAT_RGB565: return GL_RGB565;
        case AM_RENDERBUFFER_FORMAT_RGB5_A1: return GL_RGB5_A1;
        case AM_RENDERBUFFER_FORMAT_DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT16;
        case AM_RENDERBUFFER_FORMAT_STENCIL_INDEX8: return GL_STENCIL_INDEX8;
        default: am_abort("INTERNAL ERROR: unknown am_renderbuffer_format %d", f); return 0;
    }
}

static GLenum to_gl_framebuffer_attachment(am_framebuffer_attachment a) {
    switch (a) {
        case AM_FRAMEBUFFER_COLOR_ATTACHMENT0: return GL_COLOR_ATTACHMENT0;
        case AM_FRAMEBUFFER_DEPTH_ATTACHMENT: return GL_DEPTH_ATTACHMENT;
        case AM_FRAMEBUFFER_STENCIL_ATTACHMENT: return GL_STENCIL_ATTACHMENT;
        default: am_abort("INTERNAL ERROR: unknown am_framebuffer_attachment %d", a); return 0;
    }
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
        default: am_abort("INTERNAL ERROR: unknown am_draw_mode %d", m); return 0;
    }
}

static GLenum to_gl_element_index_type(am_element_index_type t) {
    switch (t) {
        case AM_ELEMENT_TYPE_UBYTE: return GL_UNSIGNED_BYTE;
        case AM_ELEMENT_TYPE_USHORT: return GL_UNSIGNED_SHORT;
        default: am_abort("INTERNAL ERROR: unknown am_element_index_type %d", t); return 0;
    }
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
#ifdef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return AM_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: return AM_FRAMEBUFFER_STATUS_INCOMPLETE_DIMENSIONS;
#endif
#ifdef GL_FRAMEBUFFER_MISSING_ATTACHMENT
        case GL_FRAMEBUFFER_MISSING_ATTACHMENT: return AM_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT;
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
            am_report_error("OpenGL error at %s:%d %s: %s", file, line, func, str);
        }
    }
}
