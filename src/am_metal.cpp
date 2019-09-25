#include "amulet.h"
#ifdef AM_USE_METAL

// https://developer.apple.com/documentation/metal?language=objc

#if defined(AM_OSX)
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#elif defined(AM_IOS)
#import <MetalKit/MetalKit.h>
#endif
#import <Metal/Metal.h>
#include "glsl_optimizer.h"

#define AM_METAL_EXTRA_VERT_UNIFORMS_SIZE 16
#define AM_METAL_EXTRA_FRAG_UNIFORMS_SIZE 16

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

bool am_metal_use_highdpi = false;
bool am_metal_window_depth_buffer = false;
bool am_metal_window_stencil_buffer = false;
int am_metal_window_msaa_samples = 1;
int am_metal_window_swidth = 0;
int am_metal_window_sheight = 0;
int am_metal_window_pwidth = 0;
int am_metal_window_pheight = 0;

#if defined(AM_OSX)
NSWindow *am_metal_window = nil;

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
    int w = self.bounds.size.width;
    int h = self.bounds.size.height;
    float scale = self.layer.contentsScale;
    am_metal_window_pwidth = (int)(w * scale);
    am_metal_window_pheight = (int)(h * scale);
    am_metal_window_swidth = w;
    am_metal_window_sheight = h;
}

@end

static CAMetalLayer *metal_layer = nil;
static MetalView* metal_view = nil;

#elif defined(AM_IOS)

MTKView *am_metal_ios_view = NULL;

#endif

static id<MTLDevice> metal_device = nil;
static id <MTLRenderCommandEncoder> metal_encoder = nil;
static id <MTLCommandBuffer> metal_command_buffer = nil;
static id <MTLCommandQueue> metal_queue = nil;
static id<CAMetalDrawable> metal_active_drawable = nil;

static am_framebuffer_id metal_bound_framebuffer = 0;
static am_framebuffer_id metal_active_framebuffer = 0;

struct metal_framebuffer {
    int width;
    int height;
    float clear_r;
    float clear_g;
    float clear_b;
    float clear_a;

    id<MTLTexture> color_texture;
    id<MTLTexture> depth_texture;
    id<MTLTexture> stencil_texture;
    id<MTLTexture> depthstencil_texture;

    int stencil_clear_value;

    int msaa_samples;
    id<MTLTexture> msaa_texture;
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

    MTLPixelFormat color_pixel_format;
    int msaa_samples;

    bool has_depthstencil;
    bool has_depth;
    bool has_stencil;

    bool depth_test_enabled;
    bool depth_mask;
    am_depth_func depth_func;
    id<MTLDepthStencilState> mtldepthstencilstate;

    bool face_culling_enabled;
    am_cull_face_side face_cull_side;
    am_face_winding face_winding;

    bool                    stencil_test_enabled;
    am_gluint               stencil_read_mask;
    am_gluint               stencil_write_mask;
    am_stencil_func         stencil_func_back;
    am_stencil_func         stencil_func_front;
    am_stencil_op           stencil_op_fail_front;
    am_stencil_op           stencil_op_zfail_front;
    am_stencil_op           stencil_op_zpass_front;
    am_stencil_op           stencil_op_fail_back;
    am_stencil_op           stencil_op_zfail_back;
    am_stencil_op           stencil_op_zpass_back;
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
    MTLSamplerDescriptor *samplerdescr;
    id<MTLSamplerState> sampler;
    bool writable;
};

static freelist<metal_texture> metal_texture_freelist;

static am_texture_id metal_bound_texture = 0;

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

static bool                    metal_stencil_test_enabled;
static am_glint                metal_stencil_ref;
static am_gluint               metal_stencil_read_mask;
static am_gluint               metal_stencil_write_mask;
static am_stencil_func         metal_stencil_func_back;
static am_stencil_func         metal_stencil_func_front;
static am_stencil_op           metal_stencil_op_fail_front;
static am_stencil_op           metal_stencil_op_zfail_front;
static am_stencil_op           metal_stencil_op_zpass_front;
static am_stencil_op           metal_stencil_op_fail_back;
static am_stencil_op           metal_stencil_op_zfail_back;
static am_stencil_op           metal_stencil_op_zpass_back;

static bool metal_face_culling_enabled;
static am_cull_face_side metal_face_cull_side;
static am_face_winding metal_face_winding;

static int metal_bound_viewport_x = -1;
static int metal_bound_viewport_y = -1;
static int metal_bound_viewport_w = -1;
static int metal_bound_viewport_h = -1;

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

static MTLWinding to_metal_winding(bool invert, am_face_winding w) {
    switch (w) {
        case AM_FACE_WIND_CW: return invert ? MTLWindingCounterClockwise : MTLWindingClockwise;
        case AM_FACE_WIND_CCW: return invert ? MTLWindingClockwise : MTLWindingCounterClockwise;
    }
    return MTLWindingCounterClockwise;
}

static MTLSamplerMinMagFilter to_metal_min_filter(am_texture_min_filter f) {
    switch (f) {
        case AM_MIN_FILTER_NEAREST: return MTLSamplerMinMagFilterNearest;
        case AM_MIN_FILTER_LINEAR: return MTLSamplerMinMagFilterLinear;
        case AM_MIN_FILTER_NEAREST_MIPMAP_NEAREST: return MTLSamplerMinMagFilterNearest;
        case AM_MIN_FILTER_LINEAR_MIPMAP_NEAREST: return MTLSamplerMinMagFilterLinear;
        case AM_MIN_FILTER_NEAREST_MIPMAP_LINEAR: return MTLSamplerMinMagFilterNearest;
        case AM_MIN_FILTER_LINEAR_MIPMAP_LINEAR: return MTLSamplerMinMagFilterLinear;
    }
    return MTLSamplerMinMagFilterNearest;
}

static MTLSamplerMinMagFilter to_metal_mag_filter(am_texture_mag_filter f) {
    switch (f) {
        case AM_MAG_FILTER_NEAREST: return MTLSamplerMinMagFilterNearest;
        case AM_MAG_FILTER_LINEAR: return MTLSamplerMinMagFilterLinear;
    }
    return MTLSamplerMinMagFilterNearest;
}

static MTLSamplerAddressMode to_metal_wrap(am_texture_wrap w) {
    switch (w) {
        case AM_TEXTURE_WRAP_CLAMP_TO_EDGE: return MTLSamplerAddressModeClampToEdge;
        case AM_TEXTURE_WRAP_MIRRORED_REPEAT: return MTLSamplerAddressModeMirrorRepeat;
        case AM_TEXTURE_WRAP_REPEAT: return MTLSamplerAddressModeRepeat;
    }
    return MTLSamplerAddressModeClampToEdge;
}

