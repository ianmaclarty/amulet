#include "amulet.h"

#ifdef AM_BACKEND_IOS

#import <objc/runtime.h>
#import <QuartzCore/QuartzCore.h>

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioServices.h>
#import <CoreMotion/CoreMotion.h>

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#include <mach/mach_time.h>

static lua_State *ios_L = NULL;
static am_package *package = NULL;
static am_display_orientation ios_orientation = AM_DISPLAY_ORIENTATION_ANY;
static bool ios_window_created = false;
static bool ios_running = false;
static bool init_run = false;
static int screen_width = 0;
static int screen_height = 0;

static double t0;
static double t_debt;
static double frame_time = 0.0;
static double delta_time;
static double real_delta_time;

#define MIN_UPDATE_TIME (1.0/400.0)

//---------------------------------------------------------------------------

/*
static float ios_scaling() {
    float scale = 1.0f;
    if([[UIScreen mainScreen] respondsToSelector: NSSelectorFromString(@"scale")]) {
        scale = [[UIScreen mainScreen] scale];
    }
    return scale;
}

static bool ios_is_ipad() {
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
        return true;
    } else {
        return false;
    }
}

static bool ios_is_retina_iphone() {
    return !ios_is_ipad() && ios_scaling() >= 2.0f;
}
*/

/*
static const char *ios_doc_path(const char *file, const char *suffix) {
    NSArray *dirs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    const char *dir = [[dirs objectAtIndex:0] UTF8String];
    int len = strlen(dir) + strlen(file) + 2;
    if (suffix != NULL) {
        len += strlen(suffix);
    }
    char *path = (char*)malloc(len);
    if (suffix == NULL) {
        snprintf(path, len, "%s/%s", dir, file);
    } else {
        snprintf(path, len, "%s/%s%s", dir, file, suffix);
    }
    return path;
}
*/

static const char *ios_bundle_path() {
    return [[[NSBundle mainBundle] bundlePath] UTF8String];
}

//------------- prefs store ----------------

static NSUserDefaults *prefs = nil;

/*
static void ensure_prefs_initialized() {
    if (prefs == nil) {
        prefs = [NSUserDefaults standardUserDefaults];
    }
}

void ltIOSStorePickledData(const char *key, LTPickler *pickler) {
    ensure_prefs_initialized();
    NSData *nsdata = [NSData dataWithBytesNoCopy:pickler->data length:pickler->size freeWhenDone:NO];
    NSString *skey = [NSString stringWithUTF8String:key];
    [prefs setObject:nsdata forKey:skey];
}

LTUnpickler *ltIOSRetrievePickledData(const char *key) {
    ensure_prefs_initialized();
    NSData *nsdata = [prefs dataForKey:[NSString stringWithUTF8String:key]];
    if (nsdata != nil) {
        LTUnpickler *unpickler = new LTUnpickler(nsdata.bytes, nsdata.length);
        return unpickler;
    } else {
        return NULL;
    }
}
*/

static void ios_sync_store() {
    if (prefs != nil) {
        [prefs synchronize];
    }
}

/*
static void ios_launch_url(const char *url) {
    NSString *ns_url = [NSString stringWithUTF8String:url];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:ns_url]];
}
*/

// ---------------------------------------------------------------------------


@class AMViewController;
@class AMView;
static AMViewController *am_view_controller = nil;
static AMView *am_view = nil;

static void set_audio_category() {
    UInt32 category = kAudioSessionCategory_AmbientSound;
    AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
}

static void ios_audio_suspend() {
}

static void ios_audio_resume() {
}

static void audio_interrupt(void *ud, UInt32 state) {
    if (state == kAudioSessionBeginInterruption) {
        AudioSessionSetActive(NO);
        ios_audio_suspend();
    } else if (state == kAudioSessionEndInterruption) {
        set_audio_category();
        AudioSessionSetActive(YES);
        ios_audio_resume();
    }
}

static bool open_package() {
    char *package_filename = am_format("%s/%s", am_opt_data_dir, "data.pak");
    if (am_file_exists(package_filename)) {
        char *errmsg;
        package = am_open_package(package_filename, &errmsg);
        if (package == NULL) {
            am_log0("%s", errmsg);
            free(errmsg);
            free(package_filename);
            return false;
        }
    }
    free(package_filename);
    return true;
}

