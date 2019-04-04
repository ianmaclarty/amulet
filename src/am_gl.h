typedef uint32_t am_gluint;
typedef int32_t  am_glint;

// Initialization

void am_init_gl();
void am_destroy_gl();
bool am_gl_is_initialized();

// Constants (available after initialization)

extern int am_max_combined_texture_image_units;
extern int am_max_cube_map_texture_size;
extern int am_max_fragment_uniform_vectors;
extern int am_max_renderbuffer_size;
extern int am_max_texture_image_units;
extern int am_max_texture_size;
extern int am_max_varying_vectors;
extern int am_max_vertex_attribs;
extern int am_max_vertex_texture_image_units;
extern int am_max_vertex_uniform_vectors;
extern int am_frame_draw_calls;
extern int am_frame_use_program_calls;

// Per-Fragment Operations

enum am_blend_equation {
    AM_BLEND_EQUATION_ADD,
    AM_BLEND_EQUATION_SUBTRACT,
    AM_BLEND_EQUATION_REVERSE_SUBTRACT,
};

enum am_blend_sfactor {
    AM_BLEND_SFACTOR_ZERO,
    AM_BLEND_SFACTOR_ONE,
    AM_BLEND_SFACTOR_SRC_COLOR,
    AM_BLEND_SFACTOR_ONE_MINUS_SRC_COLOR,
    AM_BLEND_SFACTOR_DST_COLOR,
    AM_BLEND_SFACTOR_ONE_MINUS_DST_COLOR,
    AM_BLEND_SFACTOR_SRC_ALPHA,
    AM_BLEND_SFACTOR_ONE_MINUS_SRC_ALPHA,
    AM_BLEND_SFACTOR_DST_ALPHA,
    AM_BLEND_SFACTOR_ONE_MINUS_DST_ALPHA,
    AM_BLEND_SFACTOR_CONSTANT_COLOR,
    AM_BLEND_SFACTOR_ONE_MINUS_CONSTANT_COLOR,
    AM_BLEND_SFACTOR_CONSTANT_ALPHA,
    AM_BLEND_SFACTOR_ONE_MINUS_CONSTANT_ALPHA,
    AM_BLEND_SFACTOR_SRC_ALPHA_SATURATE,
};

enum am_blend_dfactor {
    AM_BLEND_DFACTOR_ZERO,
    AM_BLEND_DFACTOR_ONE,
    AM_BLEND_DFACTOR_SRC_COLOR,
    AM_BLEND_DFACTOR_ONE_MINUS_SRC_COLOR,
    AM_BLEND_DFACTOR_DST_COLOR,
    AM_BLEND_DFACTOR_ONE_MINUS_DST_COLOR,
    AM_BLEND_DFACTOR_SRC_ALPHA,
    AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA,
    AM_BLEND_DFACTOR_DST_ALPHA,
    AM_BLEND_DFACTOR_ONE_MINUS_DST_ALPHA,
    AM_BLEND_DFACTOR_CONSTANT_COLOR,
    AM_BLEND_DFACTOR_ONE_MINUS_CONSTANT_COLOR,
    AM_BLEND_DFACTOR_CONSTANT_ALPHA,
    AM_BLEND_DFACTOR_ONE_MINUS_CONSTANT_ALPHA,
};

void am_set_blend_enabled(bool enabled);
void am_set_blend_color(float r, float g, float b, float a);
void am_set_blend_equation(am_blend_equation rgb, am_blend_equation alpha);
void am_set_blend_func(am_blend_sfactor src_rgb, am_blend_dfactor dst_rgb, am_blend_sfactor src_alpha, am_blend_dfactor dst_alpha);