static MTLStencilOperation to_metal_stencil_op(am_stencil_op o) {
    switch (o) {
        case AM_STENCIL_OP_KEEP: return MTLStencilOperationKeep;
        case AM_STENCIL_OP_ZERO: return MTLStencilOperationZero;
        case AM_STENCIL_OP_REPLACE: return MTLStencilOperationReplace;
        case AM_STENCIL_OP_INCR: return MTLStencilOperationIncrementClamp;
        case AM_STENCIL_OP_DECR: return MTLStencilOperationDecrementClamp;
        case AM_STENCIL_OP_INVERT: return MTLStencilOperationInvert;
        case AM_STENCIL_OP_INCR_WRAP: return MTLStencilOperationIncrementWrap;
        case AM_STENCIL_OP_DECR_WRAP: return MTLStencilOperationDecrementWrap;
    }
    return MTLStencilOperationKeep;
}

static MTLCompareFunction to_metal_stencil_func(am_stencil_func f) {
    switch (f) {
        case AM_STENCIL_FUNC_NEVER: return MTLCompareFunctionNever;
        case AM_STENCIL_FUNC_ALWAYS: return MTLCompareFunctionAlways;
        case AM_STENCIL_FUNC_EQUAL: return MTLCompareFunctionEqual;
        case AM_STENCIL_FUNC_NOTEQUAL: return MTLCompareFunctionNotEqual;
        case AM_STENCIL_FUNC_LESS: return MTLCompareFunctionLess;
        case AM_STENCIL_FUNC_LEQUAL: return MTLCompareFunctionLessEqual;
        case AM_STENCIL_FUNC_GREATER: return MTLCompareFunctionGreater;
        case AM_STENCIL_FUNC_GEQUAL: return MTLCompareFunctionGreaterEqual;
    }
    return MTLCompareFunctionAlways;
}

/*
static const char* to_metal_blend_op_str(MTLBlendOperation op) {
    switch (op) {
        case MTLBlendOperationAdd: return "add";
        case MTLBlendOperationSubtract: return "sub";
        case MTLBlendOperationReverseSubtract: return "rsub";
        case MTLBlendOperationMin: return "min";
        case MTLBlendOperationMax : return "max";
    }
    return "unknown";
}

static const char* to_metal_blend_fact_str(MTLBlendFactor op) {
    switch (op) {
        case MTLBlendFactorZero: return "zero";
        case MTLBlendFactorOne: return "one";
        case MTLBlendFactorSourceColor: return "scolor";
        case MTLBlendFactorOneMinusSourceColor: return "1-scolor";
        case MTLBlendFactorSourceAlpha: return "salpha";
        case MTLBlendFactorOneMinusSourceAlpha: return "1-salpha";
        case MTLBlendFactorDestinationColor: return "dcolor";
        case MTLBlendFactorOneMinusDestinationColor: return "1-dcolor";
        case MTLBlendFactorDestinationAlpha: return "dalpha";
        case MTLBlendFactorOneMinusDestinationAlpha: return "1-dalpha";
        case MTLBlendFactorSourceAlphaSaturated: return "salphasat";
        case MTLBlendFactorBlendColor: return "blendcolor";
        case MTLBlendFactorOneMinusBlendColor: return "1-blendcolor";
        case MTLBlendFactorBlendAlpha: return "blendalpha";
        case MTLBlendFactorOneMinusBlendAlpha: return "1-blendalpha";
        case MTLBlendFactorSource1Color: return "s1color";
        case MTLBlendFactorOneMinusSource1Color: return "1-s1color";
        case MTLBlendFactorSource1Alpha: return "s1alpha";
        case MTLBlendFactorOneMinusSource1Alpha: return "1-s1alpha";
    }
    return "unknown";
}
*/

static MTLRenderPassDescriptor *make_bound_framebuffer_render_desc(bool clear_color_buf, bool clear_depth_buf, bool clear_stencil_buf) {
    assert(metal_active_drawable != nil);
    assert(metal_command_buffer != nil);

    metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
    assert(fb->color_texture != nil);
    MTLRenderPassDescriptor *renderdesc = [MTLRenderPassDescriptor renderPassDescriptor];
    MTLRenderPassColorAttachmentDescriptor *colorattachment = renderdesc.colorAttachments[0];

    if (fb->msaa_texture != nil) {
        colorattachment.texture = fb->msaa_texture;
        colorattachment.resolveTexture = fb->color_texture;
        colorattachment.storeAction = MTLStoreActionMultisampleResolve;
    } else {
        colorattachment.texture = fb->color_texture;
        colorattachment.storeAction = MTLStoreActionStore;
    }

    if (clear_color_buf) {
        colorattachment.loadAction = MTLLoadActionClear;
        colorattachment.clearColor = MTLClearColorMake(fb->clear_r, fb->clear_g, fb->clear_b, fb->clear_a);
    } else {
        colorattachment.loadAction = MTLLoadActionLoad;
    }

    if (fb->depthstencil_texture != nil) {
        renderdesc.depthAttachment.texture = fb->depthstencil_texture;
        if (clear_depth_buf) {
            renderdesc.depthAttachment.loadAction = MTLLoadActionClear;
        } else {
            renderdesc.depthAttachment.loadAction = MTLLoadActionLoad;
        }
        if (metal_bound_framebuffer == 0) {
            renderdesc.depthAttachment.storeAction = MTLStoreActionDontCare;
        } else {
            renderdesc.depthAttachment.storeAction = MTLStoreActionStore;
        }
        renderdesc.stencilAttachment.texture = fb->depthstencil_texture;
        if (clear_stencil_buf) {
            renderdesc.stencilAttachment.clearStencil = (uint32_t)fb->stencil_clear_value;
            renderdesc.stencilAttachment.loadAction = MTLLoadActionClear;
        } else {
            renderdesc.stencilAttachment.loadAction = MTLLoadActionLoad;
        }
        if (metal_bound_framebuffer == 0) {
            renderdesc.stencilAttachment.storeAction = MTLStoreActionDontCare;
        } else {
            renderdesc.stencilAttachment.storeAction = MTLStoreActionStore;
        }
    } else if (fb->depth_texture != nil) {
        renderdesc.depthAttachment.texture = fb->depth_texture;
        if (clear_depth_buf) {
            renderdesc.depthAttachment.loadAction = MTLLoadActionClear;
        } else {
            renderdesc.depthAttachment.loadAction = MTLLoadActionLoad;
        }
        if (metal_bound_framebuffer == 0) {
            renderdesc.depthAttachment.storeAction = MTLStoreActionDontCare;
        } else {
            renderdesc.depthAttachment.storeAction = MTLStoreActionStore;
        }
    } else if (fb->stencil_texture != nil) {
        renderdesc.stencilAttachment.texture = fb->stencil_texture;
        if (clear_stencil_buf) {
            renderdesc.stencilAttachment.clearStencil = (uint32_t)fb->stencil_clear_value;
            renderdesc.stencilAttachment.loadAction = MTLLoadActionClear;
        } else {
            renderdesc.stencilAttachment.loadAction = MTLLoadActionLoad;
        }
        if (metal_bound_framebuffer == 0) {
            renderdesc.stencilAttachment.storeAction = MTLStoreActionDontCare;
        } else {
            renderdesc.stencilAttachment.storeAction = MTLStoreActionStore;
        }
    }

    return renderdesc;
}