static void ios_init() {
    ios_running = false;

    am_opt_data_dir = ios_bundle_path();
    am_opt_main_module = "main";
    if (!open_package()) return;
    ios_L = am_init_engine(false, 0, NULL);
    if (ios_L == NULL) return;

    frame_time = am_get_current_time();
    lua_pushcclosure(ios_L, am_load_module, 0);
    lua_pushstring(ios_L, am_opt_main_module);
    if (am_call(ios_L, 1, 0)) {
        ios_running = true;
    }
    t0 = am_get_current_time();
    frame_time = t0;
    t_debt = 0.0;
}

static void set_view_controller(AMViewController *view_c) {
    am_view_controller = view_c;
    [am_view_controller retain];
}

static void ios_teardown() {
    ios_running = false;
    if (ios_L != NULL) {
        am_destroy_engine(ios_L);
        ios_L = NULL;
    }
    if (am_gl_is_initialized()) {
        am_destroy_gl();
    }
    if (package != NULL) {
        am_close_package(package);
    }
    if (am_view_controller != nil) {
        [am_view_controller release];
        am_view_controller = nil;
    }
}

static void ios_resize(int width, int height) {
    screen_width = width;
    screen_height = height;
}

// This is used to decide whether rotations should
// be animated (we don't want to animate the initial
// rotation after app launch), as well as whether to
// disable rotations altogether when the app is using
// the accelerometer for input (we want to however
// allow the initial rotation).
static int frames_since_disable_animations = 0;

static void ios_render() {
    if (frames_since_disable_animations == 60) {
        [UIView setAnimationsEnabled:YES];
    }
    frames_since_disable_animations++;

    if (!ios_running) return;
    if (!am_update_windows(ios_L)) {
        ios_running = false;
        return;
    }

    frame_time = am_get_current_time();
    
    real_delta_time = frame_time - t0;
    if (am_conf_warn_delta_time > 0.0 && real_delta_time > am_conf_warn_delta_time) {
        am_log0("WARNING: FPS dropped to %0.2f (%fs)", 1.0/real_delta_time, real_delta_time);
    }
    delta_time = am_min(am_conf_min_delta_time, real_delta_time); // fmin in case process was suspended, or last frame took very long
    t_debt += delta_time;

    if (am_conf_fixed_delta_time > 0.0) {
        while (t_debt > 0.0) {
            if (!am_execute_actions(ios_L, am_conf_fixed_delta_time)) {
                ios_running = false;
                return;
            }
            t_debt -= am_conf_fixed_delta_time;
        }
    } else {
        if (t_debt > MIN_UPDATE_TIME) {
            if (!am_execute_actions(ios_L, t_debt)) {
                ios_running = false;
                return;
            }
            t_debt = 0.0;
        }
    }

    t0 = frame_time;
}

static void ios_garbage_collect() {
    lua_gc(ios_L, LUA_GCCOLLECT, 0);
    lua_gc(ios_L, LUA_GCCOLLECT, 0);
}


static void ios_touch_began(NSSet *touches) {
    if (ios_L == NULL) return;
    NSEnumerator *e = [touches objectEnumerator];
    UITouch *touch;
    while ((touch = [e nextObject])) {
        CGPoint pos = [touch locationInView:touch.view];
        lua_pushlightuserdata(ios_L, touch);
        lua_pushnumber(ios_L, pos.x);
        lua_pushnumber(ios_L, pos.y);
        am_call_amulet(ios_L, "_touch_begin", 3, 0);
    }
}

static void ios_touch_moved(NSSet *touches) {
    if (ios_L == NULL) return;
    NSEnumerator *e = [touches objectEnumerator];
    UITouch *touch;
    while ((touch = [e nextObject])) {
        CGPoint pos = [touch locationInView:touch.view];
        lua_pushlightuserdata(ios_L, touch);
        lua_pushnumber(ios_L, pos.x);
        lua_pushnumber(ios_L, pos.y);
        am_call_amulet(ios_L, "_touch_move", 3, 0);
    }
}

