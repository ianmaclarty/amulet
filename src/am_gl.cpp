/* OpenGL ES 2.0 reference: http://www.khronos.org/opengles/sdk/docs/man/ */
/* OpenGL ES 2.0 quick reference card: https://www.khronos.org/opengles/sdk/docs/reference_cards/OpenGL-ES-2_0-Reference-card.pdf */
/* WebGL 1.0 quick reference: http://www.khronos.org/files/webgl/webgl-reference-card-1_0.pdf */

#include "amulet.h"

#ifndef AM_USE_METAL

#if defined(AM_BACKEND_SDL)
    #define GL_GLEXT_PROTOTYPES
    #include <SDL_opengl.h>
#elif defined(AM_BACKEND_EMSCRIPTEN)
    #include <GLES2/gl2.h>
#elif defined(AM_BACKEND_IOS)
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
#elif defined(AM_BACKEND_ANDROID)
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
#else
    #error unknown backend
#endif

#ifdef AM_NEED_GL_FUNC_PTRS
#define AM_GLPROC(ret,func,params) ret (APIENTRY *func##_ptr) params = NULL;
#include "am_glfuncs.h"
#undef AM_GLPROC
#endif

#ifdef AM_NEED_GL_FUNC_PTRS
#define GLFUNC(func) func##_ptr
#else
#define GLFUNC(func) func
#endif

#if defined(AM_USE_GLSL_OPTIMIZER)
#include "glsl_optimizer.h"
#endif

#if defined(AM_ANGLE_TRANSLATE_GL)
#include "GLSLANG/ShaderLang.h"
#endif

#ifdef AM_MSVC
#define __func__ "unknown"
#endif

#define check_for_errors { if (am_conf_check_gl_errors) check_glerror(__FILE__, __LINE__, __func__); }

//#if defined(AM_ANDROID)
//#define log_gl(fmt, ...) {am_log0(fmt "\n", __VA_ARGS__);}
//#define log_gl_ptr(ptr, len)
//#else
#define log_gl(fmt, ...) {if (am_conf_log_gl_calls && am_conf_log_gl_frames > 0) {fprintf(gl_log_file, fmt "\n", __VA_ARGS__);fflush(gl_log_file);}}
#define log_gl_ptr(ptr, len) {if (am_conf_log_gl_calls && am_conf_log_gl_frames > 0) {print_ptr(gl_log_file, (void*)ptr, len);fflush(gl_log_file);}}
//#endif

static FILE *gl_log_file = NULL;

static void print_ptr(FILE* f, void* p, int len);

#define check_initialized(...) {if (!gl_initialized) {am_log1("%s:%d: attempt to call %s without a valid gl context", __FILE__, __LINE__, __func__); return __VA_ARGS__;}}

#define ATTR_NAME_SIZE 100
#define UNI_NAME_SIZE 100

#if defined(AM_USE_GLSL_OPTIMIZER)
static void init_glslopt();
static void destroy_glslopt();
static void glslopt_translate_shader(am_shader_type type,
    const char *src, char **objcode, char **errmsg, glslopt_shader **shader);
#endif

#if defined(AM_ANGLE_TRANSLATE_GL)
static void init_angle();
static void destroy_angle();
static void angle_translate_shader(am_shader_type type,
    const char *src, char **objcode, char **errmsg);
#endif

int am_max_combined_texture_image_units = 0;
int am_max_cube_map_texture_size = 0;
int am_max_fragment_uniform_vectors = 0;
int am_max_renderbuffer_size = 0;
int am_max_texture_image_units = 0;
int am_max_texture_size = 0;
int am_max_varying_vectors = 0;
int am_max_vertex_attribs = 0;
int am_max_vertex_texture_image_units = 0;
int am_max_vertex_uniform_vectors = 0;
int am_frame_draw_calls = 0;
int am_frame_use_program_calls = 0;

static bool gl_initialized = false;

static void check_glerror(const char *file, int line, const char *func);