#if defined(AM_OSX)
static void create_framebuffer_depthstencil_texture(metal_framebuffer *fb) {
    if (fb->depthstencil_texture != nil) {
        [fb->depthstencil_texture release];
        fb->depthstencil_texture = nil;
    }
    MTLTextureDescriptor *texdescr = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
        width:fb->width height:fb->height mipmapped:NO];
    texdescr.usage = MTLTextureUsageRenderTarget;
    texdescr.storageMode = MTLStorageModePrivate;
    if (fb->msaa_samples > 1) {
        texdescr.sampleCount = fb->msaa_samples;
        texdescr.textureType = MTLTextureType2DMultisample;
    }
    fb->depthstencil_texture = [metal_device newTextureWithDescriptor:texdescr];
}

static void create_framebuffer_depth_texture(metal_framebuffer *fb) {
    if (fb->depth_texture != nil) {
        [fb->depth_texture release];
        fb->depth_texture = nil;
    }
    MTLTextureDescriptor *texdescr = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
        width:fb->width height:fb->height mipmapped:NO];
    texdescr.usage = MTLTextureUsageRenderTarget;
    texdescr.storageMode = MTLStorageModePrivate;
    if (fb->msaa_samples > 1) {
        texdescr.sampleCount = fb->msaa_samples;
        texdescr.textureType = MTLTextureType2DMultisample;
    }
    fb->depth_texture = [metal_device newTextureWithDescriptor:texdescr];
}

static void create_framebuffer_stencil_texture(metal_framebuffer *fb) {
    if (fb->stencil_texture != nil) {
        [fb->stencil_texture release];
        fb->stencil_texture = nil;
    }
    MTLTextureDescriptor *texdescr = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatStencil8
        width:fb->width height:fb->height mipmapped:NO];
    texdescr.usage = MTLTextureUsageRenderTarget;
    texdescr.storageMode = MTLStorageModePrivate;
    if (fb->msaa_samples > 1) {
        texdescr.sampleCount = fb->msaa_samples;
        texdescr.textureType = MTLTextureType2DMultisample;
    }
    fb->stencil_texture = [metal_device newTextureWithDescriptor:texdescr];
}

static void create_framebuffer_msaa_texture(metal_framebuffer *fb) {
    assert(fb->color_texture != nil);
    if (fb->msaa_texture != nil) {
        [fb->msaa_texture release];
        fb->msaa_texture = nil;
    }
    if (fb->msaa_samples == 1) return;
    MTLPixelFormat pxlfmt = fb->color_texture.pixelFormat;
    MTLTextureDescriptor *texdescr = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pxlfmt
        width:fb->width height:fb->height mipmapped:NO];
    texdescr.usage = MTLTextureUsageRenderTarget;
    texdescr.storageMode = MTLStorageModePrivate;
    texdescr.sampleCount = fb->msaa_samples;
    texdescr.textureType = MTLTextureType2DMultisample;
    fb->msaa_texture = [metal_device newTextureWithDescriptor:texdescr];
}
#endif

static void create_new_metal_encoder(bool clear_color_buf, bool clear_depth_buf, bool clear_stencil_buf) {
    assert(metal_encoder == nil);

    bool metal_command_buffer_was_nil = (metal_command_buffer == nil);
    if (metal_command_buffer == nil) {
        assert(default_metal_framebuffer.color_texture == nil);
        assert(metal_active_drawable == nil);
#if defined(AM_OSX)
        bool resized = false;
#endif
        if (default_metal_framebuffer.width != am_metal_window_pwidth || default_metal_framebuffer.height != am_metal_window_pheight) {
            default_metal_framebuffer.width = am_metal_window_pwidth;
            default_metal_framebuffer.height = am_metal_window_pheight;
#if defined(AM_OSX)
            metal_layer.drawableSize = CGSizeMake(am_metal_window_pwidth, am_metal_window_pheight);
            resized = true;
#endif
        }

#if defined(AM_OSX)
        metal_active_drawable = [metal_layer nextDrawable];
#elif defined(AM_IOS)
        metal_active_drawable = am_metal_ios_view.currentDrawable;
#endif
        default_metal_framebuffer.color_texture = metal_active_drawable.texture;
#if defined(AM_OSX)
        if (resized) {
            create_framebuffer_msaa_texture(&default_metal_framebuffer);
            if (am_metal_window_depth_buffer && am_metal_window_stencil_buffer) {
                create_framebuffer_depthstencil_texture(&default_metal_framebuffer);
            } else if (am_metal_window_depth_buffer) {
                create_framebuffer_depth_texture(&default_metal_framebuffer);
            } else if (am_metal_window_stencil_buffer) {
                create_framebuffer_stencil_texture(&default_metal_framebuffer);
            }
        }
#elif defined(AM_IOS)
        if (am_metal_window_depth_buffer && am_metal_window_stencil_buffer) {
            default_metal_framebuffer.depthstencil_texture = am_metal_ios_view.depthStencilTexture;
        } else if (am_metal_window_depth_buffer) {
            default_metal_framebuffer.depth_texture = am_metal_ios_view.depthStencilTexture;
        } else if (am_metal_window_stencil_buffer) {
            default_metal_framebuffer.stencil_texture = am_metal_ios_view.depthStencilTexture;
        }
        if (default_metal_framebuffer.msaa_samples > 1) {
            default_metal_framebuffer.msaa_texture = am_metal_ios_view.multisampleColorTexture;
        }
#endif

        metal_command_buffer = [metal_queue commandBuffer];
        [metal_command_buffer retain];
    }

    MTLRenderPassDescriptor *renderdesc = make_bound_framebuffer_render_desc(clear_color_buf, clear_depth_buf, clear_stencil_buf);
    if (renderdesc == NULL) {
        am_log1("%s %i", "error: attempt to use incomplete framebuffer", (int)metal_command_buffer_was_nil);
        return;
    }
    metal_encoder = [metal_command_buffer renderCommandEncoderWithDescriptor:renderdesc];
    metal_active_pipeline = -1;
    metal_active_framebuffer = metal_bound_framebuffer;
}