enum am_depth_func {
    AM_DEPTH_FUNC_NEVER,
    AM_DEPTH_FUNC_ALWAYS,
    AM_DEPTH_FUNC_EQUAL,
    AM_DEPTH_FUNC_NOTEQUAL,
    AM_DEPTH_FUNC_LESS,
    AM_DEPTH_FUNC_LEQUAL,
    AM_DEPTH_FUNC_GREATER,
    AM_DEPTH_FUNC_GEQUAL,
};

void am_set_depth_test_enabled(bool enabled);
void am_set_depth_func(am_depth_func func);

enum am_stencil_func {
    AM_STENCIL_FUNC_NEVER,
    AM_STENCIL_FUNC_ALWAYS,
    AM_STENCIL_FUNC_EQUAL,
    AM_STENCIL_FUNC_NOTEQUAL,
    AM_STENCIL_FUNC_LESS,
    AM_STENCIL_FUNC_LEQUAL,
    AM_STENCIL_FUNC_GREATER,
    AM_STENCIL_FUNC_GEQUAL,
};

enum am_stencil_op {
    AM_STENCIL_OP_KEEP,
    AM_STENCIL_OP_ZERO,
    AM_STENCIL_OP_REPLACE,
    AM_STENCIL_OP_INCR,
    AM_STENCIL_OP_DECR,
    AM_STENCIL_OP_INVERT,
    AM_STENCIL_OP_INCR_WRAP,
    AM_STENCIL_OP_DECR_WRAP,
};

enum am_stencil_face_side {
    AM_STENCIL_FACE_FRONT,
    AM_STENCIL_FACE_BACK,
};

void am_set_stencil_test_enabled(bool enabled);
void am_set_stencil_func(am_glint ref, am_gluint mask, am_stencil_func func_front, am_stencil_func func_back);
void am_set_stencil_op(am_stencil_face_side face, am_stencil_op fail, am_stencil_op zfail, am_stencil_op zpass);

void am_set_sample_alpha_to_coverage_enabled(bool enabled);
void am_set_sample_coverage_enabled(bool enabled);
void am_set_sample_coverage(float value, bool invert);

// Whole Framebuffer Operations

void am_clear_framebuffer(bool clear_color_buf, bool clear_depth_buf, bool clear_stencil_buf);
void am_set_framebuffer_clear_color(float r, float g, float b, float a);
void am_set_framebuffer_clear_depth(float depth);
void am_set_framebuffer_clear_stencil_val(am_glint val);
void am_set_framebuffer_color_mask(bool r, bool g, bool b, bool a);
void am_set_framebuffer_depth_mask(bool flag);
void am_set_framebuffer_stencil_mask(am_gluint mask);

// Buffer Objects

typedef am_gluint am_buffer_id;

enum am_buffer_target {
    AM_ARRAY_BUFFER,
    AM_ELEMENT_ARRAY_BUFFER,
};

enum am_buffer_usage {
    AM_BUFFER_USAGE_STREAM_DRAW,
    AM_BUFFER_USAGE_STATIC_DRAW,
    AM_BUFFER_USAGE_DYNAMIC_DRAW
};

am_buffer_id am_create_buffer_object();
void am_bind_buffer(am_buffer_target target, am_buffer_id buffer);
void am_set_buffer_data(am_buffer_target target, int size, void *data, am_buffer_usage usage);
void am_set_buffer_sub_data(am_buffer_target target, int offset, int size, void *data);
void am_delete_buffer(am_buffer_id buffer);

// View and Clip

void am_set_depth_range(float near, float far);

void am_set_scissor_test_enabled(bool enabled);
void am_set_scissor(int x, int y, int w, int h);

void am_set_viewport(int x, int y, int w, int h);

// Rasterization

enum am_face_winding {
    AM_FACE_WIND_CW,
    AM_FACE_WIND_CCW,
};

void am_set_front_face_winding(am_face_winding mode);

enum am_cull_face_side {
    AM_CULL_FACE_FRONT,
    AM_CULL_FACE_BACK,
};

