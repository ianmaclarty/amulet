#include "amulet.h"
#ifdef AM_USE_METAL

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "glsl_optimizer.h"

static void get_src_error_line(char *errmsg, const char *src, int *line_no, char **line_str);

template<typename T>
union freelist_item {
    int next;
    T item;
};

template<typename T>
struct freelist {
    std::vector<freelist_item<T> > items;
    int first;
    freelist() {
        first = 0;
    }
    int add(T item) {
        if (first == 0) {
            freelist_item<T> it;
            it.item = item;
            items.push_back(it);
            return items.size();
        } else {
            int next = items[first-1].next;
            int index = first;
            items[first-1].item = item;
            first = next;
            return index;
        }
    }
    void remove(int index) {
        items[index-1].next = first;
        first = index;
    }
    T* get(int index) {
        return &items[index-1].item;   
    }
    void clear() {
        first = 0;
        items.clear();
    }
};

int am_max_combined_texture_image_units;
int am_max_cube_map_texture_size;
int am_max_fragment_uniform_vectors;
int am_max_renderbuffer_size;
int am_max_texture_image_units;
int am_max_texture_size;
int am_max_varying_vectors;
int am_max_vertex_attribs;
int am_max_vertex_texture_image_units;
int am_max_vertex_uniform_vectors;
int am_frame_draw_calls;
int am_frame_use_program_calls;

NSWindow *am_metal_window = nil;
bool am_metal_use_highdpi = false;

@interface MetalView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                        scale:(CGFloat)scale;

@end

@implementation MetalView