#if defined(AM_OSX)
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
#endif

#define check_initialized(...) {if (!metal_initialized) {am_log1("%s:%d: attempt to call %s before metal initialized", __FILE__, __LINE__, __func__); return __VA_ARGS__;}}

static bool metal_initialized = false;

static void set_framebuffer_msaa_samples(metal_framebuffer *fb, int samples) {
    if (metal_device == nil) {
        fb->msaa_samples = 1;
        return;
    }
    while (samples > 1) {
        if ([metal_device supportsTextureSampleCount:samples]) {
            fb->msaa_samples = samples;
            return;
        } else {
            samples--;
        }
    }
    fb->msaa_samples = 1;
}

void am_init_gl() {
#if defined (AM_OSX)
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
    if (@available(macOS 10.13, iOS 11.0, *)) {
        metal_layer.allowsNextDrawableTimeout = NO;
        metal_layer.displaySyncEnabled = (am_conf_vsync ? YES : NO);
    }
    metal_active_drawable = [metal_layer nextDrawable];
#elif defined(AM_IOS)
    metal_device = am_metal_ios_view.device;
    [metal_device retain];
    metal_active_drawable = [am_metal_ios_view currentDrawable];
#endif

    metal_queue = [metal_device newCommandQueue];
    [metal_queue retain];
    metal_command_buffer = [metal_queue commandBuffer];
    [metal_command_buffer retain];

#if defined(AM_OSX)
    default_metal_framebuffer.width = metal_layer.drawableSize.width;
    default_metal_framebuffer.height = metal_layer.drawableSize.height;
#elif defined(AM_IOS)
    default_metal_framebuffer.width = am_metal_ios_view.drawableSize.width;
    default_metal_framebuffer.height = am_metal_ios_view.drawableSize.height;
#endif
    am_metal_window_pwidth = default_metal_framebuffer.width;
    am_metal_window_pheight = default_metal_framebuffer.height;
#if defined(AM_OSX)
    float scale = metal_layer.contentsScale;
#elif defined(AM_IOS)
    float scale = am_metal_ios_view.contentScaleFactor;
#endif
    am_metal_window_swidth = (int)((float)am_metal_window_pwidth / scale);
    am_metal_window_sheight = (int)((float)am_metal_window_pheight / scale);
    default_metal_framebuffer.clear_r = 0.0f;
    default_metal_framebuffer.clear_g = 0.0f;
    default_metal_framebuffer.clear_b = 0.0f;
    default_metal_framebuffer.clear_a = 1.0f;
    default_metal_framebuffer.color_texture = metal_active_drawable.texture;
    default_metal_framebuffer.depth_texture = nil;
    default_metal_framebuffer.stencil_texture = nil;
    default_metal_framebuffer.depthstencil_texture = nil;
    default_metal_framebuffer.stencil_clear_value = 0;
    default_metal_framebuffer.msaa_samples = 1;
    default_metal_framebuffer.msaa_texture = nil;
    set_framebuffer_msaa_samples(&default_metal_framebuffer, am_metal_window_msaa_samples);
#if defined(AM_OSX)
    if (am_metal_window_depth_buffer && am_metal_window_stencil_buffer) {
        create_framebuffer_depthstencil_texture(&default_metal_framebuffer);
    } else if (am_metal_window_depth_buffer) {
        create_framebuffer_depth_texture(&default_metal_framebuffer);
    } else if (am_metal_window_stencil_buffer) {
        create_framebuffer_stencil_texture(&default_metal_framebuffer);
    }
    create_framebuffer_msaa_texture(&default_metal_framebuffer);
#elif defined(AM_IOS)
    // textures are created by MTKView
#endif

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

    for (int i = 0; i < NUM_TEXTURE_UNITS; i++) {
        texture_units[i] = -1;
    }

    metal_bound_texture = 0;
    metal_bound_framebuffer = 0;
    metal_active_element_buffer = 0;
    metal_active_array_buffer = 0;
    metal_active_pipeline = -1;
    metal_active_program = 0;

    metal_blend_enabled = false;
    metal_blend_eq_rgb = AM_BLEND_EQUATION_ADD;
    metal_blend_eq_alpha = AM_BLEND_EQUATION_ADD;
    metal_blend_sfactor_rgb = AM_BLEND_SFACTOR_SRC_ALPHA;
    metal_blend_sfactor_alpha = AM_BLEND_SFACTOR_SRC_ALPHA;
    metal_blend_dfactor_rgb = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;
    metal_blend_dfactor_alpha = AM_BLEND_DFACTOR_ONE_MINUS_SRC_ALPHA;

    metal_depth_test_enabled = false;
    metal_depth_mask = false;
    metal_depth_func = AM_DEPTH_FUNC_ALWAYS;

    metal_stencil_test_enabled = false;
    metal_stencil_ref = 0;
    metal_stencil_read_mask = 255;
    metal_stencil_write_mask = 255;
    metal_stencil_func_front = AM_STENCIL_FUNC_ALWAYS;
    metal_stencil_op_fail_front = AM_STENCIL_OP_KEEP;
    metal_stencil_op_zfail_front = AM_STENCIL_OP_KEEP;
    metal_stencil_op_zpass_front = AM_STENCIL_OP_KEEP;
    metal_stencil_func_back = AM_STENCIL_FUNC_ALWAYS;
    metal_stencil_op_fail_back = AM_STENCIL_OP_KEEP;
    metal_stencil_op_zfail_back = AM_STENCIL_OP_KEEP;
    metal_stencil_op_zpass_back = AM_STENCIL_OP_KEEP;

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
    if (metal_command_buffer != nil) {
        [metal_command_buffer release];
        metal_command_buffer = nil;
    }
    metal_encoder = nil;
    metal_bound_framebuffer = 0;
#if defined (AM_OSX)
    if (default_metal_framebuffer.depthstencil_texture != nil) {
        [default_metal_framebuffer.depthstencil_texture release];
        default_metal_framebuffer.depthstencil_texture = nil;
    }
    if (default_metal_framebuffer.depth_texture != nil) {
        [default_metal_framebuffer.depth_texture release];
        default_metal_framebuffer.depth_texture = nil;
    }
    if (default_metal_framebuffer.stencil_texture != nil) {
        [default_metal_framebuffer.stencil_texture release];
        default_metal_framebuffer.stencil_texture = nil;
    }
    if (default_metal_framebuffer.msaa_texture != nil) {
        [default_metal_framebuffer.msaa_texture release];
        default_metal_framebuffer.msaa_texture = nil;
    }
#elif defined(AM_IOS)
    // in iOS the MTKView will release the textures
    default_metal_framebuffer.depthstencil_texture = nil;
    default_metal_framebuffer.depth_texture = nil;
    default_metal_framebuffer.stencil_texture = nil;
    default_metal_framebuffer.msaa_texture = nil;
#endif
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
    metal_stencil_test_enabled = true;
}