static void reset_gl() {
    log_gl("glPixelStorei(GL_PACK_ALIGNMENT, %d);", 1);
    GLFUNC(glPixelStorei)(GL_PACK_ALIGNMENT, 1);
    check_for_errors
    log_gl("glPixelStorei(GL_UNPACK_ALIGNMENT, %d);", 1);
    GLFUNC(glPixelStorei)(GL_UNPACK_ALIGNMENT, 1);
    check_for_errors
    log_gl("glHint(GL_GENERATE_MIPMAP_HINT, %s);", "GL_NICEST");
    GLFUNC(glHint)(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    check_for_errors

    // initialize global gl state so it corresponds with the initial
    // value of am_render_state.

    am_set_depth_test_enabled(false);
    am_set_depth_func(AM_DEPTH_FUNC_ALWAYS);

    am_set_cull_face_enabled(false);
    am_set_front_face_winding(AM_FACE_WIND_CCW);
    am_set_cull_face_side(AM_CULL_FACE_BACK);

    am_set_blend_enabled(false);
    am_set_blend_equation(AM_BLEND_EQUATION_ADD, AM_BLEND_EQUATION_ADD);
    am_set_blend_func(AM_BLEND_SFACTOR_SRC_ALPHA, AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA,
        AM_BLEND_SFACTOR_SRC_ALPHA, AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA);
    am_set_blend_color(1.0f, 1.0f, 1.0f, 1.0f);
    
    // need to explicitly enable point sprites on desktop gl
    // (they are always enabled in opengles)
#if defined(AM_GLPROFILE_DESKTOP)
    if (!am_conf_d3dangle) {
        log_gl("glEnable(%s);", "GL_POINT_SPRITE");
        GLFUNC(glEnable)(GL_POINT_SPRITE);
        log_gl("glEnable(%s);", "GL_VERTEX_PROGRAM_POINT_SIZE");
        GLFUNC(glEnable)(GL_VERTEX_PROGRAM_POINT_SIZE);
    }
#endif
}

static void log_gl_prog_header() {
    if (!am_conf_log_gl_calls) return;
    fprintf(gl_log_file, "%s\n", "#define SDL_MAIN_HANDLED 1");
    fprintf(gl_log_file, "%s\n", "#include \"SDL.h\"");
    fprintf(gl_log_file, "%s\n", "#define GL_GLEXT_PROTOTYPES");
    fprintf(gl_log_file, "%s\n", "#include \"SDL_opengl.h\"");
    fprintf(gl_log_file, "%s\n", "#include <map>");
    fprintf(gl_log_file, "%s\n", "#include <assert.h>");
    fprintf(gl_log_file, "%s\n", "int main( int argc, char *argv[] )");
    fprintf(gl_log_file, "%s\n", "{");
    fprintf(gl_log_file, "%s\n", "    std::map<uintptr_t, void*> ptr;");
    fprintf(gl_log_file, "%s\n", "    std::map<GLuint, GLuint> prog;");
    fprintf(gl_log_file, "%s\n", "    std::map<GLuint, GLuint> tex;");
    fprintf(gl_log_file, "%s\n", "    std::map<GLuint, GLuint> buf;");
    fprintf(gl_log_file, "%s\n", "    std::map<GLuint, GLuint> shader;");
    fprintf(gl_log_file, "%s\n", "    std::map<GLuint, GLuint> fbuf;");
    fprintf(gl_log_file, "%s\n", "    std::map<GLuint, GLuint> rbuf;");
    fprintf(gl_log_file, "%s\n", "    ptr[0] = NULL;");
    fprintf(gl_log_file, "%s\n", "    prog[0] = 0;");
    fprintf(gl_log_file, "%s\n", "    tex[0] = 0;");
    fprintf(gl_log_file, "%s\n", "    buf[0] = 0;");
    fprintf(gl_log_file, "%s\n", "    shader[0] = 0;");
    fprintf(gl_log_file, "%s\n", "    fbuf[0] = 0;");
    fprintf(gl_log_file, "%s\n", "    rbuf[0] = 0;");
    fprintf(gl_log_file, "%s\n", "    SDL_Init(SDL_INIT_VIDEO);");
    fprintf(gl_log_file, "%s\n", "    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);");
    fprintf(gl_log_file, "%s\n", "    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);");
    fprintf(gl_log_file, "%s\n", "    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);");
    fprintf(gl_log_file, "%s\n", "    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);");
    fprintf(gl_log_file, "%s\n", "    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);");
    fprintf(gl_log_file, "%s\n", "    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);");
    fprintf(gl_log_file, "%s\n", "    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);");
    fprintf(gl_log_file, "%s\n", "    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);");
    fprintf(gl_log_file, "%s\n", "    SDL_Window *win = SDL_CreateWindow(\"\", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL);");
    fprintf(gl_log_file, "%s\n", "    SDL_GL_MakeCurrent(win, SDL_GL_CreateContext(win));");
    fprintf(gl_log_file, "\n\n");
}

static void log_gl_prog_footer() {
    if (!am_conf_log_gl_calls) return;
    fprintf(gl_log_file, "%s\n", "    SDL_Quit();");
    fprintf(gl_log_file, "%s\n", "}");
}

void am_init_gl() {
    if (gl_initialized) {
        am_log0("INTERNAL ERROR: %s", "gl already initialized");
        return;
    }
    gl_initialized = true;

    check_for_errors

    GLint pval; 
    GLFUNC(glGetIntegerv)(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &pval);
    check_for_errors
    am_max_combined_texture_image_units = pval;
    GLFUNC(glGetIntegerv)(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &pval);
    check_for_errors
    am_max_cube_map_texture_size = pval;
#if defined(AM_GLPROFILE_DESKTOP)
    if (am_conf_d3dangle) {
        GLFUNC(glGetIntegerv)(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &pval);
        check_for_errors
        am_max_fragment_uniform_vectors = pval;
    } else {
        GLFUNC(glGetIntegerv)(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &pval);
        check_for_errors
        am_max_fragment_uniform_vectors = pval/4;
    }
#else
    GLFUNC(glGetIntegerv)(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &pval);
    check_for_errors
    am_max_fragment_uniform_vectors = pval;
#endif
    GLFUNC(glGetIntegerv)(GL_MAX_RENDERBUFFER_SIZE, &pval);
    check_for_errors
    am_max_renderbuffer_size = pval;
    GLFUNC(glGetIntegerv)(GL_MAX_TEXTURE_IMAGE_UNITS, &pval);
    check_for_errors
    am_max_texture_image_units = pval;
    GLFUNC(glGetIntegerv)(GL_MAX_TEXTURE_SIZE, &pval);
    check_for_errors
    am_max_texture_size = pval;
#if defined(AM_GLPROFILE_DESKTOP)
    if (am_conf_d3dangle) {
        GLFUNC(glGetIntegerv)(GL_MAX_VARYING_VECTORS, &pval);
        check_for_errors
        am_max_varying_vectors = pval;
    } else {
        GLFUNC(glGetIntegerv)(GL_MAX_VARYING_FLOATS, &pval);
        check_for_errors
        am_max_varying_vectors = pval/4;
    }
#else
    GLFUNC(glGetIntegerv)(GL_MAX_VARYING_VECTORS, &pval);
    check_for_errors
    am_max_varying_vectors = pval;
#endif
    GLFUNC(glGetIntegerv)(GL_MAX_VERTEX_ATTRIBS, &pval);
    check_for_errors
    am_max_vertex_attribs = pval;
    GLFUNC(glGetIntegerv)(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &pval);
    check_for_errors
    am_max_vertex_texture_image_units = pval;
#if defined(AM_GLPROFILE_DESKTOP)
    if (am_conf_d3dangle) {
        GLFUNC(glGetIntegerv)(GL_MAX_VERTEX_UNIFORM_VECTORS, &pval);
        check_for_errors
        am_max_vertex_uniform_vectors = pval;
    } else {
        GLFUNC(glGetIntegerv)(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &pval);
        check_for_errors
        am_max_vertex_uniform_vectors = pval/4;
    }
#else
    GLFUNC(glGetIntegerv)(GL_MAX_VERTEX_UNIFORM_VECTORS, &pval);
    check_for_errors
    am_max_vertex_uniform_vectors = pval;
#endif

    // initialize glsl optimizer if using
#if defined(AM_USE_GLSL_OPTIMIZER)
    init_glslopt();
#endif

    // initialize angle if using
#if defined(AM_ANGLE_TRANSLATE_GL)
    init_angle();
#endif

    if (am_conf_log_gl_calls) {
        gl_log_file = am_fopen("amulet_gllog.txt", "w");
        if (gl_log_file == NULL) {
            fprintf(stderr, "ERROR: unable to open amulet_gllog.txt for writing\n");
            exit(1);
        }
    }
    log_gl_prog_header();
    reset_gl();
}

void am_close_gllog() {
    if (am_conf_log_gl_calls) {
        fclose(gl_log_file);
    }
}

void am_destroy_gl() {
    if (!gl_initialized) return;
    gl_initialized = false;
#if defined(AM_USE_GLSL_OPTIMIZER)
    destroy_glslopt();
#endif
#if defined(AM_ANGLE_TRANSLATE_GL)
    destroy_angle();
#endif
    log_gl_prog_footer();
}

bool am_gl_is_initialized() {
    return gl_initialized;
}

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
        log_gl("glEnable(%s);", "GL_BLEND");
        GLFUNC(glEnable)(GL_BLEND);
    } else {
        log_gl("glDisable(%s);", "GL_BLEND");
        GLFUNC(glDisable)(GL_BLEND);
    }
    check_for_errors
}

void am_set_blend_color(float r, float g, float b, float a) {
    check_initialized();
    log_gl("glBlendColor(%0.2f, %0.2f, %0.2f, %0.2f);", r, g, b, a);
    GLFUNC(glBlendColor)(r, g, b, a);
    check_for_errors
}

void am_set_blend_equation(am_blend_equation rgb, am_blend_equation alpha) {
    check_initialized();
    GLenum gl_rgb = to_gl_blend_equation(rgb);
    GLenum gl_alpha = to_gl_blend_equation(alpha);
    if (gl_rgb == gl_alpha) {
        log_gl("glBlendEquation(%s);", gl_blend_equation_str(gl_rgb));
        GLFUNC(glBlendEquation)(gl_rgb);
    } else {
        log_gl("glBlendEquationSeparate(%s, %s);", gl_blend_equation_str(gl_rgb), gl_blend_equation_str(gl_alpha));
        GLFUNC(glBlendEquationSeparate)(gl_rgb, gl_alpha);
    }
    check_for_errors
}

void am_set_blend_func(am_blend_sfactor src_rgb, am_blend_dfactor dst_rgb, am_blend_sfactor src_alpha, am_blend_dfactor dst_alpha) {
    check_initialized();
    GLenum gl_srgb = to_gl_blend_sfactor(src_rgb);
    GLenum gl_drgb = to_gl_blend_dfactor(dst_rgb);
    GLenum gl_salpha = to_gl_blend_sfactor(src_alpha);
    GLenum gl_dalpha = to_gl_blend_dfactor(dst_alpha);
    if (gl_srgb == gl_salpha && gl_drgb == gl_dalpha) {
        log_gl("glBlendFunc(%s, %s);",
            gl_blend_factor_str(gl_srgb), 
            gl_blend_factor_str(gl_drgb));
        GLFUNC(glBlendFunc)(gl_srgb, gl_drgb);
    } else {
        log_gl("glBlendFuncSeparate(%s, %s, %s, %s);",
            gl_blend_factor_str(gl_srgb), 
            gl_blend_factor_str(gl_drgb), 
            gl_blend_factor_str(gl_salpha), 
            gl_blend_factor_str(gl_dalpha));
        GLFUNC(glBlendFuncSeparate)(gl_srgb, gl_drgb, gl_salpha, gl_dalpha);
    }
    check_for_errors
}