+ (Class)layerClass
{
    return NSClassFromString(@"CAMetalLayer");
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (CALayer*)makeBackingLayer
{
    return [self.class.layerClass layer];
}

- (instancetype)initWithFrame:(NSRect)frame
                        scale:(CGFloat)scale
{
    if ((self = [super initWithFrame:frame])) {
        self.wantsLayer = YES;
        self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        self.layer.contentsScale = scale;
    }
  
    return self;
}
- (void)resizeWithOldSuperviewSize:(NSSize)oldSize
{
    [super resizeWithOldSuperviewSize:oldSize];
}

@end

static MetalView* metal_view = nil;
static id<MTLDevice> metal_device = nil;
static id <MTLRenderCommandEncoder> metal_encoder = nil;
static id <MTLCommandBuffer> metal_command_buffer = nil;
static id <MTLCommandQueue> metal_queue = nil;
static CAMetalLayer *metal_layer = nil;
static id<CAMetalDrawable> metal_active_drawable = nil;
static am_render_state *rstate = NULL;

static am_framebuffer_id metal_active_framebuffer = 0;

struct metal_framebuffer {
    float clear_r;
    float clear_g;
    float clear_b;
    float clear_a;
};

static freelist<metal_framebuffer> metal_framebuffer_freelist;

static metal_framebuffer default_metal_framebuffer;

static metal_framebuffer *get_metal_framebuffer(int id) {
    if (id == 0) {
        return &default_metal_framebuffer;
    } else {
        return metal_framebuffer_freelist.get(id);
    }
}

static id<CAMetalDrawable> get_active_metal_drawable() {
    if (metal_active_drawable != nil) return metal_active_drawable;
    metal_active_drawable = [metal_layer nextDrawable];
    return metal_active_drawable;
}

static MTLRenderPassDescriptor *make_active_framebuffer_render_desc() {
    metal_framebuffer *fb = get_metal_framebuffer(metal_active_framebuffer);
    MTLRenderPassDescriptor *renderdesc = [MTLRenderPassDescriptor renderPassDescriptor];
    MTLRenderPassColorAttachmentDescriptor *colorattachment = renderdesc.colorAttachments[0];

    /* Clear to a red-orange color when beginning the render pass. */
    colorattachment.clearColor  = MTLClearColorMake(fb->clear_r, fb->clear_g, fb->clear_b, fb->clear_a);
    colorattachment.loadAction  = MTLLoadActionLoad;
    colorattachment.storeAction = MTLStoreActionStore;

    return renderdesc;
}

static glslopt_ctx* glslopt_context = NULL;

struct metal_shader {
    id<MTLLibrary> lib;
    id<MTLFunction> func;
    am_shader_type type;
    glslopt_shader *gshader;
    int gshader_refs;
};

static freelist<metal_shader> metal_shader_freelist;

static void metal_shader_init(metal_shader *shader, am_shader_type type) {
    shader->type = type;
    shader->lib = nil;
    shader->func = nil;
    shader->gshader = NULL;
    shader->gshader_refs = 0;
}

static void metal_shader_retain(metal_shader *shader) {
    if (shader->lib != nil) [shader->lib retain];
    if (shader->func != nil) [shader->func retain];
    if (shader->gshader != NULL) shader->gshader_refs++;
}

static void metal_shader_release(metal_shader *shader) {
    if (shader->lib != nil) [shader->lib release];
    if (shader->func != nil) [shader->func release];
    if (shader->gshader != NULL) {
        shader->gshader_refs--;
        if (shader->gshader_refs <= 0) {
            glslopt_shader_delete(shader->gshader);
        }
    }
}

struct metal_program {
    metal_shader vert_shader;
    metal_shader frag_shader;
};

static freelist<metal_program> metal_program_freelist;

static void create_new_metal_encoder(bool clear_color_buf, bool clear_depth_buf, bool clear_stencil_buf) {
    if (metal_encoder != nil) {
        // in metal clear can only be done when creating the encoder,
        // so if there is already an encoder finish it and create a new one.
        [metal_encoder endEncoding];

        // XXX auto-releases?
        metal_encoder = nil;
    }
    if (metal_command_buffer == nil) {
        metal_command_buffer = [metal_queue commandBuffer];
    }

    MTLRenderPassDescriptor *renderdesc = make_active_framebuffer_render_desc();
    if (clear_color_buf) {
        renderdesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    }
    if (metal_active_framebuffer == 0) {
        renderdesc.colorAttachments[0].texture = get_active_metal_drawable().texture;
    }
    metal_encoder = [metal_command_buffer renderCommandEncoderWithDescriptor:renderdesc];
}

static void init_metal_view() {
    if (am_metal_window == nil) return;

    NSView *view = am_metal_window.contentView;
    CGFloat scale = 1.0;
    if (am_metal_use_highdpi) {
        if ([am_metal_window.screen respondsToSelector:@selector(backingScaleFactor)]) {
            scale = am_metal_window.screen.backingScaleFactor;
        }
    }
    metal_view = [[MetalView alloc] initWithFrame:view.frame scale:scale];
    [view addSubview:metal_view];
}

#define check_initialized(...) {if (!metal_initialized) {am_log1("%s:%d: attempt to call %s before metal initialized", __FILE__, __LINE__, __func__); return __VA_ARGS__;}}

static bool metal_initialized = false;

void am_init_gl() {
    metal_device = MTLCreateSystemDefaultDevice();
    [metal_device retain];
    if (metal_device == nil) {
        am_log0("%s", "unable to create metal device");
        return;
    }
    init_metal_view();
    if (metal_view == nil) {
        am_log0("%s", "unable to create metal view");
        return;
    }

    metal_layer = (CAMetalLayer *)[metal_view layer];
    metal_layer.device = metal_device;
    //metal_layer.displaySyncEnabled = (am_conf_vsync ? YES : NO);

    metal_queue = [metal_device newCommandQueue];
    [metal_queue retain];

    default_metal_framebuffer.clear_r = 0.0f;
    default_metal_framebuffer.clear_g = 0.0f;
    default_metal_framebuffer.clear_b = 0.0f;
    default_metal_framebuffer.clear_a = 1.0f;

    glslopt_context = glslopt_initialize(kGlslTargetMetal);
    if (glslopt_context == NULL) {
        am_abort("unable to initialize glsl optimizer");
    }

    // TODO
    am_max_combined_texture_image_units = 0;
    am_max_cube_map_texture_size = 0;
    am_max_fragment_uniform_vectors = 0;
    am_max_renderbuffer_size = 0;
    am_max_texture_image_units = 0;
    am_max_texture_size = 0;
    am_max_varying_vectors = 0;
    am_max_vertex_attribs = 0;
    am_max_vertex_texture_image_units = 0;
    am_max_vertex_uniform_vectors = 0;

    am_frame_draw_calls = 0;
    am_frame_use_program_calls = 0;

    rstate = am_global_render_state;

    metal_initialized = true;
}

bool am_gl_is_initialized() {
    return metal_initialized;
}

void am_close_gllog() {
}

void am_destroy_gl() {
    rstate = NULL;
    metal_command_buffer = nil;
    metal_encoder = nil;
    metal_active_framebuffer = 0;
    [metal_queue release];
    [metal_device release];

    glslopt_cleanup(glslopt_context);

    metal_initialized = false;
}

// Per-Fragment Operations

void am_set_blend_enabled(bool enabled) {
    check_initialized();
    // TODO
}

void am_set_blend_color(float r, float g, float b, float a) {
    check_initialized();
    // TODO
}

void am_set_blend_equation(am_blend_equation rgb, am_blend_equation alpha) {
    check_initialized();
    // TODO
}

void am_set_blend_func(am_blend_sfactor src_rgb, am_blend_dfactor dst_rgb, am_blend_sfactor src_alpha, am_blend_dfactor dst_alpha) {
    check_initialized();
    // TODO
}

void am_set_depth_test_enabled(bool enabled) {
    check_initialized();
    // TODO
}

void am_set_depth_func(am_depth_func func) {
    check_initialized();
    // TODO
}

void am_set_stencil_test_enabled(bool enabled) {
    check_initialized();
    // TODO
}

void am_set_stencil_func(am_stencil_face_side face, am_stencil_func func, am_glint ref, am_gluint mask) {
    check_initialized();
    // TODO
}

void am_set_stencil_op(am_stencil_face_side face, am_stencil_op fail, am_stencil_op zfail, am_stencil_op zpass) {
    check_initialized();
    // TODO
}

void am_set_sample_alpha_to_coverage_enabled(bool enabled) {
    check_initialized();
    // TODO
}

void am_set_sample_coverage_enabled(bool enabled) {
    check_initialized();
    // TODO
}

void am_set_sample_coverage(float value, bool invert) {
    check_initialized();
    // TODO
}

// Whole Framebuffer Operations

void am_clear_framebuffer(bool clear_color_buf, bool clear_depth_buf, bool clear_stencil_buf) {
    check_initialized();
    create_new_metal_encoder(clear_color_buf, clear_depth_buf, clear_stencil_buf);
}

void am_set_framebuffer_clear_color(float r, float g, float b, float a) {
    check_initialized();
    metal_framebuffer *fb = get_metal_framebuffer(metal_active_framebuffer);
    fb->clear_r = r;
    fb->clear_g = g;
    fb->clear_b = b;
    fb->clear_a = a;
}

void am_set_framebuffer_clear_depth(float depth) {
    check_initialized();
    // TODO
}

void am_set_framebuffer_clear_stencil_val(am_glint val) {
    check_initialized();
    // TODO
}

void am_set_framebuffer_color_mask(bool r, bool g, bool b, bool a) {
    check_initialized();
    // TODO
}

void am_set_framebuffer_depth_mask(bool flag) {
    check_initialized();
    // TODO
}

void am_set_framebuffer_stencil_mask(am_stencil_face_side face, am_gluint mask) {
    check_initialized();
    // TODO
}

// Buffer Objects

am_buffer_id am_create_buffer_object() {
    check_initialized(0);
    // XXX
    return 0;
}

void am_bind_buffer(am_buffer_target target, am_buffer_id buffer) {
    check_initialized();
    // XXX
}

void am_set_buffer_data(am_buffer_target target, int size, void *data, am_buffer_usage usage) {
    check_initialized();
    // XXX
}

void am_set_buffer_sub_data(am_buffer_target target, int offset, int size, void *data) {
    check_initialized();
    // XXX
}

void am_delete_buffer(am_buffer_id buffer) {
    check_initialized();
    // XXX
}

// View and Clip

void am_set_depth_range(float near, float far) {
    check_initialized();
    // TODO
}

void am_set_scissor_test_enabled(bool enabled) {
    check_initialized();
    // TODO
}

void am_set_scissor(int x, int y, int w, int h) {
    check_initialized();
    // TODO
}

void am_set_viewport(int x, int y, int w, int h) {
    check_initialized();
    // XXX
}

// Rasterization

void am_set_front_face_winding(am_face_winding mode) {
    check_initialized();
    // TODO
}

void am_set_cull_face_enabled(bool enabled) {
    check_initialized();
    // TODO
}

void am_set_cull_face_side(am_cull_face_side face) {
    check_initialized();
    // TODO
}

void am_set_line_width(float width) {
    check_initialized();
    // TODO
}

void am_set_polygon_offset_fill_enabled(bool enabled) {
    check_initialized();
    // TODO
}

void am_set_polygon_offset(float factor, float units) {
    check_initialized();
    // TODO
}

// Dithering

void am_set_dither_enabled(bool enabled) {
    check_initialized();
    // TODO
}

// Programs and Shaders

am_program_id am_create_program() {
    check_initialized(0);
    metal_program prog;
    metal_shader_init(&prog.vert_shader, AM_VERTEX_SHADER);
    metal_shader_init(&prog.frag_shader, AM_FRAGMENT_SHADER);
    return metal_program_freelist.add(prog);
}

am_shader_id am_create_shader(am_shader_type type) {
    check_initialized(0);
    metal_shader shader;
    metal_shader_init(&shader, type);
    return metal_shader_freelist.add(shader);
}

bool am_compile_shader(am_shader_id id, am_shader_type type, const char *src, char **msg, int *line_no, char **line_str) {
    check_initialized(false);
    metal_shader *shader = metal_shader_freelist.get(id);
    *msg = NULL;
    *line_no = 0;
    *line_str = NULL;
    glslopt_shader_type gtype;
    switch (type) {
        case AM_VERTEX_SHADER: gtype = kGlslOptShaderVertex; break;
        case AM_FRAGMENT_SHADER: gtype = kGlslOptShaderFragment; break;
    }
    shader->gshader = glslopt_optimize(glslopt_context, gtype, src, 0);
    bool success;
    if (glslopt_get_status(shader->gshader)) {
        NSError *error = nil;
        const char *metal_src = glslopt_get_output(shader->gshader);
        am_debug("metal: %s", metal_src);
        shader->lib = [metal_device newLibraryWithSource:[NSString stringWithUTF8String:metal_src] options: nil error:&error];
        if (shader->lib == nil) {
            *msg = am_format("%s", [[error localizedDescription] UTF8String]);
            get_src_error_line(*msg, src, line_no, line_str);
            success = false;
        } else {
            shader->func = [shader->lib newFunctionWithName:@"xlatMtlMain"];
            metal_shader_retain(shader);
            success = true;
        }
    } else {
        *msg = am_format("%s", glslopt_get_log(shader->gshader));
        get_src_error_line(*msg, src, line_no, line_str);
        success = false;
    }
    return success;
}

void am_attach_shader(am_program_id program_id, am_shader_id shader_id) {
    check_initialized();
    metal_program *prog = metal_program_freelist.get(program_id);
    metal_shader *shader = metal_shader_freelist.get(shader_id);
    switch (shader->type) {
        case AM_VERTEX_SHADER: 
            prog->vert_shader = *shader;
            break;
        case AM_FRAGMENT_SHADER:
            prog->frag_shader = *shader;
            break;
    }
    metal_shader_retain(shader);
}

bool am_link_program(am_program_id program) {
    check_initialized(false);
    // XXX
    return true;
}

char *am_get_program_info_log(am_program_id program) {
    check_initialized(NULL);
    // XXX
    return NULL;
}

int am_get_program_active_attributes(am_program_id program) {
    check_initialized(0);
    // XXX
    return 0;
}

int am_get_program_active_uniforms(am_program_id program) {
    check_initialized(0);
    // XXX
    return 0;
}

bool am_validate_program(am_program_id program) {
    check_initialized(false);
    // TODO
    return true;
}

void am_use_program(am_program_id program) {
    check_initialized();
    // XXX
}

void am_detach_shader(am_program_id program, am_shader_id shader) {
    check_initialized();
    // nop
}

void am_delete_shader(am_shader_id id) {
    check_initialized();
    metal_shader *shader = metal_shader_freelist.get(id);
    metal_shader_release(shader);
    metal_shader_freelist.remove(id);
}

void am_delete_program(am_program_id id) {
    check_initialized();
    metal_program *prog = metal_program_freelist.get(id);
    metal_shader_release(&prog->vert_shader);
    metal_shader_release(&prog->frag_shader);
    metal_program_freelist.remove(id);
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
    // XXX
}

am_gluint am_get_attribute_location(am_program_id program, const char *name) {
    check_initialized(0);
    // XXX
    return 0;
}

am_gluint am_get_uniform_location(am_program_id program, const char *name) {
    check_initialized(0);
    // XXX
    return 0;
}

void am_get_active_attribute(am_program_id program, am_gluint index,
    char **name, am_attribute_var_type *type, int *size)
{
    check_initialized();
    // XXX
}

void am_get_active_uniform(am_program_id program, am_gluint index,
    char **name, am_uniform_var_type *type, int *size)
{
    check_initialized();
    // XXX
}

void am_set_uniform1f(am_gluint location, float value) {
    check_initialized();
    // XXX
}

void am_set_uniform2f(am_gluint location, const float *value) {
    check_initialized();
    // XXX
}

void am_set_uniform3f(am_gluint location, const float *value) {
    check_initialized();
    // XXX
}

void am_set_uniform4f(am_gluint location, const float *value) {
    check_initialized();
    // XXX
}

void am_set_uniform1i(am_gluint location, am_glint value) {
    check_initialized();
    // XXX
}

void am_set_uniform2i(am_gluint location, const am_glint *value) {
    check_initialized();
    // XXX
}

void am_set_uniform3i(am_gluint location, const am_glint *value) {
    check_initialized();
    // XXX
}

void am_set_uniform4i(am_gluint location, const am_glint *value) {
    check_initialized();
    // XXX
}

void am_set_uniform_mat2(am_gluint location, const float *value) {
    check_initialized();
    // XXX
}

void am_set_uniform_mat3(am_gluint location, const float *value) {
    check_initialized();
    // XXX
}

void am_set_uniform_mat4(am_gluint location, const float *value) {
    check_initialized();
    // XXX
}

void am_set_attribute1f(am_gluint location, const float value) {
    check_initialized();
    // XXX
}

void am_set_attribute2f(am_gluint location, const float *value) {
    check_initialized();
    // XXX
}

void am_set_attribute3f(am_gluint location, const float *value) {
    check_initialized();
    // XXX
}

void am_set_attribute4f(am_gluint location, const float *value) {
    check_initialized();
    // XXX
}

void am_set_attribute_pointer(am_gluint location, int size, am_attribute_client_type type, bool normalized, int stride, int offset) {
    check_initialized();
    // XXX
}

// Texture Objects

void am_set_active_texture_unit(int texture_unit) {
    check_initialized();
    // TODO
}

am_texture_id am_create_texture() {
    check_initialized(0);
    // TODO
    return 0;
}

void am_delete_texture(am_texture_id texture) {
    check_initialized();
    // TODO
}

void am_bind_texture(am_texture_bind_target target, am_texture_id texture) {
    check_initialized();
    // TODO
}

void am_copy_texture_image_2d(am_texture_copy_target target, int level, am_texture_format format, int x, int y, int w, int h) {
    check_initialized();
    // TODO
}

void am_copy_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int x, int y, int w, int h) {
    check_initialized();
    // TODO
}