void am_set_stencil_func(am_glint ref, am_gluint mask, am_stencil_func func_front, am_stencil_func func_back) {
    check_initialized();
    metal_stencil_ref = ref;
    metal_stencil_read_mask = mask;
    metal_stencil_func_front = func_front;
    metal_stencil_func_back = func_back;
}

void am_set_stencil_op(am_stencil_face_side face, am_stencil_op fail, am_stencil_op zfail, am_stencil_op zpass) {
    check_initialized();
    switch (face) {
        case AM_STENCIL_FACE_FRONT:
            metal_stencil_op_fail_front = fail;
            metal_stencil_op_zfail_front = zfail;
            metal_stencil_op_zpass_front = zpass;
            break;
        case AM_STENCIL_FACE_BACK:
            metal_stencil_op_fail_back = fail;
            metal_stencil_op_zfail_back = zfail;
            metal_stencil_op_zpass_back = zpass;
            break;
    }
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
    // We'll clear the default framebuffer when we create the encoder.
    // Creating a separate encoder to clear the default framebuffer doesn't seem to
    // work on iOS, though it works on macOS.
    if (metal_bound_framebuffer == 0) return;
    create_new_metal_encoder(clear_color_buf, clear_depth_buf, clear_stencil_buf);
    [metal_encoder endEncoding];
    metal_encoder = nil;
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
    metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
    fb->stencil_clear_value = val;
}

void am_set_framebuffer_color_mask(bool r, bool g, bool b, bool a) {
    check_initialized();
    // TODO
}

void am_set_framebuffer_depth_mask(bool flag) {
    check_initialized();
    metal_depth_mask = flag;
}

void am_set_framebuffer_stencil_mask(am_gluint mask) {
    check_initialized();
    metal_stencil_write_mask = mask;
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
        buf->size = size;
    } else {
        memcpy([buf->mtlbuf contents], data, size);
    }
}

void am_set_buffer_sub_data(am_buffer_target target, int offset, int size, void *data) {
    check_initialized();
    metal_buffer *buf = get_active_buffer(target);
    if (buf == NULL) return;
    uint8_t *contents = (uint8_t*)[buf->mtlbuf contents];
    memcpy(&contents[offset], data, size);
}

void am_delete_buffer(am_buffer_id id) {
    check_initialized();
    metal_buffer *buf = metal_buffer_freelist.get(id);
    if (buf->mtlbuf != nil) {
        [buf->mtlbuf release];
        buf->mtlbuf = nil;
    }
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
    metal_bound_viewport_x = x;
    metal_bound_viewport_y = y;
    metal_bound_viewport_w = w;
    metal_bound_viewport_h = h;
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
    am_program_id progid = metal_program_freelist.add(prog);
    return progid;
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
        //printf("Metal:\n%s\n", metal_src);
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
    int vert_bytes = glslopt_shader_get_uniform_total_size(vert->gshader) + AM_METAL_EXTRA_VERT_UNIFORMS_SIZE;
    int frag_bytes = glslopt_shader_get_uniform_total_size(frag->gshader) + AM_METAL_EXTRA_FRAG_UNIFORMS_SIZE;
    // round sizes up to nearest 16 bytes to avoid metal assertion failures
    while (vert_bytes & (16-1)) vert_bytes++;
    while (frag_bytes & (16-1)) frag_bytes++;
    prog->vert_uniform_size = vert_bytes;
    prog->vert_uniform_data = (uint8_t*)malloc(vert_bytes);
    memset(prog->vert_uniform_data, 0, vert_bytes);
    prog->frag_uniform_size = frag_bytes;
    prog->frag_uniform_data = (uint8_t*)malloc(frag_bytes);
    memset(prog->frag_uniform_data, 0, frag_bytes);
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
            loc += AM_METAL_EXTRA_VERT_UNIFORMS_SIZE;
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
            loc += AM_METAL_EXTRA_FRAG_UNIFORMS_SIZE;
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
    tex.samplerdescr = [[MTLSamplerDescriptor alloc] init];
    tex.sampler = nil;
    tex.writable = false;
    return metal_texture_freelist.add(tex);
}

void am_delete_texture(am_texture_id id) {
    check_initialized();
    metal_texture *tex = metal_texture_freelist.get(id);
    if (tex->tex != nil) {
        [tex->tex release];
        [tex->samplerdescr release];
        [tex->sampler release];
        tex->tex = nil;
        tex->samplerdescr = nil;
        tex->sampler = nil;
    }
    metal_texture_freelist.remove(id);
    if (metal_bound_texture == id) metal_bound_texture = 0;
}

void am_bind_texture(am_texture_bind_target target, am_texture_id texture) {
    check_initialized();
    if (active_texture_unit >= 0) {
        texture_units[active_texture_unit] = texture;
    }
    metal_bound_texture = texture;
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
    if (metal_bound_texture == 0) return;
    metal_texture *tex = metal_texture_freelist.get(metal_bound_texture);
    if (tex->tex != nil) {
        [tex->tex release];
        tex->tex = nil;
    }
    MTLTextureDescriptor *texdescr = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];
    tex->tex = [metal_device newTextureWithDescriptor:texdescr];
    [tex->tex replaceRegion:MTLRegionMake2D(0, 0, w, h) mipmapLevel:level withBytes:data bytesPerRow:w*4];
}

void am_set_texture_sub_image_2d(am_texture_copy_target target, int level, int xoffset, int yoffset, int w, int h, am_texture_format format, am_texture_type type, void *data) {
    check_initialized();
    if (metal_bound_texture == 0) return;
    metal_texture *tex = metal_texture_freelist.get(metal_bound_texture);
    if (tex->tex == nil) return;
    [tex->tex replaceRegion:MTLRegionMake2D(xoffset, yoffset, w, h) mipmapLevel:level withBytes:data bytesPerRow:w*4];
}