static void ios_touch_ended(NSSet *touches) {
    if (ios_L == NULL) return;
    NSEnumerator *e = [touches objectEnumerator];
    UITouch *touch;
    while ((touch = [e nextObject])) {
        CGPoint pos = [touch locationInView:touch.view];
        lua_pushlightuserdata(ios_L, touch);
        lua_pushnumber(ios_L, pos.x);
        lua_pushnumber(ios_L, pos.y);
        am_call_amulet(ios_L, "_touch_end", 3, 0);
    }
}

static void ios_touch_cancelled(NSSet *touches) {
    ios_touch_ended(touches);
}

/*
static UIViewController *ios_get_view_controller() {
    return (UIViewController*)am_view_controller;
}
*/

static void ios_save_state() {
    // XXX ltSaveState();
    ios_sync_store();
}

static void ios_resign_active() {
    // XXX ltClientShutdown();
}

static void ios_become_active() {
    // XXX ltClientInit();
    frames_since_disable_animations = 0;
}





@interface AMView : UIView {    
@private
    EAGLContext *glContext;
    
    GLint backingWidth;
    GLint backingHeight;
    
    GLuint defaultFramebuffer, colorRenderbuffer, depthRenderbuffer;

    BOOL animating;
    NSInteger animationFrameInterval;
    id displayLink;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

- (void) startAnimation;
- (void) stopAnimation;
- (void) drawView:(id)sender;

@end

@implementation AMView

@synthesize animating;
@dynamic animationFrameInterval;

+ (Class) layerClass {
    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame {    
    if ((self = [super initWithFrame:frame])) {
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
        
        depthRenderbuffer = 0;

        glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
            
        if (!glContext || ![EAGLContext setCurrentContext:glContext]) {
            am_log0("%s", "failed to create glContext");
            return nil;
        } else {
            am_debug("%s", "successfully created glContext");
        }
            
        glGenFramebuffers(1, &defaultFramebuffer);
        am_default_framebuffer = defaultFramebuffer;
        glGenRenderbuffers(1, &colorRenderbuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
        
        animating = FALSE;
        animationFrameInterval = 1;
        displayLink = nil;

        // Needed for retina display.
        if([[UIScreen mainScreen] respondsToSelector: NSSelectorFromString(@"scale")]) {
            if([self respondsToSelector: NSSelectorFromString(@"contentScaleFactor")]) {
                self.contentScaleFactor = [[UIScreen mainScreen] scale];
            }
        }
        
        [self setMultipleTouchEnabled:YES]; 
    }
    
    return self;
}

- (void) drawView:(id)sender {
    //fprintf(stderr, "render\n");
    if (!init_run) {
        ios_init();
        init_run = true;
    }

    ios_render();
    
    /*
    glEnableClientState(GL_VERTEX_ARRAY);
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    glViewport(0, 0, backingWidth, backingHeight);
    glOrthof(0.0f, 6.0f, 0.0f, 9.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    GLfloat vertices[8];
    GLfloat x1 = 1.0f;
    GLfloat y1 = 1.0f;
    GLfloat x2 = 3.0f;
    GLfloat y2 = 3.0f;
    vertices[0] = x1;
    vertices[1] = y1;
    vertices[2] = x2;
    vertices[3] = y1;
    vertices[4] = x2;
    vertices[5] = y2;
    vertices[6] = x1;
    vertices[7] = y2;
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    */
    
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    [glContext presentRenderbuffer:GL_RENDERBUFFER];
}

- (void) layoutSubviews {
    am_debug("%s", "layoutSubviews");
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    [glContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)self.layer];
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
    
    if (depthRenderbuffer) {
        glDeleteRenderbuffers(1, &depthRenderbuffer);
    }
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, backingWidth, backingHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return;
    }
    
    ios_resize(backingWidth, backingHeight);
    
    [self drawView:nil];
}

- (NSInteger) animationFrameInterval {
    return animationFrameInterval;
}

- (void) setAnimationFrameInterval:(NSInteger)frameInterval {
    if (frameInterval >= 1) {
        animationFrameInterval = frameInterval;
        if (animating) {
            [self stopAnimation];
            [self startAnimation];
        }
    }
}

- (void) startAnimation {
    if (!animating) {
        displayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(drawView:)];
        [displayLink setFrameInterval:animationFrameInterval];
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        
        animating = TRUE;
    }
}