void am_generate_mipmap(am_texture_bind_target target) {
    check_initialized();
    // TODO
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
    // TODO
}

void am_set_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int w, int h, am_texture_format format, am_texture_type type, void *data) {
    check_initialized();
    // TODO
}

void am_set_texture_min_filter(am_texture_bind_target target, am_texture_min_filter filter) {
    check_initialized();
    // TODO
}

void am_set_texture_mag_filter(am_texture_bind_target target, am_texture_mag_filter filter) {
    check_initialized();
    // TODO
}

void am_set_texture_wrap(am_texture_bind_target target, am_texture_wrap s_wrap, am_texture_wrap t_wrap) {
    check_initialized();
    // TODO
}

// Renderbuffer Objects

am_renderbuffer_id am_create_renderbuffer() {
    check_initialized(0);
    // TODO
    return 0;
}

void am_delete_renderbuffer(am_renderbuffer_id rb) {
    check_initialized();
    // TODO
}

void am_bind_renderbuffer(am_renderbuffer_id rb) {
    check_initialized();
    // TODO
}

void am_set_renderbuffer_storage(am_renderbuffer_format format, int w, int h) {
    check_initialized();
    // TODO
}

// Read Back Pixels 

void am_read_pixels(int x, int y, int w, int h, void *data) {
    check_initialized();
    // TODO
}