void am_set_texture_min_filter(am_texture_bind_target target, am_texture_min_filter filter) {
    check_initialized();
    metal_texture *tex = metal_texture_freelist.get(metal_bound_texture);
    tex->samplerdescr.minFilter = to_metal_min_filter(filter);
    if (tex->sampler != nil) {
        [tex->sampler release];
        tex->sampler = nil;
    }
}

void am_set_texture_mag_filter(am_texture_bind_target target, am_texture_mag_filter filter) {
    check_initialized();
    metal_texture *tex = metal_texture_freelist.get(metal_bound_texture);
    tex->samplerdescr.magFilter = to_metal_mag_filter(filter);
    if (tex->sampler != nil) {
        [tex->sampler release];
        tex->sampler = nil;
    }
}

void am_set_texture_wrap(am_texture_bind_target target, am_texture_wrap s_wrap, am_texture_wrap t_wrap) {
    check_initialized();
    metal_texture *tex = metal_texture_freelist.get(metal_bound_texture);
    tex->samplerdescr.sAddressMode = to_metal_wrap(s_wrap);
    tex->samplerdescr.tAddressMode = to_metal_wrap(t_wrap);
    if (tex->sampler != nil) {
        [tex->sampler release];
        tex->sampler = nil;
    }
}

// Renderbuffer Objects

am_renderbuffer_id am_create_renderbuffer() {
    check_initialized(0);
    metal_renderbuffer renderbuffer;
    renderbuffer.initialized = false;
    renderbuffer.format = AM_RENDERBUFFER_FORMAT_DEPTHSTENCIL;
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
        case AM_RENDERBUFFER_FORMAT_DEPTHSTENCIL: pxlfmt = MTLPixelFormatDepth32Float_Stencil8; break;
        case AM_RENDERBUFFER_FORMAT_DEPTH: pxlfmt = MTLPixelFormatDepth32Float; break;
        case AM_RENDERBUFFER_FORMAT_STENCIL: pxlfmt = MTLPixelFormatStencil8; break;
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

    metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
    assert(fb->color_texture != nil);

    if (metal_encoder != nil) {
        [metal_encoder endEncoding];
        metal_encoder = nil;
    }
    bool was_command_buffer = true;
    if (metal_command_buffer == nil) {
        metal_command_buffer = [metal_queue commandBuffer];
        [metal_command_buffer retain];
        was_command_buffer = false;
    }
// only needed if texture is MTLStorageModeManaged I think
// #if defined(AM_OSX)
//     id<MTLBlitCommandEncoder> blit_encoder = [metal_command_buffer blitCommandEncoder];
//     [blit_encoder synchronizeTexture:fb->color_texture slice:0 level:0];
//     [blit_encoder endEncoding];
// #endif
    [metal_command_buffer commit];
    [metal_command_buffer waitUntilCompleted];
    [metal_command_buffer release];
    metal_command_buffer = nil;

    MTLRegion region;
    region.origin.x = x;
    region.origin.y = y;
    region.origin.z = 0;
    region.size.width = w;
    region.size.height = h;
    region.size.depth = 1;
    [fb->color_texture getBytes:data bytesPerRow:w*4 fromRegion:region mipmapLevel: 0];

    if (was_command_buffer) {
        metal_command_buffer = [metal_queue commandBuffer];
        [metal_command_buffer retain];
    }
}

// Framebuffer Objects