void am_set_cull_face_enabled(bool enabled);
void am_set_cull_face_side(am_cull_face_side face);
void am_set_line_width(float w);
void am_set_polygon_offset_fill_enabled(bool enabled);
void am_set_polygon_offset(float factor, float units);

// Dithering

void am_set_dither_enabled(bool enabled);

// Programs and Shaders

typedef am_gluint am_shader_id;
typedef am_gluint am_program_id;

enum am_shader_type {
    AM_VERTEX_SHADER,
    AM_FRAGMENT_SHADER,
};

am_program_id am_create_program();
am_shader_id am_create_shader(am_shader_type type);
// returns false on error and sets msg, line_no and line_str.
// msg and line_str should be freed with free() if set.
bool am_compile_shader(am_shader_id shader, am_shader_type type, const char *src, char **msg, int *line_no, char **line_str);

void am_attach_shader(am_program_id program, am_shader_id shader);
bool am_link_program(am_program_id program);
// returned string should be freed with free()
char *am_get_program_info_log(am_program_id program);

int am_get_program_active_attributes(am_program_id program);
int am_get_program_active_uniforms(am_program_id program);

bool am_validate_program(am_program_id program);
void am_use_program(am_program_id program);

void am_detach_shader(am_program_id program, am_shader_id shader);
void am_delete_shader(am_shader_id shader);
void am_delete_program(am_program_id program);

// Uniforms and Attributes 

enum am_attribute_var_type {
    AM_ATTRIBUTE_VAR_TYPE_FLOAT,
    AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC2,
    AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC3,
    AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC4,
    AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT2,
    AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT3,
    AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT4,
    AM_ATTRIBUTE_VAR_TYPE_UNKNOWN,
};

enum am_uniform_var_type {
    AM_UNIFORM_VAR_TYPE_FLOAT,
    AM_UNIFORM_VAR_TYPE_FLOAT_VEC2,
    AM_UNIFORM_VAR_TYPE_FLOAT_VEC3,
    AM_UNIFORM_VAR_TYPE_FLOAT_VEC4,
    AM_UNIFORM_VAR_TYPE_INT,
    AM_UNIFORM_VAR_TYPE_INT_VEC2,
    AM_UNIFORM_VAR_TYPE_INT_VEC3,
    AM_UNIFORM_VAR_TYPE_INT_VEC4,
    AM_UNIFORM_VAR_TYPE_BOOL,
    AM_UNIFORM_VAR_TYPE_BOOL_VEC2,
    AM_UNIFORM_VAR_TYPE_BOOL_VEC3,
    AM_UNIFORM_VAR_TYPE_BOOL_VEC4,
    AM_UNIFORM_VAR_TYPE_FLOAT_MAT2,
    AM_UNIFORM_VAR_TYPE_FLOAT_MAT3,
    AM_UNIFORM_VAR_TYPE_FLOAT_MAT4,
    AM_UNIFORM_VAR_TYPE_SAMPLER_2D,
    AM_UNIFORM_VAR_TYPE_SAMPLER_CUBE,
    AM_UNIFORM_VAR_TYPE_UNKNOWN,
};

enum am_attribute_client_type {
    AM_ATTRIBUTE_CLIENT_TYPE_BYTE,
    AM_ATTRIBUTE_CLIENT_TYPE_SHORT,
    AM_ATTRIBUTE_CLIENT_TYPE_UBYTE,
    AM_ATTRIBUTE_CLIENT_TYPE_USHORT,
    AM_ATTRIBUTE_CLIENT_TYPE_FLOAT,
};

int am_attribute_client_type_size(am_attribute_client_type t);

void am_set_attribute_array_enabled(am_gluint location, bool enabled);

// *name should be freed with free()
void am_get_active_attribute(am_program_id program, am_gluint index,
    char **name, am_attribute_var_type *type, int *size, am_gluint *loc);
// *name should be freed with free()
void am_get_active_uniform(am_program_id program, am_gluint index,
    char **name, am_uniform_var_type *type, int *size, am_gluint *loc);