- (void)stopAnimation {
    if (animating) {
        [displayLink invalidate];
        displayLink = nil;
        
        animating = FALSE;
    }
}

- (void) dealloc {
    ios_teardown();
    
    // Tear down GL
    if (depthRenderbuffer) {
        glDeleteRenderbuffers(1, &depthRenderbuffer);
        depthRenderbuffer = 0;
    }
    
    if (defaultFramebuffer) {
        glDeleteFramebuffers(1, &defaultFramebuffer);
        defaultFramebuffer = 0;
    }
    
    if (colorRenderbuffer) {
        glDeleteRenderbuffers(1, &colorRenderbuffer);
        colorRenderbuffer = 0;
    }
    
    // Tear down context
    if ([EAGLContext currentContext] == glContext) {
        [EAGLContext setCurrentContext:nil];
    }
    
    [glContext release];
    glContext = nil;
    
    [super dealloc];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    ios_touch_began(touches);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{    
    ios_touch_moved(touches);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{    
    ios_touch_ended(touches);
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    ios_touch_cancelled(touches);
}

@end


static CMMotionManager *motionManager = nil;


@interface AMViewController : UIViewController { }
@end

static UIInterfaceOrientation last_orientation = UIInterfaceOrientationPortrait;
static bool has_rotated = false;

static BOOL handle_orientation(UIInterfaceOrientation orientation) {
    BOOL res = NO;
    if (has_rotated && motionManager != nil && frames_since_disable_animations > 60) {
        // Prevent screen rotation if using motion input.
        return orientation == last_orientation;
    }
    switch (ios_orientation) {
        case AM_DISPLAY_ORIENTATION_PORTRAIT:
            res = (orientation == UIInterfaceOrientationPortrait
                    || orientation == UIInterfaceOrientationPortraitUpsideDown);
            break;
        case AM_DISPLAY_ORIENTATION_LANDSCAPE:
            res = (orientation == UIInterfaceOrientationLandscapeRight
                    || orientation == UIInterfaceOrientationLandscapeLeft);
            break;
        case AM_DISPLAY_ORIENTATION_ANY:
            res = (orientation == UIInterfaceOrientationLandscapeRight
                    || orientation == UIInterfaceOrientationLandscapeLeft
                    || orientation == UIInterfaceOrientationPortrait
                    || orientation == UIInterfaceOrientationPortraitUpsideDown);
            break;
    }
    if (res == YES) {
        has_rotated = true;
        last_orientation = orientation;
    }
    return res;
}

@implementation AMViewController

- (void)loadView {
    [super loadView];
    // Turn off orientation change animations initially
    // to avoid an orientation change animation after loading.
    [UIView setAnimationsEnabled:NO];

    AudioSessionInitialize(NULL, NULL, audio_interrupt, NULL);
    set_audio_category();
    AudioSessionSetActive(true);



    CGRect screen_bounds = [[UIScreen mainScreen] bounds];
    CGFloat screen_w = screen_bounds.size.width;
    CGFloat screen_h = screen_bounds.size.height;
    switch (ios_orientation) {
        case AM_DISPLAY_ORIENTATION_PORTRAIT:
            if (screen_w > screen_h) {
                CGFloat tmp = screen_w;
                screen_w = screen_h;
                screen_h = tmp;
            }
            break;
        case AM_DISPLAY_ORIENTATION_LANDSCAPE:
            if (screen_w < screen_h) {
                CGFloat tmp = screen_w;
                screen_w = screen_h;
                screen_h = tmp;
            }
            break;
        case AM_DISPLAY_ORIENTATION_ANY:
            break;
    }
    CGRect frame = CGRectMake(0, 0, screen_w, screen_h);
    am_view = [[[AMView alloc] initWithFrame:frame] autorelease];
    [[self view] addSubview: am_view];
}

- (void)dealloc {
    [super dealloc];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    ios_garbage_collect();
}

- (void)viewDidLoad
{
}

- (void)viewDidUnload
{
    [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
    return handle_orientation(orientation);
}

- (BOOL)shouldAutorotate {
    /*
    UIInterfaceOrientation orientation = [[UIDevice currentDevice] orientation];
    return handle_orientation(orientation);
    */
    return YES;
}

-(NSUInteger)supportedInterfaceOrientations
{
    // XXX
    return UIInterfaceOrientationMaskLandscapeRight | UIInterfaceOrientationMaskLandscapeLeft;
}

-(BOOL)prefersStatusBarHidden
{
    // XXX
    return YES;
}

@end


static UIWindow *window;
static AMViewController *viewController;

@interface AMAppDelegate : NSObject <UIApplicationDelegate> { }
@end

@implementation AMAppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
    [window retain];
    viewController = [[[AMViewController alloc] init] autorelease];
    [viewController retain];
    [window addSubview:viewController.view];
    window.rootViewController = viewController;
    [window makeKeyAndVisible];
    set_view_controller(viewController);
    return YES;
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    ios_save_state();
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    ios_resign_active();
    [am_view stopAnimation];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    ios_become_active();
    [am_view startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
}

- (void)dealloc
{
    [super dealloc];
    if (motionManager != nil) {
        [motionManager release];
    }
}

@end

void ios_sample_accelerometer(double *x, double *y, double *z) {
    *x = 0.0;
    *y = 0.0;
    *z = 0.0;
    if (motionManager == nil) {
        motionManager = [[CMMotionManager alloc] init];
        [[UIApplication sharedApplication] setIdleTimerDisabled: YES];
    }
    if (!motionManager.accelerometerAvailable) return;
    if (!motionManager.accelerometerActive) {
        [motionManager startAccelerometerUpdates];
    }
    CMAccelerometerData *data = motionManager.accelerometerData;
    if (data == nil) return;
    CMAcceleration accel = data.acceleration;

    *z = accel.z;

    if (has_rotated) {
        switch (last_orientation) {
            case UIInterfaceOrientationPortrait:
                *x = accel.x;
                *y = accel.y;
                break;
            case UIInterfaceOrientationPortraitUpsideDown:
                *x = -accel.x;
                *y = -accel.y;
                break;
            case UIInterfaceOrientationLandscapeLeft:
                *x = accel.y;
                *y = -accel.x;
                break;
            case UIInterfaceOrientationLandscapeRight:
                *x = -accel.y;
                *y = accel.x;
                break;
        }
    } else {
        *x = accel.x;
        *y = accel.y;
    }
}

//-------------------------------------------------------------------------------


int main(int argc, char *argv[]) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    int retVal = UIApplicationMain(argc, argv, nil, @"AMAppDelegate");
    [pool release];
    return retVal;
}

int native_window = 1;

am_native_window *am_create_native_window(
    am_window_mode mode,
    am_display_orientation orientation,
    int top, int left,
    int width, int height,
    const char *title,
    bool resizable,
    bool borderless,
    bool depth_buffer,
    bool stencil_buffer,
    int msaa_samples)
{
    if (ios_window_created) {
        return NULL;
    }
    if (!am_gl_is_initialized()) {
        am_init_gl();
    }
    ios_window_created = true;
    ios_orientation = orientation;
    return (am_native_window*)&native_window;
}

void am_get_native_window_size(am_native_window *window, int *w, int *h) {
    *w = screen_width;
    *h = screen_height;
}

bool am_set_native_window_size_and_mode(am_native_window *window, int w, int h, am_window_mode mode) {
    return false;
}

void am_set_native_window_lock_pointer(am_native_window *window, bool lock) {
}

void am_destroy_native_window(am_native_window *window) {
}

void am_native_window_pre_render(am_native_window *window) {
}

void am_native_window_post_render(am_native_window *window) {
}

double am_get_current_time() {
    uint64_t t = mach_absolute_time();
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    return (double)t * (double)timebase.numer / (double)timebase.denom / 1e9;
}



void *am_read_resource(const char *filename, int *len, char** errmsg) {
    *errmsg = NULL;
    return am_read_package_resource(package, filename, len, errmsg);
}




#endif // AM_BACKEND_IOS