am_framebuffer_id am_create_framebuffer() {
    check_initialized(0);
    metal_framebuffer fb;
    fb.width = 0;
    fb.height = 0;
    fb.clear_r = 0.0f;
    fb.clear_g = 0.0f;
    fb.clear_b = 0.0f;
    fb.clear_a = 1.0f;
    fb.color_texture = nil;
    fb.depthstencil_texture = nil;
    fb.depth_texture = nil;
    fb.stencil_texture = nil;
    fb.stencil_clear_value = 0;
    set_framebuffer_msaa_samples(&fb, 1);
    fb.msaa_texture = nil;
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
    metal_framebuffer *fb = metal_framebuffer_freelist.get(metal_bound_framebuffer);
    if (fb->color_texture == nil) {
        return AM_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT;
    }
    if (fb->msaa_samples > 1 && fb->msaa_texture == nil) {
        return AM_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT;
    }
    if (fb->width == 0 || fb->height == 0) {
        return AM_FRAMEBUFFER_STATUS_INCOMPLETE_DIMENSIONS;
    }
    return AM_FRAMEBUFFER_STATUS_COMPLETE;
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
            fb->stencil_texture = renderbuffer->texture;
            break;
        case AM_FRAMEBUFFER_DEPTHSTENCIL_ATTACHMENT:
            fb->depthstencil_texture = renderbuffer->texture;
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
            // ensure texture can be a render target
            if (!tex->writable) {
                id<MTLTexture> old_tex = tex->tex;
                int w = old_tex.width;
                int h = old_tex.height;
                MTLTextureDescriptor *texdescr = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];
                texdescr.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
                tex->tex = [metal_device newTextureWithDescriptor:texdescr];
                bool was_command_buffer = true;
                if (metal_command_buffer == nil) {
                    metal_command_buffer = [metal_queue commandBuffer];
                    [metal_command_buffer retain];
                    was_command_buffer = false;
                }
                id<MTLBlitCommandEncoder> blit_encoder = [metal_command_buffer blitCommandEncoder];
                [blit_encoder 
                    copyFromTexture:old_tex
                    sourceSlice:0 sourceLevel:0
                    sourceOrigin:MTLOriginMake(0,0,0) 
                    sourceSize:MTLSizeMake(w,h,1)
                    toTexture: tex->tex 
                    destinationSlice: 0
                    destinationLevel: 0
                    destinationOrigin: MTLOriginMake(0,0,0)];
                [blit_encoder endEncoding];
                [metal_command_buffer commit];
                [metal_command_buffer waitUntilCompleted];
                [metal_command_buffer release];
                metal_command_buffer = nil;
                if (was_command_buffer) {
                    metal_command_buffer = [metal_queue commandBuffer];
                    [metal_command_buffer retain];
                }
                [old_tex release];
                tex->writable = true;
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
            break;
        case AM_FRAMEBUFFER_DEPTH_ATTACHMENT:
            // unsupported
            break;
        case AM_FRAMEBUFFER_STENCIL_ATTACHMENT:
            // unsupported
            break;
        case AM_FRAMEBUFFER_DEPTHSTENCIL_ATTACHMENT:
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
    metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
    assert(fb->color_texture != nil);
    MTLPixelFormat colorpxlfmt = fb->color_texture.pixelFormat;
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
            cached_pipeline->color_pixel_format != colorpxlfmt ||
            cached_pipeline->msaa_samples != fb->msaa_samples ||
            cached_pipeline->has_depthstencil != (fb->depthstencil_texture != nil) ||
            cached_pipeline->has_depth != (fb->depth_texture != nil) ||
            cached_pipeline->has_stencil != (fb->stencil_texture != nil) ||
            cached_pipeline->depth_test_enabled != metal_depth_test_enabled ||
            cached_pipeline->depth_mask != metal_depth_mask ||
            cached_pipeline->depth_func != metal_depth_func ||
            cached_pipeline->face_culling_enabled != metal_face_culling_enabled ||
            cached_pipeline->face_cull_side != metal_face_cull_side ||
            cached_pipeline->face_winding != metal_face_winding ||
            cached_pipeline->stencil_test_enabled      != metal_stencil_test_enabled ||
            cached_pipeline->stencil_read_mask         != metal_stencil_read_mask ||
            cached_pipeline->stencil_write_mask        != metal_stencil_write_mask ||
            cached_pipeline->stencil_func_back         != metal_stencil_func_back ||
            cached_pipeline->stencil_func_front        != metal_stencil_func_front ||
            cached_pipeline->stencil_op_fail_front     != metal_stencil_op_fail_front ||
            cached_pipeline->stencil_op_zfail_front    != metal_stencil_op_zfail_front ||
            cached_pipeline->stencil_op_zpass_front    != metal_stencil_op_zpass_front ||
            cached_pipeline->stencil_op_fail_back      != metal_stencil_op_fail_back ||
            cached_pipeline->stencil_op_zfail_back     != metal_stencil_op_zfail_back ||
            cached_pipeline->stencil_op_zpass_back     != metal_stencil_op_zpass_back ||
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
        pldescr.sampleCount = fb->msaa_samples;

        pldescr.colorAttachments[0].pixelFormat = colorpxlfmt;
        pldescr.colorAttachments[0].blendingEnabled = to_objc_bool(metal_blend_enabled);
        pldescr.colorAttachments[0].rgbBlendOperation = to_metal_blend_op(metal_blend_eq_rgb);
        pldescr.colorAttachments[0].alphaBlendOperation = to_metal_blend_op(metal_blend_eq_alpha);
        pldescr.colorAttachments[0].sourceRGBBlendFactor = to_metal_blend_sfactor(metal_blend_sfactor_rgb);
        pldescr.colorAttachments[0].sourceAlphaBlendFactor = to_metal_blend_sfactor(metal_blend_sfactor_alpha);
        pldescr.colorAttachments[0].destinationRGBBlendFactor = to_metal_blend_dfactor(metal_blend_dfactor_rgb);
        pldescr.colorAttachments[0].destinationAlphaBlendFactor = to_metal_blend_dfactor(metal_blend_dfactor_alpha);

        if (fb->depthstencil_texture != nil) {
            pldescr.depthAttachmentPixelFormat = fb->depthstencil_texture.pixelFormat;
            pldescr.stencilAttachmentPixelFormat = fb->depthstencil_texture.pixelFormat;
        }
        if (fb->depth_texture != nil) {
            pldescr.depthAttachmentPixelFormat = fb->depth_texture.pixelFormat;
        }
        if (fb->stencil_texture != nil) {
            pldescr.stencilAttachmentPixelFormat = fb->stencil_texture.pixelFormat;
        }
        /*
        am_debug(
            "\nrgbBlendOperation = %s\nalphaBlendOperation = %s\nsourceRGBBlendFactor = %s\nsourceAlphaBlendFactor = %s\ndestinationRGBBlendFactor = %s\ndestinationAlphaBlendFactor = %s\n",
            to_metal_blend_op_str(pldescr.colorAttachments[0].rgbBlendOperation),
            to_metal_blend_op_str(pldescr.colorAttachments[0].alphaBlendOperation),
            to_metal_blend_fact_str(pldescr.colorAttachments[0].sourceRGBBlendFactor),
            to_metal_blend_fact_str(pldescr.colorAttachments[0].sourceAlphaBlendFactor),
            to_metal_blend_fact_str(pldescr.colorAttachments[0].destinationRGBBlendFactor),
            to_metal_blend_fact_str(pldescr.colorAttachments[0].destinationAlphaBlendFactor));
        */

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
        cached_pipeline.color_pixel_format = colorpxlfmt;
        cached_pipeline.msaa_samples = fb->msaa_samples;
        cached_pipeline.mtlpipeline = pl;

        cached_pipeline.has_depthstencil = (fb->depthstencil_texture != nil);;
        cached_pipeline.has_depth = (fb->depth_texture != nil);;
        cached_pipeline.has_stencil = (fb->stencil_texture != nil);;

        cached_pipeline.depth_test_enabled = metal_depth_test_enabled;
        cached_pipeline.depth_mask = metal_depth_mask;
        cached_pipeline.depth_func = metal_depth_func;

        cached_pipeline.stencil_test_enabled      = metal_stencil_test_enabled;
        cached_pipeline.stencil_read_mask         = metal_stencil_read_mask;
        cached_pipeline.stencil_write_mask        = metal_stencil_write_mask;
        cached_pipeline.stencil_func_back         = metal_stencil_func_back;
        cached_pipeline.stencil_func_front        = metal_stencil_func_front;
        cached_pipeline.stencil_op_fail_front     = metal_stencil_op_fail_front;
        cached_pipeline.stencil_op_zfail_front    = metal_stencil_op_zfail_front;
        cached_pipeline.stencil_op_zpass_front    = metal_stencil_op_zpass_front;
        cached_pipeline.stencil_op_fail_back      = metal_stencil_op_fail_back;
        cached_pipeline.stencil_op_zfail_back     = metal_stencil_op_zfail_back;
        cached_pipeline.stencil_op_zpass_back     = metal_stencil_op_zpass_back;

        MTLDepthStencilDescriptor *depthstencildescr = [[MTLDepthStencilDescriptor alloc] init];
        if (metal_depth_test_enabled) {
            depthstencildescr.depthCompareFunction = to_metal_depth_func(metal_depth_func);
            depthstencildescr.depthWriteEnabled = to_objc_bool(metal_depth_mask);
        } else {
            depthstencildescr.depthCompareFunction = MTLCompareFunctionAlways;
            depthstencildescr.depthWriteEnabled = NO;
        }
        if (metal_stencil_test_enabled) {
            MTLStencilDescriptor *sdescr = [[MTLStencilDescriptor alloc] init];
            sdescr.readMask = metal_stencil_read_mask;
            sdescr.writeMask = metal_stencil_write_mask;
            sdescr.stencilFailureOperation = to_metal_stencil_op(metal_stencil_op_fail_front);
            sdescr.depthFailureOperation = to_metal_stencil_op(metal_stencil_op_zfail_front);
            sdescr.depthStencilPassOperation = to_metal_stencil_op(metal_stencil_op_zpass_front);
            sdescr.stencilCompareFunction = to_metal_stencil_func(metal_stencil_func_front);
            depthstencildescr.frontFaceStencil = sdescr;
            sdescr.stencilFailureOperation = to_metal_stencil_op(metal_stencil_op_fail_back);
            sdescr.depthFailureOperation = to_metal_stencil_op(metal_stencil_op_zfail_back);
            sdescr.depthStencilPassOperation = to_metal_stencil_op(metal_stencil_op_zpass_back);
            sdescr.stencilCompareFunction = to_metal_stencil_func(metal_stencil_func_back);
            depthstencildescr.backFaceStencil = sdescr;
            [sdescr release];
        } else {
            MTLStencilDescriptor *sdescr = [[MTLStencilDescriptor alloc] init];
            sdescr.readMask = 0;
            sdescr.writeMask = 0;
            sdescr.stencilFailureOperation = MTLStencilOperationKeep;
            sdescr.depthFailureOperation = MTLStencilOperationKeep;
            sdescr.depthStencilPassOperation = MTLStencilOperationKeep;
            sdescr.stencilCompareFunction = MTLCompareFunctionAlways;
            depthstencildescr.frontFaceStencil = sdescr;
            depthstencildescr.backFaceStencil = sdescr;
            [sdescr release];
        }
        cached_pipeline.mtldepthstencilstate = [metal_device newDepthStencilStateWithDescriptor: depthstencildescr];
        [depthstencildescr release];

        cached_pipeline.face_culling_enabled = metal_face_culling_enabled;
        cached_pipeline.face_cull_side = metal_face_cull_side;
        cached_pipeline.face_winding = metal_face_winding;

        selected_pipeline = prog->pipeline_cache.size();
        prog->pipeline_cache.push_back(cached_pipeline);
    }
    if (selected_pipeline != metal_active_pipeline) {
        metal_pipeline *pipeline = &prog->pipeline_cache[selected_pipeline];
        [metal_encoder setRenderPipelineState: pipeline->mtlpipeline];
        metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
        if (fb->depthstencil_texture != nil || fb->depth_texture != nil || fb->stencil_texture != nil) {
            [metal_encoder setDepthStencilState:pipeline->mtldepthstencilstate];
        }
        [metal_encoder setFrontFacingWinding: to_metal_winding(metal_bound_framebuffer != 0, pipeline->face_winding)];
        [metal_encoder setCullMode: to_metal_cull_mode(pipeline->face_culling_enabled, pipeline->face_cull_side)];
        metal_active_pipeline = selected_pipeline;
    }
    return true;
}