void am_set_depth_test_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        log_gl("glEnable(%s);", "GL_DEPTH_TEST");
        GLFUNC(glEnable)(GL_DEPTH_TEST);
    } else {
        log_gl("glDisable(%s);", "GL_DEPTH_TEST");
        GLFUNC(glDisable)(GL_DEPTH_TEST);
    }
    check_for_errors
}

void am_set_depth_func(am_depth_func func) {
    check_initialized();
    GLenum gl_f = to_gl_depth_func(func);
    log_gl("glDepthFunc(%s);", gl_depth_func_str(gl_f));
    GLFUNC(glDepthFunc)(gl_f);
    check_for_errors
}

void am_set_stencil_test_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        log_gl("glEnable(%s);", "GL_STENCIL_TEST");
        GLFUNC(glEnable)(GL_STENCIL_TEST);
    } else {
        log_gl("glDisable(%s);", "GL_STENCIL_TEST");
        GLFUNC(glDisable)(GL_STENCIL_TEST);
    }
    check_for_errors
}

void am_set_stencil_func(am_glint ref, am_gluint mask, am_stencil_func func_front, am_stencil_func func_back) {
    check_initialized();
    GLenum gl_face = GL_FRONT;
    GLenum gl_func = to_gl_stencil_func(func_front);
    log_gl("glStencilFuncSeparate(%s, %s, %d, %u);",
        gl_face_side_str(gl_face), 
        gl_stencil_func_str(gl_func), ref, mask);
    GLFUNC(glStencilFuncSeparate)(gl_face, gl_func, ref, mask);
    gl_face = GL_BACK;
    gl_func = to_gl_stencil_func(func_back);
    log_gl("glStencilFuncSeparate(%s, %s, %d, %u);",
        gl_face_side_str(gl_face), 
        gl_stencil_func_str(gl_func), ref, mask);
    GLFUNC(glStencilFuncSeparate)(gl_face, gl_func, ref, mask);
    check_for_errors
}

void am_set_stencil_op(am_stencil_face_side face, am_stencil_op fail, am_stencil_op zfail, am_stencil_op zpass) {
    check_initialized();
    GLenum gl_face = to_gl_stencil_face_side(face);
    GLenum gl_fail = to_gl_stencil_op(fail);
    GLenum gl_zfail = to_gl_stencil_op(zfail);
    GLenum gl_zpass = to_gl_stencil_op(zpass);
    log_gl("glStencilOpSeparate(%s, %s, %s, %s);",
        gl_face_side_str(gl_face), 
        gl_stencil_op_str(gl_fail),
        gl_stencil_op_str(gl_zfail),
        gl_stencil_op_str(gl_zpass));
    GLFUNC(glStencilOpSeparate)(gl_face, gl_fail, gl_zfail, gl_zpass);
    check_for_errors
}

void am_set_sample_alpha_to_coverage_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        log_gl("glEnable(%s);", "GL_SAMPLE_ALPHA_TO_COVERAGE");
        GLFUNC(glEnable)(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else {
        log_gl("glDisable(%s);", "GL_SAMPLE_ALPHA_TO_COVERAGE");
        GLFUNC(glDisable)(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    check_for_errors
}

void am_set_sample_coverage_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        log_gl("glEnable(%s);", "GL_SAMPLE_COVERAGE");
        GLFUNC(glEnable)(GL_SAMPLE_COVERAGE);
    } else {
        log_gl("glDisable(%s);", "GL_SAMPLE_COVERAGE");
        GLFUNC(glDisable)(GL_SAMPLE_COVERAGE);
    }
    check_for_errors
}

void am_set_sample_coverage(float value, bool invert) {
    check_initialized();
    log_gl("glSampleCoverage(%f, %d);", value, (int)invert);
    GLFUNC(glSampleCoverage)(value, invert);
    check_for_errors
}

// Whole Framebuffer Operations

void am_clear_framebuffer(bool clear_color_buf, bool clear_depth_buf, bool clear_stencil_buf) {
    check_initialized();
    log_gl("glClear(%s | %s | %s);", clear_color_buf ? "GL_COLOR_BUFFER_BIT" : "0",
        clear_depth_buf ? "GL_DEPTH_BUFFER_BIT" : "0",
        clear_stencil_buf ? "GL_STENCIL_BUFFER_BIT" : "0");
    GLFUNC(glClear)((clear_color_buf ? GL_COLOR_BUFFER_BIT : 0) | (clear_depth_buf ? GL_DEPTH_BUFFER_BIT : 0) | (clear_stencil_buf ? GL_STENCIL_BUFFER_BIT : 0));
    check_for_errors
}

void am_set_framebuffer_clear_color(float r, float g, float b, float a) {
    check_initialized();
    log_gl("glClearColor(%0.2f, %0.2f, %0.2f, %0.2f);", r, g, b, a);
    GLFUNC(glClearColor)(r, g, b, a);
    check_for_errors
}

void am_set_framebuffer_clear_depth(float depth) {
    check_initialized();
    log_gl("glClearDepthf(%f);", depth);
    GLFUNC(glClearDepthf)(depth);
    check_for_errors
}

void am_set_framebuffer_clear_stencil_val(am_glint val) {
    check_initialized();
    log_gl("glClearStencil(%d);", val);
    GLFUNC(glClearStencil)(val);
    check_for_errors
}

void am_set_framebuffer_color_mask(bool r, bool g, bool b, bool a) {
    check_initialized();
    log_gl("glColorMask(%d, %d, %d, %d);", (int)r, (int)g, (int)b, (int)a);
    GLFUNC(glColorMask)(r, g, b, a);
    check_for_errors
}

void am_set_framebuffer_depth_mask(bool flag) {
    check_initialized();
    log_gl("glDepthMask(%d);", (int)flag);
    GLFUNC(glDepthMask)(flag);
    check_for_errors
}

void am_set_framebuffer_stencil_mask(am_gluint mask) {
    check_initialized();
    log_gl("glStencilMask(%u);", mask);
    GLFUNC(glStencilMask)(mask);
    check_for_errors
}

// Buffer Objects

am_buffer_id am_create_buffer_object() {
    check_initialized(0);
    GLuint b;
    log_gl("%s", "// about to call glGenBuffers");
    GLFUNC(glGenBuffers)(1, &b);
    log_gl("glGenBuffers(1, &buf[%u]);", b);
    check_for_errors
    return b;
}

void am_bind_buffer(am_buffer_target target, am_buffer_id buffer) {
    check_initialized();
    GLenum gl_target = to_gl_buffer_target(target);
    log_gl("glBindBuffer(%s, buf[%u]);", gl_buffer_target_str(gl_target), buffer);
    GLFUNC(glBindBuffer)(gl_target, buffer);
    check_for_errors
}

void am_set_buffer_data(am_buffer_target target, int size, void *data, am_buffer_usage usage) {
    check_initialized();
    GLenum gl_target = to_gl_buffer_target(target);
    GLenum gl_usage = to_gl_buffer_usage(usage);
    log_gl_ptr(data, size);
    log_gl("glBufferData(%s, %d, ptr[%p], %s);", gl_buffer_target_str(gl_target), size, data, gl_usage_str(gl_usage));
    GLFUNC(glBufferData)(gl_target, size, data, gl_usage);
    check_for_errors
}