// Framebuffer Objects

am_framebuffer_id am_create_framebuffer() {
    check_initialized(0);
    // TODO
    return 0;
}

void am_delete_framebuffer(am_framebuffer_id fb) {
    check_initialized();
    // TODO
}

void am_bind_framebuffer(am_framebuffer_id fb) {
    check_initialized();
    if (metal_active_framebuffer != fb) {
        if (metal_encoder != nil) {
            [metal_encoder endEncoding];

            // XXX auto-releases?
            metal_encoder = nil;
        }
    }
    metal_active_framebuffer = fb;
    if (metal_encoder == nil) {
        create_new_metal_encoder(false, false, false);
    }
}

am_framebuffer_status am_check_framebuffer_status() {
    check_initialized(AM_FRAMEBUFFER_STATUS_UNKNOWN);
    // TODO
    return AM_FRAMEBUFFER_STATUS_COMPLETE;
}

void am_set_framebuffer_renderbuffer(am_framebuffer_attachment attachment, am_renderbuffer_id rb) {
    check_initialized();
    // TODO
}

void am_set_framebuffer_texture2d(am_framebuffer_attachment attachment, am_texture_copy_target target, am_texture_id texture) {
    check_initialized();
    // TODO
}

// Writing to the Draw Buffer

void am_draw_arrays(am_draw_mode mode, int first, int count) {
    check_initialized();
    // XXX
}

void am_draw_elements(am_draw_mode mode, int count, am_element_index_type type, int offset) {
    check_initialized();
    // XXX
}

void am_gl_flush() {
    check_initialized();
    // XXX
}

void am_gl_end_drawing() {
    check_initialized();
    if (metal_encoder != nil) {
        [metal_encoder endEncoding];

        // XXX auto-releases?
        metal_encoder = nil;
    }
    if (metal_command_buffer != nil) {
        if (metal_active_framebuffer == 0) {
            [metal_command_buffer presentDrawable:get_active_metal_drawable()];
        }
        [metal_command_buffer commit];

        // XXX auto-releases?
        metal_command_buffer = nil;
        if (metal_active_framebuffer == 0) {
            metal_active_drawable = nil;
        }
    }
}

void am_log_gl(const char *msg) {
}

void am_reset_gl_frame_stats() {
    am_frame_draw_calls = 0;
    am_frame_use_program_calls = 0;
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

#endif // AM_USE_METAL