static bool pre_draw_setup() {
    if (metal_encoder == nil) {
        bool clear = metal_bound_framebuffer == 0;
        create_new_metal_encoder(clear, clear, clear);
    }

    metal_program *prog = metal_program_freelist.get(metal_active_program);

    if (metal_bound_viewport_x != -1) {
        metal_framebuffer *fb = get_metal_framebuffer(metal_bound_framebuffer);
        MTLViewport vp;
        vp.originX = metal_bound_viewport_x;
        vp.originY = fb->height - metal_bound_viewport_y - metal_bound_viewport_h;
        vp.width = metal_bound_viewport_w;
        vp.height = metal_bound_viewport_h;
        vp.znear = 0;
        vp.zfar = 1;
        [metal_encoder setViewport:vp];
    }

    // pipeline
    if (!setup_pipeline(prog)) return false;

    // stencil ref value
    if (metal_stencil_test_enabled) {
        [metal_encoder setStencilReferenceValue:(uint32_t)metal_stencil_ref];
    }

    // uniforms
    // set _am_y_flip to flip y if rendering to a texture
    if (metal_bound_framebuffer == 0) {
        *((float*)(prog->vert_uniform_data)) = 1.0f;
        *((float*)(prog->frag_uniform_data)) = 1.0f;
    } else {
        *((float*)(prog->vert_uniform_data)) = -1.0f;
        *((float*)(prog->frag_uniform_data)) = -1.0f;
    }
    [metal_encoder setVertexBytes:prog->vert_uniform_data length:prog->vert_uniform_size atIndex:0];
    [metal_encoder setFragmentBytes:prog->frag_uniform_data length:prog->frag_uniform_size atIndex:0];
    
    // textures
    for (int i = 0; i < prog->num_uniforms; i++) {
        uniform_descr *uni = &prog->uniforms[i];
        if (uni->tex_unit >= 0) {
            am_texture_id texid = texture_units[uni->tex_unit];
            metal_texture *tex = metal_texture_freelist.get(texid);
            if (tex->sampler == nil) {
                tex->sampler = [metal_device newSamplerStateWithDescriptor:tex->samplerdescr];
            }
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
    if (elembuf->mtlbuf == nil) return;
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

void am_gl_end_framebuffer_render() {
    check_initialized();
    if (metal_encoder != nil) {
        [metal_encoder endEncoding];
        metal_encoder = nil;
    }
}

void am_gl_end_frame(bool present) {
    check_initialized();
    if (metal_command_buffer == nil) {
        // could be nil if nothing drawn this frame
        create_new_metal_encoder(true, true, true);
        [metal_encoder endEncoding];
        metal_encoder = nil;
    }
    assert(metal_active_drawable != nil);
    assert(default_metal_framebuffer.color_texture != nil);
    if (present) {
        [metal_command_buffer presentDrawable:metal_active_drawable];
    }
    [metal_command_buffer commit];
    [metal_command_buffer waitUntilCompleted];
    [metal_command_buffer release];
    metal_command_buffer = nil;
    metal_active_drawable = nil;
    default_metal_framebuffer.color_texture = nil;
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

bool am_gl_requires_combined_depthstencil() {
    return true;
}

#endif // AM_USE_METAL