void am_set_buffer_sub_data(am_buffer_target target, int offset, int size, void *data) {
    check_initialized();
    GLenum gl_target = to_gl_buffer_target(target);
    log_gl_ptr(data, size);
    log_gl("glBufferSubData(%s, %d, %d, ptr[%p]);", gl_buffer_target_str(gl_target), offset, size, data);
    GLFUNC(glBufferSubData)(gl_target, offset, size, data);
    check_for_errors
}

void am_delete_buffer(am_buffer_id buffer) {
    check_initialized();
    log_gl("glDeleteBuffers(1, &buf[%u]);", buffer);
    GLFUNC(glDeleteBuffers)(1, &buffer);
    check_for_errors
}

// View and Clip

void am_set_depth_range(float near, float far) {
    check_initialized();
    log_gl("glDepthRangef(%f, %f);", near, far);
    GLFUNC(glDepthRangef)(near, far);
    check_for_errors
}

void am_set_scissor_test_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        log_gl("glEnable(%s);", "GL_SCISSOR_TEST");
        GLFUNC(glEnable)(GL_SCISSOR_TEST);
    } else {
        log_gl("glDisable(%s);", "GL_SCISSOR_TEST");
        GLFUNC(glDisable)(GL_SCISSOR_TEST);
    }
    check_for_errors
}

void am_set_scissor(int x, int y, int w, int h) {
    check_initialized();
    log_gl("glScissor(%d, %d, %d, %d);", x, y, w, h);
    GLFUNC(glScissor)(x, y, w, h);
    check_for_errors
}

void am_set_viewport(int x, int y, int w, int h) {
    check_initialized();
    log_gl("glViewport(%d, %d, %d, %d);", x, y, w, h);
    GLFUNC(glViewport)(x, y, w, h);
    check_for_errors
}

// Rasterization

void am_set_front_face_winding(am_face_winding mode) {
    check_initialized();
    GLenum gl_mode = to_gl_face_winding(mode);
    log_gl("glFrontFace(%s);", gl_face_winding_str(gl_mode));
    GLFUNC(glFrontFace)(gl_mode);
    check_for_errors
}

void am_set_cull_face_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        log_gl("glEnable(%s);", "GL_CULL_FACE");
        GLFUNC(glEnable)(GL_CULL_FACE);
    } else {
        log_gl("glDisable(%s);", "GL_CULL_FACE");
        GLFUNC(glDisable)(GL_CULL_FACE);
    }
    check_for_errors
}

void am_set_cull_face_side(am_cull_face_side face) {
    check_initialized();
    GLenum gl_face = to_gl_cull_face_side(face);
    log_gl("glCullFace(%s);", gl_face_side_str(gl_face));
    GLFUNC(glCullFace)(gl_face);
    check_for_errors
}

void am_set_line_width(float width) {
    check_initialized();
    log_gl("glLineWidth(%f);", width);
    GLFUNC(glLineWidth)(width);
    check_for_errors
}

void am_set_polygon_offset_fill_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        log_gl("glEnable(%s);", "GL_POLYGON_OFFSET_FILL");
        GLFUNC(glEnable)(GL_POLYGON_OFFSET_FILL);
    } else {
        log_gl("glDisable(%s);", "GL_POLYGON_OFFSET_FILL");
        GLFUNC(glDisable)(GL_POLYGON_OFFSET_FILL);
    }
    check_for_errors
}

void am_set_polygon_offset(float factor, float units) {
    check_initialized();
    log_gl("glPolygonOffset(%f, %f);", factor, units);
    GLFUNC(glPolygonOffset)(factor, units);
    check_for_errors
}

// Dithering

void am_set_dither_enabled(bool enabled) {
    check_initialized();
    if (enabled) {
        log_gl("glEnable(%s);", "GL_DITHER");
        GLFUNC(glEnable)(GL_DITHER);
    } else {
        log_gl("glDisable(%s);", "GL_DITHER");
        GLFUNC(glDisable)(GL_DITHER);
    }
    check_for_errors
}

// Programs and Shaders

am_program_id am_create_program() {
    check_initialized(0);
    log_gl("%s", "// about to call glCreateProgram");
    GLuint p = GLFUNC(glCreateProgram)();
    log_gl("prog[%u] = glCreateProgram();", p);
    check_for_errors
    return p;
}

am_shader_id am_create_shader(am_shader_type type) {
    check_initialized(0);
    GLenum gl_type = to_gl_shader_type(type);
    log_gl("%s", "// about to call glCreateShader");
    GLuint s = GLFUNC(glCreateShader)(gl_type);
    log_gl("shader[%u] = glCreateShader(%s);", s, gl_shader_type_str(gl_type));
    check_for_errors
    return s;
}

bool am_compile_shader(am_shader_id shader, am_shader_type type, const char *src, char **msg, int *line_no, char **line_str) {
    GLint compiled;
    *msg = NULL;
    *line_str = NULL;
    *line_no = -1;
    if (!gl_initialized) {
        const char *m = "gl not initialized";
        *msg = (char*)malloc(strlen(m) + 1);
        strcpy(*msg, m);
        compiled = 0;
        goto end;
    }
    log_gl("/*\n%s\n*/", src);
#if defined(AM_USE_GLSL_OPTIMIZER)
    if (!am_conf_d3dangle) {
        char *translate_objcode = NULL;
        char *translate_errmsg = NULL;
        glslopt_shader *gshader = NULL;
        glslopt_translate_shader(type, src, &translate_objcode, &translate_errmsg, &gshader);
        if (translate_errmsg != NULL) {
            *msg = am_format("%s", translate_errmsg);
            glslopt_shader_delete(gshader);
            get_src_error_line(*msg, src, line_no, line_str);
            compiled = 0;
            goto end;
        }
        assert(translate_objcode != NULL);
        log_gl("/* GLSL Optimizer output:\n%s\n*/", translate_objcode);
        log_gl_ptr(translate_objcode, strlen(translate_objcode));
        log_gl("glShaderSource(shader[%u], 1, (const char**)&ptr[%p], NULL);", shader, translate_objcode);
        GLFUNC(glShaderSource)(shader, 1, (const char**)&translate_objcode, NULL);
        glslopt_shader_delete(gshader);
    } else {
        log_gl_ptr(src, strlen(src));
        log_gl("glShaderSource(shader[%u], 1, (const char**)&ptr[%p], NULL);", shader, src);
        GLFUNC(glShaderSource)(shader, 1, &src, NULL);
    }
#elif defined(AM_ANGLE_TRANSLATE_GL)
    if (!am_conf_d3dangle) {
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
        log_gl("/* ANGLE translation:\n%s\n*/", translate_objcode);
        log_gl_ptr(translate_objcode, strlen(translate_objcode));
        log_gl("glShaderSource(shader[%u], 1, (const char**)&ptr[%p], NULL);", shader, translate_objcode);
        GLFUNC(glShaderSource)(shader, 1, (const char**)&translate_objcode, NULL);
        free(translate_objcode);
    } else {
        log_gl_ptr(src, strlen(src));
        log_gl("glShaderSource(shader[%u], 1, (const char**)&ptr[%p], NULL);", shader, src);
        GLFUNC(glShaderSource)(shader, 1, &src, NULL);
    }
#else
    log_gl_ptr(src, strlen(src));
    log_gl("glShaderSource(shader[%u], 1, (const char**)&ptr[%p], NULL);", shader, src);
    GLFUNC(glShaderSource)(shader, 1, &src, NULL);
#endif
    check_for_errors

    log_gl("glCompileShader(shader[%u]);", shader);
    GLFUNC(glCompileShader)(shader);
    GLFUNC(glGetShaderiv)(shader, GL_COMPILE_STATUS, &compiled);
    check_for_errors
    if (!compiled) {
        GLint len = 0;
        GLFUNC(glGetShaderiv)(shader, GL_INFO_LOG_LENGTH, &len);
        if (len > 1) {
            *msg = (char*)malloc(sizeof(char) * len);
            GLFUNC(glGetShaderInfoLog)(shader, len, NULL, *msg);
            get_src_error_line(*msg, src, line_no, line_str);
        } else {
            const char *cmsg = "unknown error";
            *msg = (char*)malloc(strlen(cmsg) + 1);
            strcpy(*msg, cmsg);
        }
    }
end:
    if (compiled) {
        log_gl("%s", "// compile succeeded");
    } else {
        log_gl("%s", "// compile FAILED");
    }
    return compiled;
}

