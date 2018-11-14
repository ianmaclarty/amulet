#include "amulet.h"
#ifdef AM_USE_METAL

// https://developer.apple.com/documentation/metal?language=objc

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "glsl_optimizer.h"

static void get_src_error_line(char *errmsg, const char *src, int *line_no, char **line_str);

template<typename T>
struct freelist_item {
    int next;
    T item;
};

template<typename T>
struct freelist {
    std::vector<freelist_item<T> > items;
    am_gluint first;
    freelist() {
        first = 0;
    }
    am_gluint add(T item) {
        if (first == 0) {
            freelist_item<T> it;
            it.item = item;
            items.push_back(it);
            return items.size();
        } else {
            am_gluint next = items[first-1].next;
            am_gluint index = first;
            items[first-1].item = item;
            first = next;
            return index;
        }
    }
    void remove(am_gluint index) {
        items[index-1].next = first;
        first = index;
    }
    T* get(am_gluint index) {
        return &items[index-1].item;   
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
bool am_metal_window_depth_buffer = false;

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

static am_framebuffer_id metal_bound_framebuffer = 0;
static am_framebuffer_id metal_active_framebuffer = 0;

struct metal_framebuffer {
    bool complete;
    int width;
    int height;
    float clear_r;
    float clear_g;
    float clear_b;
    float clear_a;
    id<MTLTexture> color_texture; // nil for default framebuffer
    id<MTLTexture> depth_texture;
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

struct metal_renderbuffer {
    bool initialized;
    am_renderbuffer_format format;
    int width;
    int height;
    id<MTLTexture> texture;
};

int metal_bound_renderbuffer = 0;

static freelist<metal_renderbuffer> metal_renderbuffer_freelist;

static id<CAMetalDrawable> get_active_metal_drawable() {
    if (metal_active_drawable != nil) return metal_active_drawable;
    metal_active_drawable = [metal_layer nextDrawable];
    return metal_active_drawable;
}

static MTLRenderPassDescriptor *make_active_framebuffer_render_desc() {
    metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
    if (!fb->complete) return NULL;
    MTLRenderPassDescriptor *renderdesc = [MTLRenderPassDescriptor renderPassDescriptor];
    MTLRenderPassColorAttachmentDescriptor *colorattachment = renderdesc.colorAttachments[0];

    if (fb->color_texture != nil) {
        colorattachment.texture = fb->color_texture;
    } else {
        colorattachment.texture = get_active_metal_drawable().texture;
    }
    colorattachment.clearColor  = MTLClearColorMake(fb->clear_r, fb->clear_g, fb->clear_b, fb->clear_a);
    colorattachment.loadAction  = MTLLoadActionLoad;
    colorattachment.storeAction = MTLStoreActionStore;

    if (fb->depth_texture != nil) {
        renderdesc.depthAttachment.texture = fb->depth_texture;
    }

    return renderdesc;
}

static glslopt_ctx* glslopt_context = NULL;

struct metal_shader {
    am_shader_type type;
    int ref_count;

    id<MTLLibrary> lib;
    id<MTLFunction> func;
    glslopt_shader *gshader;
};

static freelist<metal_shader> metal_shader_freelist;

static void metal_shader_init(metal_shader *shader, am_shader_type type) {
    shader->type = type;
    shader->ref_count = 0;

    shader->lib = nil;
    shader->func = nil;
    shader->gshader = NULL;
}

static void metal_shader_retain(metal_shader *shader) {
    if (shader->lib != nil) [shader->lib retain];
    if (shader->func != nil) [shader->func retain];
    shader->ref_count++;
}

static void metal_shader_release(metal_shader *shader) {
    if (shader->lib != nil) [shader->lib release];
    if (shader->func != nil) [shader->func release];
    shader->ref_count--;
    if (shader->ref_count <= 0) {
        if (shader->gshader != NULL) glslopt_shader_delete(shader->gshader);
    }
}

struct uniform_descr {
    am_uniform_var_type type;
    int vert_location;
    int frag_location;
    int tex_unit;
    const char *name;
};

struct attr_layout {
    am_attribute_client_type type;
    int dims;
    bool normalized;
    int stride;
};

struct attr_descr {
    am_attribute_var_type type;
    int location;
    const char *name;
};

struct attr_state {
    attr_descr descr;
    am_buffer_id bufid;
    int offset;
    attr_layout layout;
};

struct tex_uniform_descr {
    int location;
};

struct metal_pipeline {
    attr_layout *attr_layouts;
    bool blend_enabled;
    am_blend_equation blend_eq_rgb;
    am_blend_equation blend_eq_alpha;
    am_blend_sfactor blend_sfactor_rgb;
    am_blend_sfactor blend_sfactor_alpha;
    am_blend_dfactor blend_dfactor_rgb;
    am_blend_dfactor blend_dfactor_alpha;
    id<MTLRenderPipelineState> mtlpipeline;

    bool depth_test_enabled;
    bool depth_mask;
    am_depth_func depth_func;
    id<MTLDepthStencilState> mtldepthstencilstate;

    bool face_culling_enabled;
    am_cull_face_side face_cull_side;
    am_face_winding face_winding;
};

struct metal_program {
    metal_shader vert_shader;
    metal_shader frag_shader;
    int vert_uniform_size;
    uint8_t *vert_uniform_data;
    int frag_uniform_size;
    uint8_t *frag_uniform_data;
    int num_uniforms;
    uniform_descr *uniforms;
    int num_tex_uniforms;
    tex_uniform_descr *tex_uniforms;
    int num_attrs;
    attr_state *attrs;
    std::vector<metal_pipeline> pipeline_cache;
    char *log;
};

static freelist<metal_program> metal_program_freelist;

static am_program_id metal_active_program = 0;

static int metal_active_pipeline = -1;

struct metal_buffer {
    int size;
    id<MTLBuffer> mtlbuf;
};

static freelist<metal_buffer> metal_buffer_freelist;

static am_buffer_id metal_active_array_buffer = 0;
static am_buffer_id metal_active_element_buffer = 0;

struct metal_texture {
    id<MTLTexture> tex;
    id<MTLSamplerState> sampler;
};

static freelist<metal_texture> metal_texture_freelist;

static am_texture_id metal_active_texture = 0;

#define NUM_TEXTURE_UNITS 32
static am_texture_id texture_units[NUM_TEXTURE_UNITS];
static int active_texture_unit = -1;

static bool metal_blend_enabled;
static am_blend_equation metal_blend_eq_rgb;
static am_blend_equation metal_blend_eq_alpha;
static am_blend_sfactor metal_blend_sfactor_rgb;
static am_blend_sfactor metal_blend_sfactor_alpha;
static am_blend_dfactor metal_blend_dfactor_rgb;
static am_blend_dfactor metal_blend_dfactor_alpha;

static bool metal_depth_test_enabled;
static bool metal_depth_mask;
static am_depth_func metal_depth_func;

static bool metal_face_culling_enabled;
static am_cull_face_side metal_face_cull_side;
static am_face_winding metal_face_winding;

static BOOL to_objc_bool(bool b) {
    return b ? YES : NO;
}

static MTLBlendFactor to_metal_blend_sfactor(am_blend_sfactor f) {
    switch (f) {
        case AM_BLEND_SFACTOR_ZERO: return MTLBlendFactorZero;
        case AM_BLEND_SFACTOR_ONE: return MTLBlendFactorOne;
        case AM_BLEND_SFACTOR_SRC_COLOR: return MTLBlendFactorSourceColor;
        case AM_BLEND_SFACTOR_ONE_MINUS_SRC_COLOR: return MTLBlendFactorOneMinusSourceColor;
        case AM_BLEND_SFACTOR_DST_COLOR: return MTLBlendFactorDestinationColor;
        case AM_BLEND_SFACTOR_ONE_MINUS_DST_COLOR: return MTLBlendFactorOneMinusDestinationColor;
        case AM_BLEND_SFACTOR_SRC_ALPHA: return MTLBlendFactorSourceAlpha;
        case AM_BLEND_SFACTOR_ONE_MINUS_SRC_ALPHA: return MTLBlendFactorOneMinusSourceAlpha;
        case AM_BLEND_SFACTOR_DST_ALPHA: return MTLBlendFactorDestinationAlpha;
        case AM_BLEND_SFACTOR_ONE_MINUS_DST_ALPHA: return MTLBlendFactorOneMinusDestinationAlpha;
        case AM_BLEND_SFACTOR_CONSTANT_COLOR: return MTLBlendFactorBlendColor;
        case AM_BLEND_SFACTOR_ONE_MINUS_CONSTANT_COLOR: return MTLBlendFactorOneMinusBlendColor;
        case AM_BLEND_SFACTOR_CONSTANT_ALPHA: return MTLBlendFactorBlendAlpha;
        case AM_BLEND_SFACTOR_ONE_MINUS_CONSTANT_ALPHA: return MTLBlendFactorOneMinusBlendAlpha;
        case AM_BLEND_SFACTOR_SRC_ALPHA_SATURATE: return MTLBlendFactorSourceAlphaSaturated;
    }
    return MTLBlendFactorZero;
}

static MTLBlendFactor to_metal_blend_dfactor(am_blend_dfactor f) {
    switch (f) {
        case AM_BLEND_DFACTOR_ZERO: return MTLBlendFactorZero;
        case AM_BLEND_DFACTOR_ONE: return MTLBlendFactorOne;
        case AM_BLEND_DFACTOR_SRC_COLOR: return MTLBlendFactorSourceColor;
        case AM_BLEND_DFACTOR_ONE_MINUS_SRC_COLOR: return MTLBlendFactorOneMinusSourceColor;
        case AM_BLEND_DFACTOR_DST_COLOR: return MTLBlendFactorDestinationColor;
        case AM_BLEND_DFACTOR_ONE_MINUS_DST_COLOR: return MTLBlendFactorOneMinusDestinationColor;
        case AM_BLEND_DFACTOR_SRC_ALPHA: return MTLBlendFactorSourceAlpha;
        case AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA: return MTLBlendFactorOneMinusSourceAlpha;
        case AM_BLEND_DFACTOR_DST_ALPHA: return MTLBlendFactorDestinationAlpha;
        case AM_BLEND_DFACTOR_ONE_MINUS_DST_ALPHA: return MTLBlendFactorOneMinusDestinationAlpha;
        case AM_BLEND_DFACTOR_CONSTANT_COLOR: return MTLBlendFactorBlendColor;
        case AM_BLEND_DFACTOR_ONE_MINUS_CONSTANT_COLOR: return MTLBlendFactorOneMinusBlendColor;
        case AM_BLEND_DFACTOR_CONSTANT_ALPHA: return MTLBlendFactorBlendAlpha;
        case AM_BLEND_DFACTOR_ONE_MINUS_CONSTANT_ALPHA: return MTLBlendFactorOneMinusBlendAlpha;
    }
    return MTLBlendFactorZero;
}

static MTLBlendOperation to_metal_blend_op(am_blend_equation e) {
    switch (e) {
        case AM_BLEND_EQUATION_ADD: return MTLBlendOperationAdd;
        case AM_BLEND_EQUATION_SUBTRACT: return MTLBlendOperationSubtract;
        case AM_BLEND_EQUATION_REVERSE_SUBTRACT: return MTLBlendOperationReverseSubtract;
    }
    return MTLBlendOperationAdd;
}

static MTLCompareFunction to_metal_depth_func(am_depth_func f) {
    switch (f) {
        case AM_DEPTH_FUNC_NEVER: return MTLCompareFunctionNever;
        case AM_DEPTH_FUNC_ALWAYS: return MTLCompareFunctionAlways;
        case AM_DEPTH_FUNC_EQUAL: return MTLCompareFunctionEqual;
        case AM_DEPTH_FUNC_NOTEQUAL: return MTLCompareFunctionNotEqual;
        case AM_DEPTH_FUNC_LESS: return MTLCompareFunctionLess;
        case AM_DEPTH_FUNC_LEQUAL: return MTLCompareFunctionLessEqual;
        case AM_DEPTH_FUNC_GREATER: return MTLCompareFunctionGreater;
        case AM_DEPTH_FUNC_GEQUAL: return MTLCompareFunctionGreaterEqual;
    }
    return MTLCompareFunctionAlways;
}

static MTLCullMode to_metal_cull_mode(bool enabled, am_cull_face_side side) {
    if (!enabled) return MTLCullModeNone;
    switch (side) {
        case AM_CULL_FACE_FRONT: return MTLCullModeFront;
        case AM_CULL_FACE_BACK: return MTLCullModeBack;
    }
    return MTLCullModeNone;
}

static MTLWinding to_metal_winding(am_face_winding w) {
    switch (w) {
        case AM_FACE_WIND_CW: return MTLWindingClockwise;
        case AM_FACE_WIND_CCW: return MTLWindingCounterClockwise;
    }
    return MTLWindingCounterClockwise;
}

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
    if (renderdesc == NULL) {
        am_log1("%s", "error: attempt to use incomplete framebuffer");
        return;
    }
    if (clear_color_buf) {
        renderdesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    }
    if (clear_depth_buf) {
        renderdesc.depthAttachment.loadAction = MTLLoadActionClear;
    }
    metal_encoder = [metal_command_buffer renderCommandEncoderWithDescriptor:renderdesc];
    metal_active_pipeline = -1;
    metal_active_framebuffer = metal_bound_framebuffer;
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
    if (@available(macOS 10.13, *)) {
        metal_layer.displaySyncEnabled = (am_conf_vsync ? YES : NO);
    }

    metal_queue = [metal_device newCommandQueue];

    get_active_metal_drawable(); // needed so metal_layer.drawableSize returns non-zero dimensions
    default_metal_framebuffer.complete = true;
    default_metal_framebuffer.width = metal_layer.drawableSize.width;
    default_metal_framebuffer.height = metal_layer.drawableSize.height;
    default_metal_framebuffer.clear_r = 0.0f;
    default_metal_framebuffer.clear_g = 0.0f;
    default_metal_framebuffer.clear_b = 0.0f;
    default_metal_framebuffer.clear_a = 1.0f;
    default_metal_framebuffer.color_texture = nil;
    if (am_metal_window_depth_buffer) {
        MTLTextureDescriptor *texdescr = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth16Unorm
            width:default_metal_framebuffer.width height:default_metal_framebuffer.height mipmapped:NO];
        texdescr.usage = MTLTextureUsageRenderTarget;
        texdescr.storageMode = MTLStorageModePrivate;
        default_metal_framebuffer.depth_texture = [metal_device newTextureWithDescriptor:texdescr];
    } else {
        default_metal_framebuffer.depth_texture = nil;
    }

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

    for (int i = 0; i < NUM_TEXTURE_UNITS; i++) {
        texture_units[i] = -1;
    }

    metal_active_texture = 0;
    metal_bound_framebuffer = 0;
    metal_active_element_buffer = 0;
    metal_active_array_buffer = 0;
    metal_active_pipeline = -1;
    metal_active_program = 0;
    metal_active_drawable = nil;

    metal_blend_enabled = false;
    metal_blend_eq_rgb = AM_BLEND_EQUATION_ADD;
    metal_blend_eq_alpha = AM_BLEND_EQUATION_ADD;
    metal_blend_sfactor_rgb = AM_BLEND_SFACTOR_SRC_ALPHA;
    metal_blend_sfactor_alpha = AM_BLEND_SFACTOR_SRC_ALPHA;
    metal_blend_dfactor_rgb = AM_BLEND_DFACTOR_SRC_ALPHA;
    metal_blend_dfactor_alpha = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;

    metal_depth_test_enabled = false;
    metal_depth_mask = false;
    metal_depth_func = AM_DEPTH_FUNC_ALWAYS;

    metal_face_culling_enabled = false;
    metal_face_cull_side = AM_CULL_FACE_BACK;
    metal_face_winding = AM_FACE_WIND_CCW;

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
    metal_bound_framebuffer = 0;
    if (default_metal_framebuffer.depth_texture != nil) {
        [default_metal_framebuffer.depth_texture release];
        default_metal_framebuffer.depth_texture = nil;
    }
    [metal_queue release];
    [metal_device release];

    glslopt_cleanup(glslopt_context);

    metal_initialized = false;
}

// Per-Fragment Operations

void am_set_blend_enabled(bool enabled) {
    check_initialized();
    metal_blend_enabled = enabled;
}

void am_set_blend_color(float r, float g, float b, float a) {
    check_initialized();
    // TODO
}

void am_set_blend_equation(am_blend_equation rgb, am_blend_equation alpha) {
    check_initialized();
    metal_blend_eq_rgb = rgb;
    metal_blend_eq_alpha = alpha;
}

void am_set_blend_func(am_blend_sfactor src_rgb, am_blend_dfactor dst_rgb, am_blend_sfactor src_alpha, am_blend_dfactor dst_alpha) {
    check_initialized();
    metal_blend_sfactor_rgb = src_rgb;
    metal_blend_dfactor_rgb = dst_rgb;
    metal_blend_sfactor_alpha = src_alpha;
    metal_blend_dfactor_alpha = dst_alpha;
}

void am_set_depth_test_enabled(bool enabled) {
    check_initialized();
    metal_depth_test_enabled = enabled;
}

void am_set_depth_func(am_depth_func func) {
    check_initialized();
    metal_depth_func = func;
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
    metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
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
    metal_depth_mask = flag;
}

void am_set_framebuffer_stencil_mask(am_stencil_face_side face, am_gluint mask) {
    check_initialized();
    // TODO
}

// Buffer Objects

am_buffer_id am_create_buffer_object() {
    check_initialized(0);
    metal_buffer buf;
    buf.size = 0;
    buf.mtlbuf = nil;
    return metal_buffer_freelist.add(buf);
}

void am_bind_buffer(am_buffer_target target, am_buffer_id buffer) {
    check_initialized();
    switch (target) {
        case AM_ARRAY_BUFFER: metal_active_array_buffer = buffer; break;
        case AM_ELEMENT_ARRAY_BUFFER: metal_active_element_buffer = buffer; break;
    }
}

static metal_buffer *get_active_buffer(am_buffer_target target) {
    switch (target) {
        case AM_ARRAY_BUFFER: 
            if (metal_active_array_buffer == 0) return NULL;
            else return metal_buffer_freelist.get(metal_active_array_buffer);
        case AM_ELEMENT_ARRAY_BUFFER:
            if (metal_active_element_buffer == 0) return NULL;
            else return metal_buffer_freelist.get(metal_active_element_buffer);
    }
    return NULL;
}

void am_set_buffer_data(am_buffer_target target, int size, void *data, am_buffer_usage usage) {
    check_initialized();
    metal_buffer *buf = get_active_buffer(target);
    if (buf == NULL) return;
    if (buf->size != size) {
        if (buf->mtlbuf != nil) {
            [buf->mtlbuf release];
            buf->mtlbuf = nil;
        }
        buf->mtlbuf = [metal_device newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
    } else {
        memcpy([buf->mtlbuf contents], data, size);
        [buf->mtlbuf didModifyRange: NSMakeRange(0, size)];
    }
}

void am_set_buffer_sub_data(am_buffer_target target, int offset, int size, void *data) {
    check_initialized();
    metal_buffer *buf = get_active_buffer(target);
    if (buf == NULL) return;
    if (buf->mtlbuf == nil) return;
    uint8_t *contents = (uint8_t*)[buf->mtlbuf contents];
    memcpy(&contents[offset], data, size);
    [buf->mtlbuf didModifyRange: NSMakeRange(offset, size)];
}

void am_delete_buffer(am_buffer_id id) {
    check_initialized();
    metal_buffer *buf = metal_buffer_freelist.get(id);
    if (buf->mtlbuf != nil) [buf->mtlbuf release];
    metal_buffer_freelist.remove(id);
    if (metal_active_array_buffer == id) metal_active_array_buffer = 0;
    if (metal_active_element_buffer == id) metal_active_element_buffer = 0;
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
    metal_face_winding = mode;
}

void am_set_cull_face_enabled(bool enabled) {
    check_initialized();
    metal_face_culling_enabled = enabled;
}

void am_set_cull_face_side(am_cull_face_side face) {
    check_initialized();
    metal_face_cull_side = face;
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
    prog.vert_uniform_size = 0;
    prog.vert_uniform_data = NULL;
    prog.frag_uniform_size = 0;
    prog.frag_uniform_data = NULL;
    prog.num_uniforms = 0;
    prog.uniforms = NULL;
    prog.num_tex_uniforms = 0;
    prog.tex_uniforms = NULL;
    prog.num_attrs = 0;
    prog.attrs = NULL;
    prog.log = NULL;
    return metal_program_freelist.add(prog);
}

am_shader_id am_create_shader(am_shader_type type) {
    check_initialized(0);
    metal_shader shader;
    metal_shader_init(&shader, type);
    return metal_shader_freelist.add(shader);
}

/*
static const char* glslopt_type_str(glslopt_basic_type t) {
    switch (t) {
	case kGlslTypeFloat: return "float";
	case kGlslTypeInt: return "int";
	case kGlslTypeBool: return "bool";
	case kGlslTypeTex2D: return "tex2d";
	case kGlslTypeTex3D: return "tex3d";
	case kGlslTypeTexCube: return "texcube";
	case kGlslTypeTex2DShadow: return "tex2dshadow";
	case kGlslTypeTex2DArray: return "tex2darray";
	case kGlslTypeOther: return "other";
	default:
            return "<error>";
    }
}

static const char* glslopt_prec_str(glslopt_precision p) {
    switch (p) {
	case kGlslPrecHigh: return "high";
	case kGlslPrecMedium: return "medium";
	case kGlslPrecLow: return "low";
	default:
            return "<error>";
    }
}
*/

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
        //printf("%s", "----------------------------------------------------------------------------------\n");
        //printf("GLSL:\n%s\n", src);
        printf("Metal:\n%s\n", metal_src);
        shader->lib = [metal_device newLibraryWithSource:[NSString stringWithUTF8String:metal_src] options: nil error:&error];
        if (shader->lib == nil) {
            *msg = am_format("%s", [[error localizedDescription] UTF8String]);
            get_src_error_line(*msg, src, line_no, line_str);
            success = false;
        } else {
            shader->func = [shader->lib newFunctionWithName:@"xlatMtlMain"];
            shader->ref_count = 1;
            //int n = glslopt_shader_get_input_count(shader->gshader);
            //printf("\n%d Inputs:\n", n);
            //for (int i = 0; i < n; i++) {
            //    const char *name;
            //    glslopt_basic_type type;
            //    glslopt_precision precision;
            //    int vecsize;
            //    int matsize;
            //    int arraysize;
            //    int location;
            //    glslopt_shader_get_input_desc (shader->gshader, i, &name, &type, &precision, &vecsize, &matsize, &arraysize, &location);
            //    printf("  %i %s %s %s v:%d m:%d a:%d l:%d\n", i, name, glslopt_type_str(type), glslopt_prec_str(precision), vecsize, matsize,
            //        arraysize, location);
            //}
            //n = glslopt_shader_get_uniform_count(shader->gshader);
            //printf("\n%d Uniforms (%d bytes):\n", n, glslopt_shader_get_uniform_total_size(shader->gshader));
            //for (int i = 0; i < n; i++) {
            //    const char *name;
            //    glslopt_basic_type type;
            //    glslopt_precision precision;
            //    int vecsize;
            //    int matsize;
            //    int arraysize;
            //    int location;
            //    glslopt_shader_get_uniform_desc (shader->gshader, i, &name, &type, &precision, &vecsize, &matsize, &arraysize, &location);
            //    printf("  %i %s %s %s v:%d m:%d a:%d l:%d\n", i, name, glslopt_type_str(type), glslopt_prec_str(precision), vecsize, matsize,
            //        arraysize, location);
            //}
            //n = glslopt_shader_get_texture_count(shader->gshader);
            //printf("\n%d Textures:\n", n);
            //for (int i = 0; i < n; i++) {
            //    const char *name;
            //    glslopt_basic_type type;
            //    glslopt_precision precision;
            //    int vecsize;
            //    int matsize;
            //    int arraysize;
            //    int location;
            //    glslopt_shader_get_texture_desc (shader->gshader, i, &name, &type, &precision, &vecsize, &matsize, &arraysize, &location);
            //    printf("  %i %s %s %s v:%d m:%d a:%d l:%d\n", i, name, glslopt_type_str(type), glslopt_prec_str(precision), vecsize, matsize,
            //        arraysize, location);
            //}
            //printf("%s", "==================================================================================\n");
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

static bool set_uniform_type(metal_program *prog, uniform_descr* uni, glslopt_basic_type type, 
    glslopt_precision prec, int msize, int vsize, int asize, const char *name)
{
    switch (type) {
        case kGlslTypeFloat:
            if (prec != kGlslPrecHigh) {
                prog->log = am_format("uniform %s is not high precision (sorry, only high precision uniforms supported)", name);
                return false;
            }
            if (asize > 1) {
                prog->log = am_format("uniform %s is an array (sorry, uniforms arrays not supported)", name);
                return false;
            }
            switch (msize) {
                case 1:
                    switch (vsize) {
                        case 1:
                            uni->type = AM_UNIFORM_VAR_TYPE_FLOAT;
                            break;
                        case 2:
                            uni->type = AM_UNIFORM_VAR_TYPE_FLOAT_VEC2;
                            break;
                        case 3:
                            uni->type = AM_UNIFORM_VAR_TYPE_FLOAT_VEC3;
                            break;
                        case 4:
                            uni->type = AM_UNIFORM_VAR_TYPE_FLOAT_VEC4;
                            break;
                        default:
                            prog->log = am_format("uniform %s has unsupported type", name);
                            return false;
                    }
                    break;
                case 2:
                     uni->type = AM_UNIFORM_VAR_TYPE_FLOAT_MAT2;
                     break;
                case 3:
                     uni->type = AM_UNIFORM_VAR_TYPE_FLOAT_MAT3;
                     break;
                case 4:
                     uni->type = AM_UNIFORM_VAR_TYPE_FLOAT_MAT4;
                     break;
                default:
                     prog->log = am_format("uniform %s has unsupported type", name);
                     return false;
            }
            break;
        case kGlslTypeTex2D:
            uni->type = AM_UNIFORM_VAR_TYPE_SAMPLER_2D;
            break;
        case kGlslTypeTexCube:
            uni->type = AM_UNIFORM_VAR_TYPE_SAMPLER_CUBE;
            break;
        default:
            prog->log = am_format("uniform %s has unsupported type", name);
            return false;
    }
    return true;
}

bool am_link_program(am_program_id program_id) {
    check_initialized(false);
    metal_program *prog = metal_program_freelist.get(program_id);
    metal_shader *vert = &prog->vert_shader;
    metal_shader *frag = &prog->frag_shader;

    // setup uniforms
    int vert_bytes = glslopt_shader_get_uniform_total_size(vert->gshader);
    int frag_bytes = glslopt_shader_get_uniform_total_size(frag->gshader);
    if (vert_bytes > 0) {
        prog->vert_uniform_size = vert_bytes;
        prog->vert_uniform_data = (uint8_t*)malloc(vert_bytes);
        memset(prog->vert_uniform_data, 0, vert_bytes);
    }
    if (frag_bytes > 0) {
        prog->frag_uniform_size = frag_bytes;
        prog->frag_uniform_data = (uint8_t*)malloc(frag_bytes);
        memset(prog->frag_uniform_data, 0, frag_bytes);
    }
    int num_vert_uniforms = glslopt_shader_get_uniform_count(vert->gshader);
    int num_vert_textures = glslopt_shader_get_texture_count(vert->gshader);
    int num_frag_uniforms = glslopt_shader_get_uniform_count(frag->gshader);
    int num_frag_textures = glslopt_shader_get_texture_count(frag->gshader);
    int max_uniform_count = num_vert_uniforms + num_vert_textures + num_frag_uniforms + num_frag_textures;
    prog->uniforms = (uniform_descr*)malloc(max_uniform_count * sizeof(uniform_descr));
    for (int i = 0; i < max_uniform_count; i++) {
        uniform_descr *uni = &prog->uniforms[i];
        uni->type = AM_UNIFORM_VAR_TYPE_UNKNOWN;
        uni->vert_location = -1;
        uni->frag_location = -1;
        uni->tex_unit = -1;
        uni->name = NULL;
    }
    for (int i = 0; i < num_vert_uniforms + num_vert_textures; i++) {
        uniform_descr *uni = &prog->uniforms[i];
        const char *name;
        glslopt_basic_type type;
        glslopt_precision prec;
        int vsize;
        int msize;
        int asize;
        int loc;
        if (i < num_vert_uniforms) {
            glslopt_shader_get_uniform_desc(vert->gshader, i, &name, &type, &prec, &vsize, &msize, &asize, &loc);
        } else {
            glslopt_shader_get_texture_desc(vert->gshader, i - num_vert_uniforms, &name, &type, &prec, &vsize, &msize, &asize, &loc);
        }
        if (!set_uniform_type(prog, uni, type, prec, msize, vsize, asize, name)) {
            return false;
        }
        uni->vert_location = loc;
        uni->name = name;
    }
    int num_uniforms = num_vert_uniforms + num_vert_textures;
    for (int i = 0; i < num_frag_uniforms + num_frag_textures; i++) {
        uniform_descr *uni = &prog->uniforms[num_uniforms];
        const char *name;
        glslopt_basic_type type;
        glslopt_precision prec;
        int vsize;
        int msize;
        int asize;
        int loc;
        if (i < num_frag_uniforms) {
            glslopt_shader_get_uniform_desc(frag->gshader, i, &name, &type, &prec, &vsize, &msize, &asize, &loc);
        } else {
            glslopt_shader_get_texture_desc(frag->gshader, i - num_frag_uniforms, &name, &type, &prec, &vsize, &msize, &asize, &loc);
        }
        if (!set_uniform_type(prog, uni, type, prec, msize, vsize, asize, name)) {
            return false;
        }
        // see if this is already a vertex shader uniform
        bool found = false;
        for (int j = 0; j < num_vert_uniforms; j++) {
            if (strcmp(prog->uniforms[j].name, name) == 0) {
                if (uni->type != prog->uniforms[j].type) {
                    prog->log = am_format("uniform %s has different types in vertex and fragment shaders", name);
                    return false;
                }
                prog->uniforms[j].frag_location = loc;
                found = true;
                break;
            }
        }
        if (!found) {
            uni->frag_location = loc;
            uni->name = name;
            num_uniforms++;
        }
    }
    prog->num_uniforms = num_uniforms;

    // setup attributes
    prog->num_attrs = glslopt_shader_get_input_count(prog->vert_shader.gshader);
    prog->attrs = (attr_state*)malloc(prog->num_attrs * sizeof(attr_state));
    for (int i = 0; i < prog->num_attrs; i++) {
        attr_state *attr = &prog->attrs[i];
        attr->bufid = 0;
        attr->offset = 0;
        const char *name;
        glslopt_basic_type type;
        glslopt_precision prec;
        int vsize;
        int msize;
        int asize;
        int loc;
        glslopt_shader_get_input_desc(prog->vert_shader.gshader, i, &name, &type, &prec, &vsize, &msize, &asize, &loc);
        if (asize > 1) {
            prog->log = am_format("attribute %s has unsupported type", name);
            return false;
        }
        switch (type) {
            case kGlslTypeFloat: {
                switch (msize) {
                    case 1:
                        switch (vsize) {
                            case 1:
                                attr->descr.type = AM_ATTRIBUTE_VAR_TYPE_FLOAT;
                                break;
                            case 2:
                                attr->descr.type = AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC2;
                                break;
                            case 3:
                                attr->descr.type = AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC3;
                                break;
                            case 4:
                                attr->descr.type = AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC4;
                                break;
                            default:
                                prog->log = am_format("attribute %s has unsupported type", name);
                                return false;
                        }
                        break;
                    case 2:
                        attr->descr.type = AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT2;
                        break;
                    case 3:
                        attr->descr.type = AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT3;
                        break;
                    case 4:
                        attr->descr.type = AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT4;
                        break;
                    default:
                        prog->log = am_format("attribute %s has unsupported type", name);
                        return false;
                }
                break;
            }
            default:
                prog->log = am_format("attribute %s has unsupported type", name);
                return false;
        }
        attr->descr.location = loc;
        attr->descr.name = name;
    }

    return true;
}

char *am_get_program_info_log(am_program_id program_id) {
    check_initialized(NULL);
    metal_program *prog = metal_program_freelist.get(program_id);
    return prog->log;
}

int am_get_program_active_attributes(am_program_id program_id) {
    check_initialized(0);
    metal_program *prog = metal_program_freelist.get(program_id);
    return prog->num_attrs;
}

int am_get_program_active_uniforms(am_program_id program_id) {
    check_initialized(0);
    metal_program *prog = metal_program_freelist.get(program_id);
    return prog->num_uniforms;
}

bool am_validate_program(am_program_id program) {
    check_initialized(false);
    // nop
    return true;
}

void am_use_program(am_program_id program_id) {
    check_initialized();
    if (program_id == metal_active_program) return;
    metal_active_program = program_id;
    metal_active_pipeline = -1;
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
    if (prog->vert_uniform_data != NULL) free(prog->vert_uniform_data);
    if (prog->frag_uniform_data != NULL) free(prog->frag_uniform_data);
    if (prog->uniforms != NULL) free(prog->uniforms);
    if (prog->attrs != NULL) free(prog->attrs);
    for (int i = 0; i < prog->pipeline_cache.size(); i++) {
        [prog->pipeline_cache[i].mtlpipeline release];
        if (prog->pipeline_cache[i].mtldepthstencilstate != nil) {
            [prog->pipeline_cache[i].mtldepthstencilstate release];
        }
    }
    prog->pipeline_cache.clear();
    if (prog->log != NULL) free(prog->log);
    metal_program_freelist.remove(id);
    if (metal_active_program == id) {
        metal_active_program = 0;
    }
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
    // nop
}

void am_get_active_attribute(am_program_id program_id, am_gluint index,
    char **name, am_attribute_var_type *type, int *size, am_gluint *loc)
{
    check_initialized();
    metal_program *prog = metal_program_freelist.get(program_id);
    attr_state *attr = &prog->attrs[index];
    *name = am_format("%s", attr->descr.name);
    *type = attr->descr.type;
    *size = 1;
    *loc = index;
}

void am_get_active_uniform(am_program_id program_id, am_gluint index,
    char **name, am_uniform_var_type *type, int *size, am_gluint *loc)
{
    check_initialized();
    metal_program *prog = metal_program_freelist.get(program_id);
    uniform_descr *uni = &prog->uniforms[index];
    *name = am_format("%s", uni->name);
    *type = uni->type;
    *size = 1;
    *loc = index;
}

static void set_float_uniform(am_gluint location, const float *value, int n) {
    if (metal_active_program == 0) return;
    metal_program *prog = metal_program_freelist.get(metal_active_program);
    uniform_descr *uni = &prog->uniforms[location];
    if (uni->vert_location >= 0) memcpy(&prog->vert_uniform_data[uni->vert_location], value, n * 4);
    if (uni->frag_location >= 0) memcpy(&prog->frag_uniform_data[uni->frag_location], value, n * 4);
}

void am_set_uniform1f(am_gluint location, float value) {
    check_initialized();
    set_float_uniform(location, &value, 1);
}

void am_set_uniform2f(am_gluint location, const float *value) {
    check_initialized();
    set_float_uniform(location, value, 2);
}

void am_set_uniform3f(am_gluint location, const float *value) {
    check_initialized();
    set_float_uniform(location, value, 3);
}

void am_set_uniform4f(am_gluint location, const float *value) {
    check_initialized();
    set_float_uniform(location, value, 4);
}

static void set_int_uniform(am_gluint location, const am_glint *value, int n) {
    if (metal_active_program == 0) return;
    metal_program *prog = metal_program_freelist.get(metal_active_program);
    uniform_descr *uni = &prog->uniforms[location];
    if (n == 1 && (uni->type == AM_UNIFORM_VAR_TYPE_SAMPLER_2D || uni->type == AM_UNIFORM_VAR_TYPE_SAMPLER_CUBE)) {
        // value is texture unit
        uni->tex_unit = *value;
    } else {
        // TODO
    }
}

void am_set_uniform1i(am_gluint location, am_glint value) {
    check_initialized();
    set_int_uniform(location, &value, 1);
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
    set_float_uniform(location, value, 4);
}

void am_set_uniform_mat3(am_gluint location, const float *value) {
    check_initialized();
    set_float_uniform(location, value, 9);
}

void am_set_uniform_mat4(am_gluint location, const float *value) {
    check_initialized();
    set_float_uniform(location, value, 16);
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

void am_set_attribute_pointer(am_gluint location, int dims, am_attribute_client_type type, bool normalized, int stride, int offset) {
    check_initialized();
    if (metal_active_program == 0) return;
    if (metal_active_array_buffer == 0) return;
    metal_program *prog = metal_program_freelist.get(metal_active_program);
    attr_state *attr = &prog->attrs[location];
    attr->bufid = metal_active_array_buffer;
    attr->offset = offset;
    attr->layout.type = type;
    attr->layout.dims = dims;
    attr->layout.normalized = normalized;
    attr->layout.stride = stride;
}

// Texture Objects

void am_set_active_texture_unit(int texture_unit) {
    check_initialized();
    if (texture_unit < NUM_TEXTURE_UNITS) {
        active_texture_unit = texture_unit;
    }
}

am_texture_id am_create_texture() {
    check_initialized(0);
    metal_texture tex;
    tex.tex = nil;
    tex.sampler = nil;
    return metal_texture_freelist.add(tex);
}

void am_delete_texture(am_texture_id id) {
    check_initialized();
    metal_texture *tex = metal_texture_freelist.get(id);
    if (tex->tex != nil) {
        [tex->tex release];
        [tex->sampler release];
        tex->tex = nil;
        tex->sampler = nil;
    }
    metal_texture_freelist.remove(id);
    if (metal_active_texture == id) metal_active_texture = 0;
}

void am_bind_texture(am_texture_bind_target target, am_texture_id texture) {
    check_initialized();
    if (active_texture_unit >= 0) {
        texture_units[active_texture_unit] = texture;
    }
    metal_active_texture = texture;
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
    if (metal_active_texture == 0) return;
    metal_texture *tex = metal_texture_freelist.get(metal_active_texture);
    if (tex->tex == nil) {
        MTLTextureDescriptor *texdescr = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];
        tex->tex = [metal_device newTextureWithDescriptor:texdescr];
        MTLSamplerDescriptor *samplerdescr = [[MTLSamplerDescriptor alloc] init];
        tex->sampler = [metal_device newSamplerStateWithDescriptor:samplerdescr];
        [samplerdescr release];
        [tex->tex replaceRegion:MTLRegionMake2D(0, 0, w, h) mipmapLevel:level withBytes:data bytesPerRow:w*4];
    }
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
    metal_renderbuffer renderbuffer;
    renderbuffer.initialized = false;
    renderbuffer.format = AM_RENDERBUFFER_FORMAT_RGBA4;
    renderbuffer.width = 0;
    renderbuffer.height = 0;
    renderbuffer.texture = nil;
    return metal_renderbuffer_freelist.add(renderbuffer);
}

void am_delete_renderbuffer(am_renderbuffer_id id) {
    check_initialized();
    if (metal_bound_renderbuffer == id) metal_bound_renderbuffer = 0;
    metal_renderbuffer *renderbuffer = metal_renderbuffer_freelist.get(id);
    if (renderbuffer->texture != nil) {
        [renderbuffer->texture release];
        renderbuffer->texture = nil;
    }
    metal_renderbuffer_freelist.remove(id);
}

void am_bind_renderbuffer(am_renderbuffer_id id) {
    check_initialized();
    metal_bound_renderbuffer = id;
}

void am_set_renderbuffer_storage(am_renderbuffer_format format, int w, int h) {
    check_initialized();
    if (metal_bound_renderbuffer == 0) {
        am_log1("%s", "error: attempt to set renderbuffer storage without first binding a renderbuffer");
        return;
    }
    metal_renderbuffer *renderbuffer = metal_renderbuffer_freelist.get(metal_bound_renderbuffer);
    if (renderbuffer->initialized) {
        am_log1("%s", "error: attempt to set renderbuffer storage twice");
        return;
    }
    renderbuffer->width = w;
    renderbuffer->height = h;
    renderbuffer->format = format;
    MTLPixelFormat pxlfmt;
    switch (format) {
        case AM_RENDERBUFFER_FORMAT_RGBA4: pxlfmt = MTLPixelFormatInvalid; break;
        case AM_RENDERBUFFER_FORMAT_DEPTH_COMPONENT16: pxlfmt = MTLPixelFormatDepth16Unorm; break;
        case AM_RENDERBUFFER_FORMAT_DEPTH_COMPONENT24: pxlfmt = MTLPixelFormatInvalid; break;
        case AM_RENDERBUFFER_FORMAT_STENCIL_INDEX8: pxlfmt = MTLPixelFormatStencil8; break;
    }
    MTLTextureDescriptor *texdescr = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pxlfmt 
            width:w height:h mipmapped:NO];
    texdescr.usage = MTLTextureUsageRenderTarget;
    texdescr.storageMode = MTLStorageModePrivate;
    renderbuffer->texture = [metal_device newTextureWithDescriptor:texdescr];
    renderbuffer->initialized = true;
}

// Read Back Pixels 

void am_read_pixels(int x, int y, int w, int h, void *data) {
    check_initialized();
    // TODO
}

// Framebuffer Objects

am_framebuffer_id am_create_framebuffer() {
    check_initialized(0);
    metal_framebuffer fb;
    fb.complete = false;
    fb.width = 0;
    fb.height = 0;
    fb.clear_r = 0.0f;
    fb.clear_g = 0.0f;
    fb.clear_b = 0.0f;
    fb.clear_a = 1.0f;
    fb.color_texture = nil;
    fb.depth_texture = nil;
    return metal_framebuffer_freelist.add(fb);
}

void am_delete_framebuffer(am_framebuffer_id id) {
    check_initialized();
    if (metal_bound_framebuffer == id) metal_bound_framebuffer = 0;
    // textures are not owned by the framebuffer and will be released elsewhere
    metal_framebuffer_freelist.remove(id);
}

void am_bind_framebuffer(am_framebuffer_id fb) {
    check_initialized();
    metal_bound_framebuffer = fb;
}

am_framebuffer_status am_check_framebuffer_status() {
    check_initialized(AM_FRAMEBUFFER_STATUS_UNKNOWN);
    if (metal_bound_framebuffer == 0) {
        // default framebuffer
        return AM_FRAMEBUFFER_STATUS_COMPLETE;
    }
    metal_framebuffer *fb = metal_framebuffer_freelist.get(metal_bound_framebuffer);
    if (fb->complete) {
        return AM_FRAMEBUFFER_STATUS_COMPLETE;
    } else {
        return AM_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT;
    }
}

void am_set_framebuffer_renderbuffer(am_framebuffer_attachment attachment, am_renderbuffer_id rb) {
    check_initialized();
    metal_renderbuffer *renderbuffer = metal_renderbuffer_freelist.get(rb);
    metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
    switch (attachment) {
        case AM_FRAMEBUFFER_COLOR_ATTACHMENT0:
            // unsupported
            break;
        case AM_FRAMEBUFFER_DEPTH_ATTACHMENT:
            fb->depth_texture = renderbuffer->texture;
            break;
        case AM_FRAMEBUFFER_STENCIL_ATTACHMENT:
            // TODO
            //fb->stencil_texture = renderbuffer->texture;
            break;
    }
    if (fb->width > 0) {
        if (fb->width != renderbuffer->width || fb->height != renderbuffer->height) {
            am_log1("error: setting framebuffer renderbuffer with incompatible dimensions (%dx%d vs %dx%d)",
                renderbuffer->width, renderbuffer->height, fb->width, fb->height);
        }
    }
}

void am_set_framebuffer_texture2d(am_framebuffer_attachment attachment, am_texture_copy_target target, am_texture_id texid) {
    check_initialized();
    metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
    metal_texture *tex = metal_texture_freelist.get(texid);
    switch (attachment) {
        case AM_FRAMEBUFFER_COLOR_ATTACHMENT0:
            if (fb->color_texture != nil) {
                am_log1("%s", "error: attempt to set framebuffer color attachement twice");
                return;
            }
            fb->color_texture = tex->tex;
            if (fb->width > 0) {
                if (fb->width != tex->tex.width || fb->height != tex->tex.height) {
                    am_log1("error: setting framebuffer texture with incompatible dimensions (%dx%d vs %dx%d)",
                        tex->tex.width, tex->tex.height, fb->width, fb->height);
                }
            }
            fb->width = tex->tex.width;
            fb->height = tex->tex.height;
            fb->complete = true;
            break;
        case AM_FRAMEBUFFER_DEPTH_ATTACHMENT:
            // unsupported
            break;
        case AM_FRAMEBUFFER_STENCIL_ATTACHMENT:
            // unsupported
            break;
    }
}

// Writing to the Draw Buffer

static MTLVertexFormat get_attr_format(attr_layout *attr) {
    switch (attr->type) {
        // commented out formats only available in OSX 10.13+
        case AM_ATTRIBUTE_CLIENT_TYPE_BYTE:
            switch (attr->dims) {
                //case 1:
                //    if (attr->normalized) return MTLVertexFormatInvalid;
                //    else                  return MTLVertexFormatChar;
                case 2:
                    if (attr->normalized) return MTLVertexFormatChar2Normalized;
                    else                  return MTLVertexFormatChar2;
                case 3:
                    if (attr->normalized) return MTLVertexFormatChar3Normalized;
                    else                  return MTLVertexFormatChar3;
                case 4:
                    if (attr->normalized) return MTLVertexFormatChar4Normalized;
                    else                  return MTLVertexFormatChar4;
                default:
                    return MTLVertexFormatInvalid;
            }
        case AM_ATTRIBUTE_CLIENT_TYPE_SHORT:
            switch (attr->dims) {
                //case 1:
                //    if (attr->normalized) return MTLVertexFormatShortNormalized;
                //    else                  return MTLVertexFormatShort;
                case 2:
                    if (attr->normalized) return MTLVertexFormatShort2Normalized;
                    else                  return MTLVertexFormatShort2;
                case 3:
                    if (attr->normalized) return MTLVertexFormatShort3Normalized;
                    else                  return MTLVertexFormatShort3;
                case 4:
                    if (attr->normalized) return MTLVertexFormatShort4Normalized;
                    else                  return MTLVertexFormatShort4;
                default:
                    return MTLVertexFormatInvalid;
            }
        case AM_ATTRIBUTE_CLIENT_TYPE_UBYTE:
            switch (attr->dims) {
                //case 1:
                //    if (attr->normalized) return MTLVertexFormatUCharNormalized;
                //    else                  return MTLVertexFormatUChar;
                case 2:
                    if (attr->normalized) return MTLVertexFormatUChar2Normalized;
                    else                  return MTLVertexFormatUChar2;
                case 3:
                    if (attr->normalized) return MTLVertexFormatUChar3Normalized;
                    else                  return MTLVertexFormatUChar3;
                case 4:
                    if (attr->normalized) return MTLVertexFormatUChar4Normalized;
                    else                  return MTLVertexFormatUChar4;
                default:
                    return MTLVertexFormatInvalid;
            }
        case AM_ATTRIBUTE_CLIENT_TYPE_USHORT:
            switch (attr->dims) {
                //case 1:
                //    if (attr->normalized) return MTLVertexFormatUShortNormalized;
                //    else                  return MTLVertexFormatUShort;
                case 2:
                    if (attr->normalized) return MTLVertexFormatUShort2Normalized;
                    else                  return MTLVertexFormatUShort2;
                case 3:
                    if (attr->normalized) return MTLVertexFormatUShort3Normalized;
                    else                  return MTLVertexFormatUShort3;
                case 4:
                    if (attr->normalized) return MTLVertexFormatUShort4Normalized;
                    else                  return MTLVertexFormatUShort4;
                default:
                    return MTLVertexFormatInvalid;
            }
        case AM_ATTRIBUTE_CLIENT_TYPE_FLOAT:
            switch (attr->dims) {
                case 1:
                    return MTLVertexFormatFloat;
                case 2:
                    return MTLVertexFormatFloat2;
                case 3:
                    return MTLVertexFormatFloat3;
                case 4:
                    return MTLVertexFormatFloat4;
                default:
                    return MTLVertexFormatInvalid;
            }
        default:
            return MTLVertexFormatInvalid;
    }
}

static bool setup_pipeline(metal_program *prog) {
    int nattrs = prog->num_attrs;
    int selected_pipeline = -1;
    for (int i = 0; i < prog->pipeline_cache.size(); i++) {
        metal_pipeline *cached_pipeline = &prog->pipeline_cache[i];
        if (
            cached_pipeline->blend_enabled != metal_blend_enabled ||
            cached_pipeline->blend_eq_rgb != metal_blend_eq_rgb ||
            cached_pipeline->blend_eq_alpha != metal_blend_eq_alpha ||
            cached_pipeline->blend_sfactor_rgb != metal_blend_sfactor_rgb ||
            cached_pipeline->blend_sfactor_alpha != metal_blend_sfactor_alpha ||
            cached_pipeline->blend_dfactor_rgb != metal_blend_dfactor_rgb ||
            cached_pipeline->blend_dfactor_alpha != metal_blend_dfactor_alpha ||
            cached_pipeline->depth_test_enabled != metal_depth_test_enabled ||
            cached_pipeline->depth_mask != metal_depth_mask ||
            cached_pipeline->depth_func != metal_depth_func ||
            cached_pipeline->face_culling_enabled != metal_face_culling_enabled ||
            cached_pipeline->face_cull_side != metal_face_cull_side ||
            cached_pipeline->face_winding != metal_face_winding ||
            false)
        {
            continue;
        }
        attr_layout *cached_layouts = cached_pipeline->attr_layouts;
        bool match = true;
        for (int j = 0; j < nattrs; j++) {
            attr_layout *cached_layout = &cached_layouts[j];
            attr_layout *active_layout = &prog->attrs[j].layout;
            if (cached_layout->type != active_layout->type ||
                cached_layout->dims != active_layout->dims ||
                cached_layout->stride != active_layout->stride ||
                cached_layout->normalized != active_layout->normalized)
            {
                match = false;
                break;
            }
        }
        if (match) {
            selected_pipeline = i;
            break;
        }
    }
    if (selected_pipeline == -1) {
        // no appropriate pipeline found, so create a new one.
        MTLRenderPipelineDescriptor *pldescr = [[MTLRenderPipelineDescriptor alloc] init];
        pldescr.vertexFunction = prog->vert_shader.func;
        pldescr.fragmentFunction = prog->frag_shader.func;

        if (metal_bound_framebuffer == 0) {
            pldescr.colorAttachments[0].pixelFormat = metal_layer.pixelFormat;
        } else {
            pldescr.colorAttachments[0].pixelFormat = get_metal_framebuffer(metal_bound_framebuffer)->color_texture.pixelFormat;
        }
        pldescr.colorAttachments[0].blendingEnabled = to_objc_bool(metal_blend_enabled);
        pldescr.colorAttachments[0].rgbBlendOperation = to_metal_blend_op(metal_blend_eq_rgb);
        pldescr.colorAttachments[0].alphaBlendOperation = to_metal_blend_op(metal_blend_eq_alpha);
        pldescr.colorAttachments[0].sourceRGBBlendFactor = to_metal_blend_sfactor(metal_blend_sfactor_rgb);
        pldescr.colorAttachments[0].sourceAlphaBlendFactor = to_metal_blend_sfactor(metal_blend_sfactor_alpha);
        pldescr.colorAttachments[0].destinationRGBBlendFactor = to_metal_blend_dfactor(metal_blend_dfactor_rgb);
        pldescr.colorAttachments[0].destinationAlphaBlendFactor = to_metal_blend_dfactor(metal_blend_dfactor_alpha);

        MTLVertexDescriptor *vertdescr = [[MTLVertexDescriptor alloc] init];
        for (int i = 0; i < nattrs; i++) {
            attr_state *attr = &prog->attrs[i];
            MTLVertexAttributeDescriptor *attrdescr = [[MTLVertexAttributeDescriptor alloc] init];
            attrdescr.format = get_attr_format(&attr->layout);
            attrdescr.offset = 0;
            attrdescr.bufferIndex = i + 1; // buffer 0 is always the uniforms
            [[vertdescr attributes] setObject:attrdescr atIndexedSubscript:attr->descr.location];
            [attrdescr release];
            MTLVertexBufferLayoutDescriptor *layoutdescr = [[MTLVertexBufferLayoutDescriptor alloc] init];
            layoutdescr.stepFunction = MTLVertexStepFunctionPerVertex;
            layoutdescr.stepRate = 1;
            layoutdescr.stride = attr->layout.stride;
            [[vertdescr layouts] setObject:layoutdescr atIndexedSubscript:i + 1];
            [layoutdescr release];
        }
        pldescr.vertexDescriptor = vertdescr;
        [vertdescr release];
        NSError *error = nil;
        id<MTLRenderPipelineState> pl = [metal_device newRenderPipelineStateWithDescriptor:pldescr error:&error];
        [pldescr release];
        if (pl == nil) {
            am_log1("Metal pipeline creation error: %s\n", [[error localizedDescription] UTF8String]);
            return false;
        }
        metal_pipeline cached_pipeline;
        cached_pipeline.attr_layouts = (attr_layout*)malloc(nattrs * sizeof(attr_layout));
        for (int i = 0; i < nattrs; i++) {
            cached_pipeline.attr_layouts[i] = prog->attrs[i].layout;
        }
        cached_pipeline.blend_enabled = metal_blend_enabled;
        cached_pipeline.blend_eq_rgb = metal_blend_eq_rgb;
        cached_pipeline.blend_eq_alpha = metal_blend_eq_alpha;
        cached_pipeline.blend_sfactor_rgb = metal_blend_sfactor_rgb;
        cached_pipeline.blend_sfactor_alpha = metal_blend_sfactor_alpha;
        cached_pipeline.blend_dfactor_rgb = metal_blend_dfactor_rgb;
        cached_pipeline.blend_dfactor_alpha = metal_blend_dfactor_alpha;
        cached_pipeline.mtlpipeline = pl;

        cached_pipeline.depth_test_enabled = metal_depth_test_enabled;;
        cached_pipeline.depth_mask = metal_depth_mask;;
        cached_pipeline.depth_func = metal_depth_func;
        if (metal_depth_test_enabled) {
            MTLDepthStencilDescriptor *descr = [[MTLDepthStencilDescriptor alloc] init];
            descr.depthCompareFunction = to_metal_depth_func(metal_depth_func);
            descr.depthWriteEnabled = to_objc_bool(metal_depth_mask);
            cached_pipeline.mtldepthstencilstate = [metal_device newDepthStencilStateWithDescriptor: descr];
            [descr release];
        } else {
            cached_pipeline.mtldepthstencilstate = nil;
        }

        cached_pipeline.face_culling_enabled = metal_face_culling_enabled;
        cached_pipeline.face_cull_side = metal_face_cull_side;
        cached_pipeline.face_winding = metal_face_winding;

        selected_pipeline = prog->pipeline_cache.size();
        prog->pipeline_cache.push_back(cached_pipeline);
    }
    if (selected_pipeline != metal_active_pipeline) {
        metal_pipeline *pipeline = &prog->pipeline_cache[selected_pipeline];
        [metal_encoder setRenderPipelineState: pipeline->mtlpipeline];
        if (pipeline->mtldepthstencilstate != nil) {
            [metal_encoder setDepthStencilState:pipeline->mtldepthstencilstate];
        } else {
            [metal_encoder setDepthStencilState:nil];
        }
        [metal_encoder setFrontFacingWinding: to_metal_winding(pipeline->face_winding)];
        [metal_encoder setCullMode: to_metal_cull_mode(pipeline->face_culling_enabled, pipeline->face_cull_side)];
        metal_active_pipeline = selected_pipeline;
    }
    return true;
}

static bool pre_draw_setup() {
    if (metal_bound_framebuffer != metal_active_framebuffer) {
        if (metal_encoder != nil) {
            [metal_encoder endEncoding];
            // XXX auto-releases?
            metal_encoder = nil;
        }
        create_new_metal_encoder(false, false, false);
    }
    if (metal_active_program == 0) return false;
    metal_program *prog = metal_program_freelist.get(metal_active_program);

    // pipeline
    if (!setup_pipeline(prog)) return false;

    // uniforms
    if (prog->vert_uniform_data != NULL) {
        [metal_encoder setVertexBytes:prog->vert_uniform_data length:prog->vert_uniform_size atIndex:0];
    }
    if (prog->frag_uniform_data != NULL) {
        [metal_encoder setFragmentBytes:prog->frag_uniform_data length:prog->frag_uniform_size atIndex:0];
    }
    
    // textures
    for (int i = 0; i < prog->num_uniforms; i++) {
        uniform_descr *uni = &prog->uniforms[i];
        if (uni->tex_unit >= 0) {
            am_texture_id texid = texture_units[uni->tex_unit];
            metal_texture *tex = metal_texture_freelist.get(texid);
            if (uni->vert_location >= 0) {
                [metal_encoder setVertexTexture:tex->tex atIndex:uni->vert_location];
                [metal_encoder setVertexSamplerState:tex->sampler atIndex:uni->vert_location];
            }
            if (uni->frag_location >= 0) {
                [metal_encoder setFragmentTexture:tex->tex atIndex:uni->frag_location];
                [metal_encoder setFragmentSamplerState:tex->sampler atIndex:uni->frag_location];
            }
        }
    }

    // attributes
    for (int i = 0; i < prog->num_attrs; i++) {
        attr_state *attr = &prog->attrs[i];
        if (attr->bufid == 0) continue;
        metal_buffer *buf = metal_buffer_freelist.get(attr->bufid);
        [metal_encoder setVertexBuffer:buf->mtlbuf offset:attr->offset atIndex:i + 1];
    }
    return true;
}

static MTLPrimitiveType to_metal_prim(am_draw_mode mode) {
    switch (mode) {
        case AM_DRAWMODE_POINTS:
            return MTLPrimitiveTypePoint;
        case AM_DRAWMODE_LINES:
            return MTLPrimitiveTypeLine;
        case AM_DRAWMODE_LINE_STRIP:
            return MTLPrimitiveTypeLineStrip;
        case AM_DRAWMODE_LINE_LOOP:
            am_log1("%s", "WARNING: line_loop primitives are not supported on Metal, using line_strip instead");
            return MTLPrimitiveTypeLineStrip;
        case AM_DRAWMODE_TRIANGLES:
            return MTLPrimitiveTypeTriangle;
        case AM_DRAWMODE_TRIANGLE_STRIP:
            return MTLPrimitiveTypeTriangleStrip;
        case AM_DRAWMODE_TRIANGLE_FAN:
            am_log1("%s", "WARNING: triangle_fan primitives are not supported on Metal, using triangle_strip instead");
            return MTLPrimitiveTypeTriangleStrip;
    }
}

void am_draw_arrays(am_draw_mode mode, int first, int count) {
    check_initialized();
    if (!pre_draw_setup()) return;
    [metal_encoder drawPrimitives:to_metal_prim(mode) vertexStart: first vertexCount: count];
}

void am_draw_elements(am_draw_mode mode, int count, am_element_index_type type, int offset) {
    check_initialized();
    if (metal_active_element_buffer == 0) return;
    metal_buffer *elembuf = metal_buffer_freelist.get(metal_active_element_buffer);
    if (!pre_draw_setup()) return;
    MTLIndexType itype = MTLIndexTypeUInt32;
    switch (type) {
        case AM_ELEMENT_TYPE_USHORT: itype = MTLIndexTypeUInt16; break;
        case AM_ELEMENT_TYPE_UINT: itype = MTLIndexTypeUInt32; break;
    }
    [metal_encoder drawIndexedPrimitives:to_metal_prim(mode)
                   indexCount:count
                    indexType:itype
                  indexBuffer:elembuf->mtlbuf
            indexBufferOffset:offset];
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
        if (metal_bound_framebuffer == 0) {
            [metal_command_buffer presentDrawable:get_active_metal_drawable()];
        }
        [metal_command_buffer commit];
        //[metal_command_buffer waitUntilCompleted];

        // XXX auto-releases?
        metal_command_buffer = nil;
        if (metal_bound_framebuffer == 0) {
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
    int res = sscanf(errmsg, "(%d:", line_no);
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