void am_set_uniform1f(am_gluint location, float value);
void am_set_uniform2f(am_gluint location, const float *value);
void am_set_uniform3f(am_gluint location, const float *value);
void am_set_uniform4f(am_gluint location, const float *value);
void am_set_uniform1i(am_gluint location, am_glint value);
void am_set_uniform2i(am_gluint location, const am_glint *value);
void am_set_uniform3i(am_gluint location, const am_glint *value);
void am_set_uniform4i(am_gluint location, const am_glint *value);
void am_set_uniform_mat2(am_gluint location, const float *value);
void am_set_uniform_mat3(am_gluint location, const float *value);
void am_set_uniform_mat4(am_gluint location, const float *value);

void am_set_attribute1f(am_gluint location, const float value);
void am_set_attribute2f(am_gluint location, const float *value);
void am_set_attribute3f(am_gluint location, const float *value);
void am_set_attribute4f(am_gluint location, const float *value);

void am_set_attribute_pointer(am_gluint location, int size, am_attribute_client_type type, bool normalized, int stride, int offset);

// Texture Objects

enum am_texture_bind_target {
    AM_TEXTURE_BIND_TARGET_2D,
    AM_TEXTURE_BIND_TARGET_CUBE_MAP,
};

enum am_texture_copy_target {
    AM_TEXTURE_COPY_TARGET_2D,
    AM_TEXTURE_COPY_TARGET_CUBE_MAP_POSITIVE_X,
    AM_TEXTURE_COPY_TARGET_CUBE_MAP_NEGATIVE_X,
    AM_TEXTURE_COPY_TARGET_CUBE_MAP_POSITIVE_Y,
    AM_TEXTURE_COPY_TARGET_CUBE_MAP_NEGATIVE_Y,
    AM_TEXTURE_COPY_TARGET_CUBE_MAP_POSITIVE_Z,
    AM_TEXTURE_COPY_TARGET_CUBE_MAP_NEGATIVE_Z,
};

enum am_texture_format {
    AM_TEXTURE_FORMAT_ALPHA,
    AM_TEXTURE_FORMAT_LUMINANCE,
    AM_TEXTURE_FORMAT_LUMINANCE_ALPHA,
    AM_TEXTURE_FORMAT_RGB,
    AM_TEXTURE_FORMAT_RGBA,
};

enum am_texture_type {
    AM_TEXTURE_TYPE_UBYTE,
    AM_TEXTURE_TYPE_USHORT_5_6_5,
    AM_TEXTURE_TYPE_USHORT_4_4_4_4,
    AM_TEXTURE_TYPE_USHORT_5_5_5_1,
};

enum am_texture_min_filter {
    AM_MIN_FILTER_NEAREST,
    AM_MIN_FILTER_LINEAR,
    AM_MIN_FILTER_NEAREST_MIPMAP_NEAREST,
    AM_MIN_FILTER_LINEAR_MIPMAP_NEAREST,
    AM_MIN_FILTER_NEAREST_MIPMAP_LINEAR,
    AM_MIN_FILTER_LINEAR_MIPMAP_LINEAR,
};

enum am_texture_mag_filter {
    AM_MAG_FILTER_NEAREST,
    AM_MAG_FILTER_LINEAR,
};

enum am_texture_wrap {
    AM_TEXTURE_WRAP_CLAMP_TO_EDGE,
    AM_TEXTURE_WRAP_MIRRORED_REPEAT,
    AM_TEXTURE_WRAP_REPEAT,
};

typedef am_gluint am_texture_id;

void am_set_active_texture_unit(int texture_unit);

am_texture_id am_create_texture();
void am_delete_texture(am_texture_id texture);

void am_bind_texture(am_texture_bind_target target, am_texture_id texture);