void am_attach_shader(am_program_id program, am_shader_id shader) {
    check_initialized();
    log_gl("glAttachShader(prog[%u], shader[%u]);", program, shader);
    GLFUNC(glAttachShader)(program, shader);
    check_for_errors
}

bool am_link_program(am_program_id program) {
    check_initialized(false);
    GLint linked;
    log_gl("glLinkProgram(prog[%u]);", program);
    GLFUNC(glLinkProgram)(program);
    GLFUNC(glGetProgramiv)(program, GL_LINK_STATUS, &linked);
    if (linked) {
        log_gl("%s", "// link succeeded");
    } else {
        log_gl("%s", "// link FAILED");
    }
    check_for_errors
    return linked;
}

char *am_get_program_info_log(am_program_id program) {
    check_initialized(NULL);
    GLint len = 0;
    log_gl("%s", "// glGetProgramiInfoLog(...);");
    GLFUNC(glGetProgramiv)(program, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        char* msg = (char*)malloc(sizeof(char) * len);
        GLFUNC(glGetProgramInfoLog)(program, len, NULL, msg);
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
    log_gl("%s", "// about to call glGetProgramiv(GL_ACTIVE_ATTRIBUTES)");
    GLFUNC(glGetProgramiv)(program, GL_ACTIVE_ATTRIBUTES, &val);
    log_gl("{GLint v;\nglGetProgramiv(prog[%u], GL_ACTIVE_ATTRIBUTES, &v);\nassert(v == %d);}", program, val);
    check_for_errors
    return val;
}

int am_get_program_active_uniforms(am_program_id program) {
    check_initialized(0);
    GLint val;
    log_gl("%s", "// about to call glGetProgramiv(GL_ACTIVE_UNIFORMS)");
    GLFUNC(glGetProgramiv)(program, GL_ACTIVE_UNIFORMS, &val);
    log_gl("{GLint v;\nglGetProgramiv(prog[%u], GL_ACTIVE_UNIFORMS, &v);\nassert(v == %d);}", program, val);
    check_for_errors
    return val;
}

bool am_validate_program(am_program_id program) {
    check_initialized(false);
    log_gl("glValidateProgram(prog[%u]);", program);
    GLFUNC(glValidateProgram)(program);
    GLint valid;
    GLFUNC(glGetProgramiv)(program, GL_VALIDATE_STATUS, &valid);
    if (valid) {
        log_gl("%s", "// validation succeeded");
    } else {
        log_gl("%s", "// validation FAILED");
    }
    return valid;
}

void am_use_program(am_program_id program) {
    check_initialized();
    log_gl("glUseProgram(prog[%u]);", program);
    GLFUNC(glUseProgram)(program);
    check_for_errors
    am_frame_use_program_calls++;
}

void am_detach_shader(am_program_id program, am_shader_id shader) {
    check_initialized();
    log_gl("glDetachShader(prog[%u], shader[%u]);", program, shader);
    GLFUNC(glDetachShader)(program, shader);
    check_for_errors
}

void am_delete_shader(am_shader_id shader) {
    check_initialized();
    log_gl("glDeleteShader(shader[%u]);", shader);
    GLFUNC(glDeleteShader)(shader);
    check_for_errors
}

void am_delete_program(am_program_id program) {
    check_initialized();
    log_gl("glDeleteProgram(prog[%u]);", program);
    GLFUNC(glDeleteProgram)(program);
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
        log_gl("glEnableVertexAttribArray(%u);", location);
        GLFUNC(glEnableVertexAttribArray)(location);
    } else {
        log_gl("glDisableVertexAttribArray(%u);", location);
        GLFUNC(glDisableVertexAttribArray)(location);
    }
    check_for_errors
}

static am_gluint get_attribute_location(am_program_id program, const char *name) {
    check_initialized(0);
    log_gl("%s", "// about to call glGetAttribLocation");
    am_gluint l = GLFUNC(glGetAttribLocation)(program, name);
    log_gl("{GLuint l = glGetAttribLocation(prog[%u], \"%s\");\nassert(l == %u);}", program, name, l);
    check_for_errors
    return l;
}

static am_gluint get_uniform_location(am_program_id program, const char *name) {
    check_initialized(0);
    log_gl("%s", "// about to call glGetUniformLocation");
    am_gluint l = GLFUNC(glGetUniformLocation)(program, name);
    log_gl("{GLuint l = glGetUniformLocation(prog[%u], \"%s\");\nassert(l == %u);}", program, name, l);
    check_for_errors
    return l;
}

void am_get_active_attribute(am_program_id program, am_gluint index,
    char **name, am_attribute_var_type *type, int *size, am_gluint *loc)
{
    check_initialized();
    GLchar gl_name[ATTR_NAME_SIZE];
    GLint gl_size;
    GLenum gl_type;
    log_gl("%s", "// about to call glGetActiveAttrib");
    GLFUNC(glGetActiveAttrib)(program, index, ATTR_NAME_SIZE, NULL, &gl_size, &gl_type, gl_name);
    log_gl("{GLint sz; GLchar nm[%d]; GLenum tp;\nglGetActiveAttrib(prog[%u], %u, %d, NULL, &sz, &tp, nm);",
        ATTR_NAME_SIZE, program, index, ATTR_NAME_SIZE);
    log_gl("assert(sz == %u);\nassert(tp == %s);\nassert(strcmp(nm, \"%s\") == 0);}",
        gl_size, gl_type_str(gl_type), gl_name);
    check_for_errors
    *name = (char*)malloc(strlen(gl_name) + 1);
    strcpy(*name, gl_name);
    *size = gl_size;
    *type = from_gl_attribute_var_type(gl_type);
    *loc = get_attribute_location(program, *name);
}

void am_get_active_uniform(am_program_id program, am_gluint index,
    char **name, am_uniform_var_type *type, int *size, am_gluint *loc)
{
    check_initialized();
    GLchar gl_name[UNI_NAME_SIZE];
    GLint gl_size;
    GLenum gl_type;
    log_gl("%s", "// about to call glGetActiveUniform");
    GLFUNC(glGetActiveUniform)(program, index, UNI_NAME_SIZE, NULL, &gl_size, &gl_type, gl_name);
    log_gl("{GLint sz; GLchar nm[%d]; GLenum tp;\nglGetActiveUniform(prog[%u], %u, %d, NULL, &sz, &tp, nm);",
        UNI_NAME_SIZE, program, index, UNI_NAME_SIZE);
    log_gl("assert(sz == %u);\nassert(tp == %s);\nassert(strcmp(nm, \"%s\") == 0);}",
        gl_size, gl_type_str(gl_type), gl_name);
    check_for_errors
    *name = (char*)malloc(strlen(gl_name) + 1);
    strcpy(*name, gl_name);
    *size = gl_size;
    *type = from_gl_uniform_var_type(gl_type);
    *loc = get_uniform_location(program, *name);
}

void am_set_uniform1f(am_gluint location, float value) {
    check_initialized();
    log_gl("{GLfloat v = %f;\nglUniform1fv(%u, 1, &v);}", value, location);
    GLFUNC(glUniform1fv)(location, 1, &value);
    check_for_errors
}

void am_set_uniform2f(am_gluint location, const float *value) {
    check_initialized();
    log_gl("{const GLfloat v[] = {%f, %f};\nglUniform2fv(%u, 1, v);}",
        value[0], value[1], location);
    GLFUNC(glUniform2fv)(location, 1, value);
    check_for_errors
}

void am_set_uniform3f(am_gluint location, const float *value) {
    check_initialized();
    log_gl("{const GLfloat v[] = {%f, %f, %f};\nglUniform3fv(%u, 1, v);}",
        value[0], value[1], value[2], location);
    GLFUNC(glUniform3fv)(location, 1, value);
    check_for_errors
}

void am_set_uniform4f(am_gluint location, const float *value) {
    check_initialized();
    log_gl("{const GLfloat v[] = {%f, %f, %f, %f};\nglUniform4fv(%u, 1, v);}",
        value[0], value[1], value[2], value[3], location);
    GLFUNC(glUniform4fv)(location, 1, value);
    check_for_errors
}

void am_set_uniform1i(am_gluint location, am_glint value) {
    check_initialized();
    log_gl("{GLint v = %d;\nglUniform1iv(%u, 1, &v);}", value, location);
    GLFUNC(glUniform1iv)(location, 1, &value);
    check_for_errors
}

void am_set_uniform2i(am_gluint location, const am_glint *value) {
    check_initialized();
    log_gl("{const GLint v[] = {%d, %d};\nglUniform2iv(%u, 1, v);}",
        value[0], value[1], location);
    GLFUNC(glUniform2iv)(location, 1, value);
    check_for_errors
}

void am_set_uniform3i(am_gluint location, const am_glint *value) {
    check_initialized();
    log_gl("{const GLint v[] = {%d, %d, %d};\nglUniform3iv(%u, 1, v);}",
        value[0], value[1], value[2], location);
    GLFUNC(glUniform3iv)(location, 1, value);
    check_for_errors
}

void am_set_uniform4i(am_gluint location, const am_glint *value) {
    check_initialized();
    log_gl("{const GLint v[] = {%d, %d, %d, %d};\nglUniform4iv(%u, 1, v);}",
        value[0], value[1], value[2], value[3], location);
    GLFUNC(glUniform4iv)(location, 1, value);
    check_for_errors
}

void am_set_uniform_mat2(am_gluint location, const float *value) {
    check_initialized();
    log_gl("{const GLfloat v[] = {\n%f, %f, \n%f, %f};\nglUniformMatrix2fv(%u, 1, GL_FALSE, v);}",
        value[0], value[1], value[2], value[3], location);
    GLFUNC(glUniformMatrix2fv)(location, 1, GL_FALSE, value);
    check_for_errors
}

void am_set_uniform_mat3(am_gluint location, const float *value) {
    check_initialized();
    log_gl("{const GLfloat v[] = {\n%f, %f, %f,\n%f, %f, %f,\n%f, %f, %f};\nglUniformMatrix3fv(%u, 1, GL_FALSE, v);}", 
        value[0], value[1], value[2],
        value[3], value[4], value[5],
        value[6], value[7], value[8], location);
    GLFUNC(glUniformMatrix3fv)(location, 1, GL_FALSE, value);
    check_for_errors
}

void am_set_uniform_mat4(am_gluint location, const float *value) {
    check_initialized();
    log_gl("{const GLfloat v[] = {\n%f, %f, %f, %f,\n%f, %f, %f, %f,\n%f, %f, %f, %f,\n%f, %f, %f, %f};\nglUniformMatrix4fv(%u, 1, GL_FALSE, v);}",
        value[0], value[1], value[2], value[3],
        value[4], value[5], value[6], value[7],
        value[8], value[9], value[10], value[11],
        value[12], value[13], value[14], value[15], location);
    GLFUNC(glUniformMatrix4fv)(location, 1, GL_FALSE, value);
    check_for_errors
}

void am_set_attribute1f(am_gluint location, const float value) {
    check_initialized();
    log_gl("glVertexAttrib1f(%u, %f);", location, value);
    GLFUNC(glVertexAttrib1f)(location, value);
    check_for_errors
}

void am_set_attribute2f(am_gluint location, const float *value) {
    check_initialized();
    log_gl("{const GLfloat v[] = {%f, %f};\nglVertexAttrib2fv(%u, v);}",
        value[0], value[1], location);
    GLFUNC(glVertexAttrib2fv)(location, value);
    check_for_errors
}

void am_set_attribute3f(am_gluint location, const float *value) {
    check_initialized();
    log_gl("{const GLfloat v[] = {%f, %f, %f};\nglVertexAttrib3fv(%u, v);}",
        value[0], value[1], value[2], location);
    GLFUNC(glVertexAttrib3fv)(location, value);
    check_for_errors
}

void am_set_attribute4f(am_gluint location, const float *value) {
    check_initialized();
    log_gl("{const GLfloat v[] = {%f, %f, %f, %f};\nglVertexAttrib4fv(%u, v);}",
        value[0], value[1], value[2], value[3], location);
    GLFUNC(glVertexAttrib4fv)(location, value);
    check_for_errors
}

void am_set_attribute_pointer(am_gluint location, int size, am_attribute_client_type type, bool normalized, int stride, int offset) {
    check_initialized();
    GLenum gl_type = to_gl_attr_client_type(type);
    log_gl("glVertexAttribPointer(%u, %d, %s, %d, %d, (void*)((uintptr_t)%d));", location, size, gl_type_str(gl_type), (int)normalized, stride, offset);
    GLFUNC(glVertexAttribPointer)(location, size, gl_type, normalized, stride, (void*)((uintptr_t)offset));
    check_for_errors
}

// Texture Objects

void am_set_active_texture_unit(int texture_unit) {
    check_initialized();
    if (texture_unit < am_max_combined_texture_image_units) {
        log_gl("glActiveTexture(GL_TEXTURE0 + %d);", texture_unit);
        GLFUNC(glActiveTexture)(GL_TEXTURE0 + texture_unit);
    } else {
        am_log1("WARNING: too many active texture units (max %d)", am_max_combined_texture_image_units);
    }
    check_for_errors
}

am_texture_id am_create_texture() {
    check_initialized(0);
    GLuint tex;
    log_gl("%s", "// about to call glGenTextures");
    GLFUNC(glGenTextures)(1, &tex);
    log_gl("glGenTextures(1, &tex[%u]);", tex);
    check_for_errors
    return tex;
}

void am_delete_texture(am_texture_id texture) {
    check_initialized();
    log_gl("glDeleteTextures(1, &tex[%u]);", texture);
    GLFUNC(glDeleteTextures)(1, &texture);
    check_for_errors
}

void am_bind_texture(am_texture_bind_target target, am_texture_id texture) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    log_gl("glBindTexture(%s, tex[%u]);", gl_texture_target_str(gl_target), texture);
    GLFUNC(glBindTexture)(gl_target, texture);
    check_for_errors
}

void am_copy_texture_image_2d(am_texture_copy_target target, int level, am_texture_format format, int x, int y, int w, int h) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    GLenum gl_format = to_gl_texture_format(format);
    log_gl("glCopyTexImage2D(%s, %d, %s, %d, %d, %d, %d, 0);",
        gl_texture_target_str(gl_target), level, gl_texture_format_str(gl_format),
        x, y, w, h);
    GLFUNC(glCopyTexImage2D)(gl_target, level, gl_format, x, y, w, h, 0);
    check_for_errors
}