void am_copy_texture_image_2d(am_texture_copy_target target, int level, am_texture_format format, int x, int y, int w, int h);
void am_copy_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int x, int y, int w, int h);

void am_generate_mipmap(am_texture_bind_target target);

int am_compute_pixel_size(am_texture_format format, am_texture_type type);

void am_set_texture_image_2d(am_texture_copy_target target, int level, am_texture_format format, int w, int h, am_texture_type type, void *data);
void am_set_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int w, int h, am_texture_format format, am_texture_type type, void *data);

void am_set_texture_min_filter(am_texture_bind_target target, am_texture_min_filter filter);
void am_set_texture_mag_filter(am_texture_bind_target target, am_texture_mag_filter filter);
void am_set_texture_wrap(am_texture_bind_target target, am_texture_wrap s_wrap, am_texture_wrap t_wrap);

// Renderbuffer Objects

enum am_renderbuffer_format {
    AM_RENDERBUFFER_FORMAT_DEPTH,
    AM_RENDERBUFFER_FORMAT_STENCIL,
    AM_RENDERBUFFER_FORMAT_DEPTHSTENCIL,
};

typedef am_gluint am_renderbuffer_id;

am_renderbuffer_id am_create_renderbuffer();
void am_delete_renderbuffer(am_renderbuffer_id rb);

void am_bind_renderbuffer(am_renderbuffer_id rb);
void am_set_renderbuffer_storage(am_renderbuffer_format format, int w, int h);

// Read Back Pixels 

// pixels will be in format RGBA, type UNSIGNED_BYTE.
void am_read_pixels(int x, int y, int w, int h, void *data);

// Framebuffer Objects

enum am_framebuffer_status {
    AM_FRAMEBUFFER_STATUS_COMPLETE,
    AM_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT,
    AM_FRAMEBUFFER_STATUS_INCOMPLETE_DIMENSIONS,
    AM_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT,
    AM_FRAMEBUFFER_STATUS_UNSUPPORTED,
    AM_FRAMEBUFFER_STATUS_UNKNOWN,
};

enum am_framebuffer_attachment {
    AM_FRAMEBUFFER_COLOR_ATTACHMENT0,
    AM_FRAMEBUFFER_DEPTH_ATTACHMENT,
    AM_FRAMEBUFFER_STENCIL_ATTACHMENT,
    AM_FRAMEBUFFER_DEPTHSTENCIL_ATTACHMENT,
};

typedef am_gluint am_framebuffer_id;

am_framebuffer_id am_create_framebuffer();
void am_delete_framebuffer(am_framebuffer_id fb);

void am_bind_framebuffer(am_framebuffer_id fb);
am_framebuffer_status am_check_framebuffer_status();

void am_set_framebuffer_renderbuffer(am_framebuffer_attachment attachment, am_renderbuffer_id rb);
void am_set_framebuffer_texture2d(am_framebuffer_attachment attachment, am_texture_copy_target target, am_texture_id texture);

// Writing to the Draw Buffer

enum am_draw_mode {
    AM_DRAWMODE_POINTS,
    AM_DRAWMODE_LINES,
    AM_DRAWMODE_LINE_STRIP,
    AM_DRAWMODE_LINE_LOOP,
    AM_DRAWMODE_TRIANGLES,
    AM_DRAWMODE_TRIANGLE_STRIP,
    AM_DRAWMODE_TRIANGLE_FAN,
};

enum am_element_index_type {
    AM_ELEMENT_TYPE_USHORT,
    AM_ELEMENT_TYPE_UINT,
};

void am_draw_arrays(am_draw_mode mode, int first, int count);
void am_draw_elements(am_draw_mode mode, int count, am_element_index_type type, int offset);

// Other

void am_gl_end_framebuffer_render();
void am_gl_end_frame(bool present);

void am_log_gl(const char *msg);
void am_close_gllog();
void am_reset_gl_frame_stats();

bool am_gl_requires_combined_depthstencil();