void am_copy_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int x, int y, int w, int h) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    log_gl("glCopyTexSubImage2D(%s, %d, %d, %d, %d, %d, %d, %d);",
        gl_texture_target_str(gl_target), level,
        xoffset, yoffset, x, y, w, h);
    GLFUNC(glCopyTexSubImage2D)(gl_target, level, xoffset, yoffset, x, y, w, h);
    check_for_errors
}

void am_generate_mipmap(am_texture_bind_target target) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    log_gl("glGenerateMipmap(%s);", gl_texture_target_str(gl_target));
    GLFUNC(glGenerateMipmap)(gl_target);
    check_for_errors
}

int am_compute_pixel_size(am_texture_format format, am_texture_type type) {
    switch (type) {
        case AM_TEXTURE_TYPE_UBYTE:
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
        case AM_TEXTURE_TYPE_USHORT_5_6_5:
        case AM_TEXTURE_TYPE_USHORT_4_4_4_4:
        case AM_TEXTURE_TYPE_USHORT_5_5_5_1:
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
    log_gl_ptr(data, w * h * am_compute_pixel_size(format, type));
    log_gl("glTexImage2D(%s, %d, %s, %d, %d, 0, %s, %s, ptr[%p]);",
        gl_texture_target_str(gl_target), level,
        gl_texture_format_str(gl_format),
        w, h,
        gl_texture_format_str(gl_format),
        gl_texture_type_str(gl_type),
        data);
    GLFUNC(glTexImage2D)(gl_target, level, gl_format, w, h, 0, gl_format, gl_type, data);
    check_for_errors
}

void am_set_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int w, int h, am_texture_format format, am_texture_type type, void *data) {
    check_initialized();
    GLenum gl_target = to_gl_texture_copy_target(target);
    GLenum gl_format = to_gl_texture_format(format);
    GLenum gl_type = to_gl_texture_type(type);
    log_gl_ptr(data, w * h * am_compute_pixel_size(format, type));
    log_gl("glTexSubImage2D(%s, %d, %d, %d, %d, %d, %s, %s, ptr[%p]);",
        gl_texture_target_str(gl_target), level, xoffset, yoffset, w, h,
        gl_texture_format_str(gl_format),
        gl_texture_type_str(gl_type),
        data);
    GLFUNC(glTexSubImage2D)(gl_target, level, xoffset, yoffset, w, h, gl_format, gl_type, data);
    check_for_errors
}

void am_set_texture_min_filter(am_texture_bind_target target, am_texture_min_filter filter) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    GLenum gl_filter = to_gl_texture_min_filter(filter);
    log_gl("glTexParameteri(%s, GL_TEXTURE_MIN_FILTER, %s);",
        gl_texture_target_str(gl_target),
        gl_texture_filter_str(gl_filter));
    GLFUNC(glTexParameteri)(gl_target, GL_TEXTURE_MIN_FILTER, gl_filter);
    check_for_errors
}

void am_set_texture_mag_filter(am_texture_bind_target target, am_texture_mag_filter filter) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    GLenum gl_filter = to_gl_texture_mag_filter(filter);
    log_gl("glTexParameteri(%s, GL_TEXTURE_MAG_FILTER, %s);",
        gl_texture_target_str(gl_target),
        gl_texture_filter_str(gl_filter));
    GLFUNC(glTexParameteri)(gl_target, GL_TEXTURE_MAG_FILTER, gl_filter);
    check_for_errors
}

void am_set_texture_wrap(am_texture_bind_target target, am_texture_wrap s_wrap, am_texture_wrap t_wrap) {
    check_initialized();
    GLenum gl_target = to_gl_texture_bind_target(target);
    GLenum gl_swrap = to_gl_texture_wrap(s_wrap);
    GLenum gl_twrap = to_gl_texture_wrap(t_wrap);
    log_gl("glTexParameteri(%s, GL_TEXTURE_WRAP_S, %s);",
        gl_texture_target_str(gl_target),
        gl_texture_wrap_str(gl_swrap));
    GLFUNC(glTexParameteri)(gl_target, GL_TEXTURE_WRAP_S, gl_swrap);
    check_for_errors
    log_gl("glTexParameteri(%s, GL_TEXTURE_WRAP_T, %s);",
        gl_texture_target_str(gl_target),
        gl_texture_wrap_str(gl_twrap));
    GLFUNC(glTexParameteri)(gl_target, GL_TEXTURE_WRAP_T, gl_twrap);
    check_for_errors
}

// Renderbuffer Objects

am_renderbuffer_id am_create_renderbuffer() {
    check_initialized(0);
    GLuint rb;
    log_gl("%s", "// about to call glGenRenderbuffers");
    GLFUNC(glGenRenderbuffers)(1, &rb);
    log_gl("glGenRenderbuffers(1, &rbuf[%u]);", rb);
    check_for_errors
    return rb;
}

void am_delete_renderbuffer(am_renderbuffer_id rb) {
    check_initialized();
    log_gl("glDeleteRenderbuffers(1, &rbuf[%u]);", rb);
    GLFUNC(glDeleteRenderbuffers)(1, &rb);
    check_for_errors
}

void am_bind_renderbuffer(am_renderbuffer_id rb) {
    check_initialized();
    log_gl("glBindRenderbuffer(GL_RENDERBUFFER, rbuf[%u]);", rb);
    GLFUNC(glBindRenderbuffer)(GL_RENDERBUFFER, rb);
    check_for_errors
}

void am_set_renderbuffer_storage(am_renderbuffer_format format, int w, int h) {
    check_initialized();
    GLenum gl_format = to_gl_renderbuffer_format(format);
    log_gl("glRenderbufferStorage(GL_RENDERBUFFER, %s, %d, %d);",
        gl_renderbuffer_format_str(gl_format), w, h);
    GLFUNC(glRenderbufferStorage)(GL_RENDERBUFFER, gl_format, w, h);
    check_for_errors
}

// Read Back Pixels 

void am_read_pixels(int x, int y, int w, int h, void *data) {
    check_initialized();
    log_gl("{char data[%d];\nglReadPixels(%d, %d, %d, %d, GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);}",
        w * h * 4, x, y, w, h);
    GLFUNC(glReadPixels)(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
    check_for_errors
}

// Framebuffer Objects

am_framebuffer_id am_create_framebuffer() {
    check_initialized(0);
    GLuint fb;
    log_gl("%s", "// about to call glGenFramebuffers");
    GLFUNC(glGenFramebuffers)(1, &fb);
    log_gl("glGenFramebuffers(1, &fbuf[%u]);", fb);
    check_for_errors
    return fb;
}

void am_delete_framebuffer(am_framebuffer_id fb) {
    check_initialized();
    log_gl("glDeleteFramebuffers(1, &fbuf[%u]);", fb);
    GLFUNC(glDeleteFramebuffers)(1, &fb);
    check_for_errors
}

void am_bind_framebuffer(am_framebuffer_id fb) {
    check_initialized();
    log_gl("glBindFramebuffer(GL_FRAMEBUFFER, fbuf[%u]);", fb);
    GLFUNC(glBindFramebuffer)(GL_FRAMEBUFFER, fb);
    check_for_errors
}

am_framebuffer_status am_check_framebuffer_status() {
    check_initialized(AM_FRAMEBUFFER_STATUS_UNKNOWN);
    log_gl("%s", "// about to call glCheckFramebufferStatus");
    GLenum gl_status = GLFUNC(glCheckFramebufferStatus)(GL_FRAMEBUFFER);
    log_gl("glCheckFramebufferStatus(GL_FRAMEBUFFER); // %s", gl_framebuffer_status_string(gl_status));
    return from_gl_framebuffer_status(gl_status);
}

void am_set_framebuffer_renderbuffer(am_framebuffer_attachment attachment, am_renderbuffer_id rb) {
    check_initialized();
    GLenum gl_attachment = to_gl_framebuffer_attachment(attachment);
    log_gl("glFramebufferRenderbuffer(GL_FRAMEBUFFER, %s, GL_RENDERBUFFER, rbuf[%u]);",
        gl_framebuffer_attachment_str(gl_attachment), rb);
    GLFUNC(glFramebufferRenderbuffer)(GL_FRAMEBUFFER, gl_attachment, GL_RENDERBUFFER, rb);
    check_for_errors
}

void am_set_framebuffer_texture2d(am_framebuffer_attachment attachment, am_texture_copy_target target, am_texture_id texture) {
    check_initialized();
    GLenum gl_attachment = to_gl_framebuffer_attachment(attachment);
    GLenum gl_target = to_gl_texture_copy_target(target);
    log_gl("glFramebufferTexture2D(GL_FRAMEBUFFER, %s, %s, tex[%u], 0);",
        gl_framebuffer_attachment_str(gl_attachment), gl_texture_target_str(gl_target),
        texture);
    GLFUNC(glFramebufferTexture2D)(GL_FRAMEBUFFER, gl_attachment, gl_target, texture, 0);
    check_for_errors
}

// Writing to the Draw Buffer

void am_draw_arrays(am_draw_mode mode, int first, int count) {
    check_initialized();
    GLenum gl_mode = to_gl_draw_mode(mode);
    log_gl("glDrawArrays(%s, %d, %d);",
        gl_draw_mode_str(gl_mode), first, count);
    GLFUNC(glDrawArrays)(gl_mode, first, count);
    check_for_errors
    am_frame_draw_calls++;
}

void am_draw_elements(am_draw_mode mode, int count, am_element_index_type type, int offset) {
    check_initialized();
    GLenum gl_mode = to_gl_draw_mode(mode);
    GLenum gl_type = to_gl_element_index_type(type);
    log_gl("glDrawElements(%s, %d, %s, %d);",
        gl_draw_mode_str(gl_mode), count, gl_type_str(gl_type), offset);
    GLFUNC(glDrawElements)(gl_mode, count, gl_type, (void*)((uintptr_t)offset));
    check_for_errors
    am_frame_draw_calls++;
}

void am_gl_end_framebuffer_render() {
}

void am_gl_end_frame(bool present) {
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
        case AM_TEXTURE_TYPE_UBYTE: return GL_UNSIGNED_BYTE;
        case AM_TEXTURE_TYPE_USHORT_5_6_5: return GL_UNSIGNED_SHORT_5_6_5;
        case AM_TEXTURE_TYPE_USHORT_4_4_4_4: return GL_UNSIGNED_SHORT_4_4_4_4;
        case AM_TEXTURE_TYPE_USHORT_5_5_5_1: return GL_UNSIGNED_SHORT_5_5_5_1;
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
#if defined(GL_DEPTH_COMPONENT24)
        case AM_RENDERBUFFER_FORMAT_DEPTH: return GL_DEPTH_COMPONENT24;
#elif defined(GL_DEPTH_COMPONENT24_OES)
        case AM_RENDERBUFFER_FORMAT_DEPTH: return GL_DEPTH_COMPONENT24_OES;
#else
        case AM_RENDERBUFFER_FORMAT_DEPTH: return GL_DEPTH_COMPONENT16;
#endif
        case AM_RENDERBUFFER_FORMAT_STENCIL: return GL_STENCIL_INDEX8;
        case AM_RENDERBUFFER_FORMAT_DEPTHSTENCIL: {
            am_log1("%s", "error: combined depthstencil format unsupported in this backend");
            return 0;
        }
    }
    return 0;
}

static GLenum to_gl_framebuffer_attachment(am_framebuffer_attachment a) {
    switch (a) {
        case AM_FRAMEBUFFER_COLOR_ATTACHMENT0: return GL_COLOR_ATTACHMENT0;
        case AM_FRAMEBUFFER_DEPTH_ATTACHMENT: return GL_DEPTH_ATTACHMENT;
        case AM_FRAMEBUFFER_STENCIL_ATTACHMENT: return GL_STENCIL_ATTACHMENT;
        case AM_FRAMEBUFFER_DEPTHSTENCIL_ATTACHMENT: {
            am_log1("%s", "error: combined depthstencil attachment unsupported in this backend");
            return 0;
        }
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
    GLenum err = GLFUNC(glGetError)();
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

static void print_ptr(FILE* f, void* p, int len) {
    uint8_t *ptr = (uint8_t*)p;
    fprintf(f, "ptr[%p] = (void*)\"", ptr);
    for (int i = 0; i < len; i++) {
        fprintf(f, "\\x%02X", *ptr);
        ptr++;
    }
    fprintf(f, "\";\n");
}

void am_log_gl(const char *msg) {
    if (gl_log_file != NULL) {
        log_gl("%s", msg);
    }
}

//-------------------------------------------------------------------------------------------------
// GLSL-optimiser
#if defined(AM_USE_GLSL_OPTIMIZER)

static glslopt_ctx* glslopt_context = NULL;

static void init_glslopt() {
    glslopt_target target = kGlslTargetOpenGLES20;
    glslopt_context = glslopt_initialize(target);
    if (!glslopt_context) {
        am_abort("unable to initialize glsl optimizer");
    }
}

static void destroy_glslopt() {
    glslopt_cleanup(glslopt_context);
}

static void glslopt_translate_shader(am_shader_type type,
    const char *src, char **objcode, char **errmsg, glslopt_shader **shader)
{
    *objcode = NULL;
    *errmsg = NULL;
    glslopt_shader_type gtype;

    switch (type) {
        case AM_VERTEX_SHADER: gtype = kGlslOptShaderVertex; break;
        case AM_FRAGMENT_SHADER: gtype = kGlslOptShaderFragment; break;
    }
    unsigned options = 0;

    *shader = glslopt_optimize(glslopt_context, gtype, src, options);

    if (glslopt_get_status(*shader)) {
        *objcode = (char*)glslopt_get_output(*shader);
    } else {
        *errmsg = (char*)glslopt_get_log(*shader);
    }
}
#endif

//-------------------------------------------------------------------------------------------------
// Angle shader translater
#if defined(AM_ANGLE_TRANSLATE_GL)

static ShBuiltInResources angle_resources;
static ShHandle angle_vcompiler;
static ShHandle angle_fcompiler;
static int angle_comp_options =
    SH_OBJECT_CODE | SH_INIT_GL_POSITION |
    SH_UNFOLD_SHORT_CIRCUIT | SH_INIT_VARYINGS_WITHOUT_STATIC_USE;

static void init_angle() {
    ShInitialize();

    ShInitBuiltInResources(&angle_resources);
    angle_resources.MaxVertexAttribs = am_max_vertex_attribs;
    angle_resources.MaxVertexUniformVectors = am_max_vertex_uniform_vectors;
    angle_resources.MaxVaryingVectors = am_max_varying_vectors;
    angle_resources.MaxVertexTextureImageUnits = am_max_vertex_texture_image_units;
    angle_resources.MaxCombinedTextureImageUnits = am_max_combined_texture_image_units;
    angle_resources.MaxTextureImageUnits = am_max_texture_image_units;
    angle_resources.MaxFragmentUniformVectors = am_max_fragment_uniform_vectors;
    angle_resources.MaxDrawBuffers = 4;
    angle_resources.OES_standard_derivatives = 1;
    angle_resources.FragmentPrecisionHigh = 1;

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

void am_reset_gl_frame_stats() {
    am_frame_draw_calls = 0;
    am_frame_use_program_calls = 0;
}

bool am_gl_requires_combined_depthstencil() {
    return false;
}

#endif  // AM_USE_METAL
