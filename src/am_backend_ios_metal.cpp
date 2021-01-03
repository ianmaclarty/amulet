#include "amulet.h"

#if defined(AM_BACKEND_IOS) && defined(AM_USE_METAL)

#import <objc/runtime.h>
#import <QuartzCore/QuartzCore.h>

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioServices.h>
#import <AudioUnit/AudioUnit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMotion/CoreMotion.h>
#import <MetalKit/MetalKit.h>
#import <GameKit/GameKit.h>
#import <StoreKit/StoreKit.h>
#import <WebKit/WebKit.h>

#import <Metal/Metal.h>

#include <mach/mach_time.h>

//#define AM_IOS_DEBUG

extern bool am_metal_use_highdpi;
extern bool am_metal_window_depth_buffer;
extern bool am_metal_window_stencil_buffer;
extern int am_metal_window_msaa_samples;
extern int am_metal_window_pwidth;
extern int am_metal_window_pheight;
extern int am_metal_window_swidth;
extern int am_metal_window_sheight;

static am_engine *ios_eng = NULL;
static am_package *package = NULL;
static am_display_orientation ios_orientation = AM_DISPLAY_ORIENTATION_ANY;
static bool ios_window_created = false;
static bool ios_running = false;
static bool ios_paused = false;
static bool ios_audio_initialized = false;
static bool ios_force_touch_recognised = false;
static bool force_touch_is_available();
static float *ios_audio_buffer = NULL;
static int ios_audio_offset = 0;
static NSLock *ios_audio_mutex = [[NSLock alloc] init];
static bool ios_done_first_draw = false;
#ifdef AM_GOOGLE_ADS
#import <GoogleMobileAds/GADBannerView.h>
static GADBannerView *banner_ad = nil;
static bool banner_ad_filled = false;
static bool banner_ad_want_visible = false;
static bool banner_ad_is_visible = false;
static id banner_delegate = nil;
#endif

static MTKView *ios_view = nil;
static UIViewController *ios_view_controller = nil;

static double t0;
static double t_debt;
static double frame_time = 0.0;
static double delta_time;
static double real_delta_time;

static bool ios_performing_restart = false;
static bool ios_was_icloud_update = false;

extern MTKView *am_metal_ios_view;

//static void init_gamecenter();

#define MIN_UPDATE_TIME (1.0/400.0)

static void ios_sync_store();

//---------------------------------------------------------------------------

static bool ios_is_ipad() {
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
        return true;
    } else {
        return false;
    }
}

/*
static float ios_scaling() {
    float scale = 1.0f;
    if([[UIScreen mainScreen] respondsToSelector: NSSelectorFromString(@"scale")]) {
        scale = [[UIScreen mainScreen] scale];
    }
    return scale;
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
static NSUbiquitousKeyValueStore *cloudstore = nil;

static void ensure_prefs_initialized() {
    if (prefs == nil) {
        prefs = [NSUserDefaults standardUserDefaults];
    }
    if (cloudstore == nil) {
        cloudstore = [NSUbiquitousKeyValueStore defaultStore];
    }
}

static void ios_store_pref_data(const char *key, const void *val, size_t len) {
    ensure_prefs_initialized();
    long long t = (long long)time(NULL);
    NSNumber *ts = [NSNumber numberWithLongLong:t];
    NSString *skey = [NSString stringWithUTF8String:key];
    NSString *skey_ts = [skey stringByAppendingString:@"_timestamp"];
    NSData *dval = [NSData dataWithBytes:val length:len];
    NSObject *existingObj = [prefs objectForKey:skey];
    if (existingObj != nil && [existingObj isKindOfClass:[NSData class]]) {
        NSData *existingData = (NSData*)existingObj;
        size_t existingLen = [existingData length];
        if ((existingLen == len) && (memcmp([existingData bytes], val, len) == 0)) {
            // data is unchanged, so avoid updating the timestamp
            #ifdef AM_IOS_DEBUG
            am_debug("%s", "not saving because data unchanged");
            #endif
            return;
        }
    }
    [prefs setObject:dval forKey:skey];
    [prefs setObject:ts forKey:skey_ts];
    if (cloudstore != nil) {
        #ifdef AM_IOS_DEBUG
        am_debug("storing value in cloud (%lld)", t);
        #endif
        [cloudstore setObject:dval forKey:skey];
        [cloudstore setObject:ts forKey:skey_ts];
    } else {
        #ifdef AM_IOS_DEBUG
        am_debug("%s", "icloud not available");
        #endif
    }
}

static int ios_store_pref(lua_State *L) {
    am_check_nargs(L, 2);
    const char *key = lua_tostring(L, 1);
    if (key == NULL) return luaL_error(L, "expecting a string in position 1");
    int valtype = am_get_type(L, 2);
    size_t len;
    const void *val;
    switch (valtype) {
        case MT_am_buffer: {
            am_buffer *buf = am_get_userdata(L, am_buffer, 2);
            val = (const void*)buf->data;
            len = (size_t)buf->size;
            break;
        }
        default: {
            val = (const void*)lua_tolstring(L, 2, &len);
            break;
        }
    }
    if (val == NULL) return luaL_error(L, "expecting a string or non-empty buffer in position 2");
    ios_store_pref_data(key, val, len);
    return 0;
}

static void* ios_retrieve_pref_data(const char *key, size_t *len) {
    *len = 0;
    ensure_prefs_initialized();

    NSString *skey = [NSString stringWithUTF8String:key];
    NSString *skey_ts = [skey stringByAppendingString:@"_timestamp"];
    NSObject *local_tsobj = [prefs objectForKey:skey_ts];
    NSObject *cloud_tsobj = nil;

    bool use_local = true;
    if (cloudstore != nil) {
        cloud_tsobj = [cloudstore objectForKey:skey_ts];
    }
    if (local_tsobj != nil && cloud_tsobj != nil) {
        // value is in cloud and local so compare timestamps.
        long long local_ts = [((NSNumber*)local_tsobj) longLongValue];
        long long cloud_ts = [((NSNumber*)cloud_tsobj) longLongValue];
        #ifdef AM_IOS_DEBUG
        am_debug("value in cloud and local (%lld vs %lld)", cloud_ts, local_ts);
        #endif
        use_local = local_ts >= cloud_ts;
    } else if (local_tsobj == nil && cloud_tsobj != nil) {
        // value is in cloud, but not local OR value is in local, but from an older version of amulet with no timestamps.
        // in either case we want to use the cloud version.
        use_local = false;
        #ifdef AM_IOS_DEBUG
        am_debug("%s", "value in cloud only");
        #endif
    } else {
        #ifdef AM_IOS_DEBUG
        am_debug("%s", "value in local only (or neither)");
        #endif
    }

    NSObject *obj = nil;
    if (use_local) {
        obj = [prefs objectForKey:skey];
    } else {
        obj = [cloudstore objectForKey:skey];
    }
    if (obj == nil) return NULL;
    if ([obj isKindOfClass:[NSData class]]) {
        NSData *data = (NSData*)obj;
        *len = [data length];
        void* val = malloc(*len);
        memcpy(val, [data bytes], *len);
        return val;
    } else if ([obj isKindOfClass:[NSString class]]) {
        // check for NSString for backwards compatibility
        const char *str = [((NSString*)obj) UTF8String];
        *len = strlen(str);
        void* val = malloc(*len);
        memcpy(val, str, *len);
        return val;
    } else {
        return NULL;
    }
}

static int ios_retrieve_pref(lua_State *L) {
    am_check_nargs(L, 1);
    const char *key = lua_tostring(L, 1);
    if (key == NULL) return luaL_error(L, "expecting a string in position 1");
    size_t len;
    void *val = ios_retrieve_pref_data(key, &len);
    if (val == NULL) {
        lua_pushnil(L);
    } else {
        lua_pushlstring(L, (char*)val, len);
        free(val);
    }
    return 1;
}

static int force_touch_available(lua_State *L) {
    lua_pushboolean(L, force_touch_is_available() ? 1 : 0);
    return 1;
}

static void ios_sync_store() {
    ensure_prefs_initialized();
    if (prefs != nil) {
        [prefs synchronize];
    }
    if (cloudstore != nil) {
        [cloudstore synchronize];
    }
}

// ---------------------------------------------------------------------------


static void set_audio_category() {
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:nil];
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

static OSStatus
ios_audio_callback(void *inRefCon,
               AudioUnitRenderActionFlags * ioActionFlags,
               const AudioTimeStamp * inTimeStamp,
               UInt32 inBusNumber, UInt32 inNumberFrames,
               AudioBufferList * ioData)
{
    if (!ios_audio_initialized || ios_paused) {
        for (int i = 0; i < ioData->mNumberBuffers; i++) {
            AudioBuffer *abuf = &ioData->mBuffers[i];
            // silence.
            memset(abuf->mData, 0, abuf->mDataByteSize);
        }
        return noErr;
    }

    int num_channels = am_conf_audio_channels;
    int num_samples = am_conf_audio_buffer_size;
    for (int i = 0; i < ioData->mNumberBuffers; i++) {
        AudioBuffer *abuf = &ioData->mBuffers[i];
        int remaining = abuf->mDataByteSize / (sizeof(float) * num_channels);
        float *ptr = (float*)abuf->mData;
        while (remaining > 0) {
            if (ios_audio_offset >= num_samples) {
                // Generate the data
                memset(ios_audio_buffer, 0, num_samples * num_channels * sizeof(float));
                am_audio_bus bus(num_channels, num_samples, ios_audio_buffer);
                [ios_audio_mutex lock];
                am_fill_audio_bus(&bus);
                [ios_audio_mutex unlock];
                ios_audio_offset = 0;
            }

            int len = num_samples - ios_audio_offset;
            if (len > remaining) {
                len = remaining;
            }
            // CoreAudio expects channels to be interleaved
            am_interleave_audio(ptr, ios_audio_buffer,
                num_channels, num_samples, ios_audio_offset, len);
            //memcpy(ptr, (char *)ios_audio_buffer_interleaved + ios_audio_offset, len);
            /*
            for (int k = 0; k < len/4; k++) {
                fprintf(stderr, "ptr[%d] = %f", k, ((float*)ptr)[k]);
            }
            */
            ptr = ptr + len * num_channels;
            remaining -= len;
            ios_audio_offset += len;
        }
    }

    return noErr;
}

static void ios_init_audio() {
    AURenderCallbackStruct callback;
    AudioStreamBasicDescription strdesc;
    AudioComponentDescription desc;
    AudioComponent comp = NULL;
    AudioComponentInstance instance;
    const AudioUnitElement output_bus = 0;
    const AudioUnitElement bus = output_bus;
    const AudioUnitScope scope = kAudioUnitScope_Input;

    set_audio_category();
    if (![[AVAudioSession sharedInstance] setActive:YES error:nil]) {
        am_log0("%s", "WARNING: unable to activate AVAudioSession");
        return;
    }

    memset(&desc, 0, sizeof(AudioComponentDescription));
    desc.componentType = kAudioUnitType_Output;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    comp = AudioComponentFindNext(NULL, &desc);
    if (comp == NULL) {
        am_log0("%s", "WARNING: unable to find suitable audio component");
        return;
    }

    if (noErr != AudioComponentInstanceNew(comp, &instance)) {
        am_log0("%s", "WARNING: failed to create audio instance");
        return;
    }

    // set format
    memset(&strdesc, 0, sizeof(AudioStreamBasicDescription));
    strdesc.mFormatID = kAudioFormatLinearPCM;
    strdesc.mFormatFlags = kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsFloat;
    strdesc.mChannelsPerFrame = am_conf_audio_channels;
    strdesc.mSampleRate = am_conf_audio_sample_rate;
    strdesc.mFramesPerPacket = 1;
    strdesc.mBitsPerChannel = 8 * sizeof(float);
    strdesc.mBytesPerFrame =
        strdesc.mBitsPerChannel * strdesc.mChannelsPerFrame / 8;
    strdesc.mBytesPerPacket =
        strdesc.mBytesPerFrame * strdesc.mFramesPerPacket;

    if (noErr != 
        AudioUnitSetProperty(instance, kAudioUnitProperty_StreamFormat,
            scope, bus, &strdesc, sizeof(AudioStreamBasicDescription)))
    {
        am_log0("%s", "WARNING: unable to set audio format");
        return;
    }

    // set callback
    memset(&callback, 0, sizeof(AURenderCallbackStruct));
    callback.inputProc = ios_audio_callback;
    callback.inputProcRefCon = NULL;
    if (noErr != 
        AudioUnitSetProperty(instance, kAudioUnitProperty_SetRenderCallback,
            scope, bus, &callback, sizeof(AURenderCallbackStruct)))
    {
        am_log0("%s", "WARNING: unable to set audio callback");
        return;
    }

    // create buffer
    assert(ios_audio_buffer == NULL);
    ios_audio_buffer = (float*)malloc(am_conf_audio_channels * am_conf_audio_buffer_size * sizeof(float));
    ios_audio_offset = am_conf_audio_buffer_size; // so it gets filled straight away

    ios_audio_initialized = true;

    // start
    if (noErr != AudioUnitInitialize(instance)) {
        am_log0("%s", "WARNING: unable to initialize audio unit");
        return;
    }

    if (noErr != AudioOutputUnitStart(instance)) {
        am_log0("%s", "WARNING: unable to start audio unit");
        return;
    }
}

static void ios_init_engine() {
    ios_running = false;
    ios_paused = false;

    am_opt_data_dir = ios_bundle_path();
    am_opt_main_module = "main";
    if (!ios_performing_restart) {
        if (!open_package()) return;
        if (!am_load_config()) return;
    }
    ios_eng = am_init_engine(false, 0, NULL);
    if (ios_eng == NULL) return;

    frame_time = am_get_current_time();
    lua_pushcclosure(ios_eng->L, am_require, 0);
    lua_pushstring(ios_eng->L, am_opt_main_module);
    if (am_call(ios_eng->L, 1, 0)) {
        ios_running = true;
    }
    if (!ios_performing_restart) {
        am_gl_end_framebuffer_render();
        am_gl_end_frame(false);
    }
    t0 = am_get_current_time();
    frame_time = t0;
    t_debt = 0.0;
}

static void ios_teardown() {
    ios_running = false;
    if (ios_eng->L != NULL) {
        am_destroy_engine(ios_eng);
        ios_eng = NULL;
    }
    if (!ios_performing_restart) {
        if (am_gl_is_initialized()) {
            am_destroy_gl();
        }
        if (package != NULL) {
            am_close_package(package);
        }
    }
}

static void ios_restart() {
    #ifdef AM_IOS_DEBUG
    am_debug("%s", "restarting...");
    #endif
    ios_performing_restart = true;
    ios_teardown();
    ios_init_engine();
    ios_performing_restart = false;
}

// This is used to decide whether rotations should
// be animated (we don't want to animate the initial
// rotation after app launch), as well as whether to
// disable rotations altogether when the app is using
// the accelerometer for input (we want to however
// allow the initial rotation).
static int frames_since_disable_animations = 0;

static void ios_draw() {
    if (ios_eng == NULL) return;
    am_update_windows(ios_eng->L);
    //am_debug("%s", "done drawing");
}

static void ios_update() {
    if (frames_since_disable_animations == 60) {
        [UIView setAnimationsEnabled:YES];
    }
    frames_since_disable_animations++;
    if (!ios_running) return;
    if (ios_paused) return;

    [ios_audio_mutex lock];
    am_sync_audio_graph(ios_eng->L);
    [ios_audio_mutex unlock];

    frame_time = am_get_current_time();
    
    real_delta_time = frame_time - t0;
    // take min in case app paused, or last frame took very long
    delta_time = am_min(am_conf_max_delta_time, real_delta_time); 
    t_debt += delta_time;

    if (am_conf_fixed_delta_time > 0.0) {
        while (t_debt > 0.0) {
            if (!am_execute_actions(ios_eng->L, am_conf_fixed_delta_time)) {
                ios_running = false;
                return;
            }
            t_debt -= am_conf_fixed_delta_time;
        }
    } else {
        if (t_debt > am_conf_min_delta_time) {
            if (!am_execute_actions(ios_eng->L, t_debt)) {
                ios_running = false;
                return;
            }
            t_debt = 0.0;
        }
    }

    t0 = frame_time;
}

static void ios_garbage_collect() {
    if (ios_eng != NULL && ios_eng->L != NULL) {
        lua_gc(ios_eng->L, LUA_GCCOLLECT, 0);
        lua_gc(ios_eng->L, LUA_GCCOLLECT, 0);
    }
}

static bool force_touch_is_available() {
    //am_debug("ios_force_touch_recognised = %s", ios_force_touch_recognised ? "true" : "false");
    //const char *str = "???";
    //switch ([[ios_view traitCollection] forceTouchCapability]) {
    //    case UIForceTouchCapabilityAvailable: str = "available"; break;
    //    case UIForceTouchCapabilityUnavailable: str = "unavailable"; break;
    //    case UIForceTouchCapabilityUnknown: str = "unknown"; break;
    //}
    //am_debug("forceTouchCapability = %s", str);
    return (ios_force_touch_recognised && ios_view != nil &&
        [[ios_view traitCollection] forceTouchCapability] == UIForceTouchCapabilityAvailable);
}

static void ios_touch_began(NSSet *touches) {
    if (ios_eng->L == NULL) return;
    NSEnumerator *e = [touches objectEnumerator];
    UITouch *touch;
    while ((touch = [e nextObject])) {
        double force = force_touch_is_available() ? [touch force] : 1.0;
        CGPoint pos = [touch locationInView:touch.view];
        //am_debug("pos = %f %f", pos.x, pos.y);
        am_find_window((am_native_window*)ios_view)->touch_begin(ios_eng->L, touch, pos.x, pos.y, force);
    }
}

static void ios_touch_moved(NSSet *touches) {
    if (ios_eng->L == NULL) return;
    NSEnumerator *e = [touches objectEnumerator];
    UITouch *touch;
    while ((touch = [e nextObject])) {
        double force = force_touch_is_available() ? [touch force] : 1.0;
        CGPoint pos = [touch locationInView:touch.view];
        am_find_window((am_native_window*)ios_view)->touch_move(ios_eng->L, touch, pos.x, pos.y, force);
    }
}

static void ios_touch_ended(NSSet *touches) {
    if (ios_eng->L == NULL) return;
    NSEnumerator *e = [touches objectEnumerator];
    UITouch *touch;
    while ((touch = [e nextObject])) {
        double force = force_touch_is_available() ? [touch force] : 0.0;
        CGPoint pos = [touch locationInView:touch.view];
        am_find_window((am_native_window*)ios_view)->touch_end(ios_eng->L, touch, pos.x, pos.y, force);
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

static void ios_entered_background() {
    ios_sync_store();
}

static void ios_resign_active() {
    ios_paused = true;
}

static void ios_become_active() {
    ios_sync_store();
    frames_since_disable_animations = 0;
    if (ios_was_icloud_update) {
        ios_was_icloud_update = false;
        if (ios_running) {
            ios_restart(); // restart the app, so we load new save data
        }
    } else {
        if (ios_view != nil && ios_eng != NULL) {
            // reset event data in case touch end events missing
            am_find_window((am_native_window*)ios_view)->push(ios_eng->L);
            am_call_amulet(ios_eng->L, "_reset_window_event_data", 1, 0);

            am_find_window((am_native_window*)ios_view)->push(ios_eng->L);
            am_call_amulet(ios_eng->L, "_became_active", 0, 0);
        }
    }
    ios_paused = false;
}

#ifdef AM_GOOGLE_ADS
static void hide_banner() {
    banner_ad_is_visible = false;
    [UIView animateWithDuration:0.5
        animations:^ { banner_ad.frame = CGRectMake(0.0, -banner_ad.frame.size.height,
                                  banner_ad.frame.size.width,
                                  banner_ad.frame.size.height);
        }
        completion:^(BOOL finished){
            banner_ad.hidden = YES;
        }];
}

static void show_banner() {
    /*
    if (banner_ad.frame.origin.y < 0.0) {
        banner_ad.frame = CGRectMake(0.0, -banner_ad.frame.size.height,
                                      banner_ad.frame.size.width,
                                      banner_ad.frame.size.height);
    }
    */
    banner_ad.hidden = NO;
    [UIView animateWithDuration:0.5
        animations:^ {banner_ad.frame = CGRectMake(0.0, 0.0,
                              banner_ad.frame.size.width,
                              banner_ad.frame.size.height);
        }
        completion:^(BOOL finished){
            banner_ad_is_visible = true;
        }];
}
#endif

static CMMotionManager *motionManager = nil;

@interface AMViewController : UIViewController { }
@end

static UIInterfaceOrientation last_orientation = UIInterfaceOrientationLandscapeLeft;
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
        case AM_DISPLAY_ORIENTATION_HYBRID:
            if (ios_is_ipad()) {
                res = (orientation == UIInterfaceOrientationLandscapeRight
                        || orientation == UIInterfaceOrientationLandscapeLeft);
            } else {
                res = (orientation == UIInterfaceOrientationPortrait
                        || orientation == UIInterfaceOrientationPortraitUpsideDown);
            }
            break;
    }
    if (res == YES) {
        has_rotated = true;
        last_orientation = orientation;
    }
    return res;
}

@implementation AMViewController

- (void)dealloc {
    [super dealloc];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    ios_garbage_collect();
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
    //am_debug("%s", "shouldAutorotateToInterfaceOrientation");
    return handle_orientation(orientation);
}

-(NSUInteger)supportedInterfaceOrientations
{
    //am_debug("%s", "supportedInterfaceOrientations");
    switch (ios_orientation) {
        case AM_DISPLAY_ORIENTATION_PORTRAIT: 
            return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
        case AM_DISPLAY_ORIENTATION_LANDSCAPE:
            return UIInterfaceOrientationMaskLandscapeRight | UIInterfaceOrientationMaskLandscapeLeft;
        case AM_DISPLAY_ORIENTATION_ANY:
            return UIInterfaceOrientationMaskLandscapeRight | UIInterfaceOrientationMaskLandscapeLeft
            | UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
        case AM_DISPLAY_ORIENTATION_HYBRID:
            if (ios_is_ipad()) {
                return UIInterfaceOrientationMaskLandscapeRight | UIInterfaceOrientationMaskLandscapeLeft;
            } else {
                return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
            }
    }
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
    //am_debug("%s", "preferredInterfaceOrientationForPresentation");
    switch (ios_orientation) {
        case AM_DISPLAY_ORIENTATION_PORTRAIT:
            return UIInterfaceOrientationPortrait;
        case AM_DISPLAY_ORIENTATION_LANDSCAPE:
            return UIInterfaceOrientationLandscapeLeft;
        case AM_DISPLAY_ORIENTATION_ANY:
            return UIInterfaceOrientationPortrait;
        case AM_DISPLAY_ORIENTATION_HYBRID:
            if (ios_is_ipad()) {
                return UIInterfaceOrientationLandscapeLeft;
            } else {
                return UIInterfaceOrientationPortrait;
            }
    }
}

-(BOOL)prefersStatusBarHidden
{
    return YES;
}

// If this returns YES, then preferredScreenEdgesDeferringSystemGestures below does
// not apply to the task switcher bar at the bottom of the screen. The auto-hide is pretty
// useless anyway, since any tap seems to cause it to display. At least with deferred gestures
// it remains half faded out.
//-(BOOL)prefersHomeIndicatorAutoHidden
//{
//    return YES;
//}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures
{
    return UIRectEdgeAll;
}

@end


@interface AMAppDelegate : UIResponder <UIApplicationDelegate, SKPaymentTransactionObserver, MTKViewDelegate, WKNavigationDelegate
#ifdef AM_GOOGLE_ADS
    , GADAdDelegate
#endif
>

@property (strong, nonatomic) UIWindow *window;

@end

extern void am_ios_custom_init(UIViewController *view_controller);

@implementation AMAppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    #ifdef AM_IOS_DEBUG
    am_debug("%s", "didFinishLaunchingWithOptions");
    #endif
    application.delegate = self;

    ios_sync_store();

    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(updateFromiCloud:)
        name:NSUbiquitousKeyValueStoreDidChangeExternallyNotification
        object:nil];

    // Prevent rotate animation when application launches.
    [UIView setAnimationsEnabled:NO];

#ifdef AM_GOOGLE_ADS
    banner_delegate = self;
#endif

    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
 
    MTKView *view = [[MTKView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    view.device = MTLCreateSystemDefaultDevice();
    view.delegate = self;
    view.preferredFramesPerSecond = 60;
    [view setMultipleTouchEnabled:YES]; 
    ios_view = view;
    am_metal_ios_view = ios_view;

    if ([ios_view respondsToSelector: NSSelectorFromString(@"traitCollection")] &&
        [[ios_view traitCollection] respondsToSelector: NSSelectorFromString(@"forceTouchCapability")])
    {
        ios_force_touch_recognised = true;
    }

    AMViewController * viewController = [[AMViewController alloc] initWithNibName:nil bundle:nil];
    viewController.view = view;
    self.window.rootViewController = viewController;
    ios_view_controller = viewController;

    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];

    [self.window makeKeyAndVisible];

    ios_init_engine();
    ios_init_audio();
 
    am_ios_custom_init(viewController);

    return YES;
}

-(void)updateFromiCloud:(NSNotification*) notificationObject
{
    #ifdef AM_IOS_DEBUG
    am_debug("%s", "updateFromiCloud");
    #endif
    ios_was_icloud_update = true;
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
}

- (void)drawInMTKView:(MTKView *)view {
    if (ios_view == nil) return;
    am_metal_window_pwidth = ios_view.drawableSize.width;
    am_metal_window_pheight = ios_view.drawableSize.height;
    float scale = am_metal_ios_view.contentScaleFactor;
    am_metal_window_swidth = (int)((float)am_metal_window_pwidth / scale);
    am_metal_window_sheight = (int)((float)am_metal_window_pheight / scale);

    @autoreleasepool {
        //am_debug("%s", "draw"); 
        if (ios_done_first_draw) {
            // make sure we do a draw before our first update
            ios_update();
        }
        ios_draw();
        am_gl_end_frame(true);
        ios_done_first_draw = true;
    }
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    #ifdef AM_IOS_DEBUG
    am_debug("%s", "applicationDidEnterBackground");
    #endif
    ios_entered_background();
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    #ifdef AM_IOS_DEBUG
    am_debug("%s", "applicationWillEnterForeground");
    #endif
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    #ifdef AM_IOS_DEBUG
    am_debug("%s", "applicationWillResignActive");
    #endif
    ios_resign_active();
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    #ifdef AM_IOS_DEBUG
    am_debug("%s", "applicationDidBecomeActive");
    #endif
    ios_become_active();
}

- (void)applicationWillTerminate:(UIApplication *)application
{
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (ios_view_controller != nil && ios_view_controller.presentedViewController == nil) {
        ios_touch_began(touches);
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{    
    if (ios_view_controller != nil && ios_view_controller.presentedViewController == nil) {
        ios_touch_moved(touches);
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{    
    if (ios_view_controller != nil && ios_view_controller.presentedViewController == nil) {
        ios_touch_ended(touches);
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (ios_view_controller != nil && ios_view_controller.presentedViewController == nil) {
        ios_touch_cancelled(touches);
    }
}

- (void)paymentQueue:(SKPaymentQueue *)queue
 updatedTransactions:(NSArray *)transactions
{
    // need to run on main thread, because it will access lua engine.
    [self performSelectorOnMainThread:@selector(updateTransactionsMain:) withObject:transactions waitUntilDone:YES];
}

- (void)updateTransactionsMain:(NSArray *)transactions;
{
    if (ios_eng != NULL && ios_eng->L != NULL) {
        lua_State *L = ios_eng->L;
        for (SKPaymentTransaction *transaction in transactions) {
            const char *status = "unknown";
            switch (transaction.transactionState) {
                case SKPaymentTransactionStatePurchasing:
                    status = "purchasing";
                    break;
                case SKPaymentTransactionStateDeferred:
                    status = "deferred";
                    break;
                case SKPaymentTransactionStateFailed:
                    if (transaction.error.code == SKErrorPaymentCancelled) {
                        status = "cancelled";
                    } else {
                        status = "failed";
                    }
                    break;
                case SKPaymentTransactionStatePurchased:
                    status = "purchased";
                    break;
                case SKPaymentTransactionStateRestored:
                    status = "restored";
                    break;
            }
            lua_pushstring(L, [transaction.payment.productIdentifier UTF8String]);
            lua_pushstring(L, status);
            am_call_amulet(L, "_iap_transaction_updated", 2, 0);
            switch (transaction.transactionState) {
                case SKPaymentTransactionStateFailed:
                case SKPaymentTransactionStatePurchased:
                case SKPaymentTransactionStateRestored:
                    [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
                    break;
            }
        }
    }
}

- (void)paymentQueue:(SKPaymentQueue *)queue 
restoreCompletedTransactionsFailedWithError:(NSError *)error
{
    [self performSelectorOnMainThread:@selector(restoreCompletedTransactionsFailedWithErrorMain) withObject:nil waitUntilDone:YES];
}

- (void)restoreCompletedTransactionsFailedWithErrorMain
{
    if (ios_eng != NULL && ios_eng->L != NULL) {
        lua_State *L = ios_eng->L;
        lua_pushboolean(L, 0);
        am_call_amulet(L, "_iap_restore_finished", 1, 0);
    }
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
    [self performSelectorOnMainThread:@selector(paymentQueueRestoreCompletedTransactionsFinishedMain) withObject:nil waitUntilDone:YES];
}

- (void)paymentQueueRestoreCompletedTransactionsFinishedMain
{
    if (ios_eng != NULL && ios_eng->L != NULL) {
        lua_State *L = ios_eng->L;
        lua_pushboolean(L, 1);
        am_call_amulet(L, "_iap_restore_finished", 1, 0);
    }
}

#ifdef AM_GOOGLE_ADS
- (void)adViewDidReceiveAd:(nonnull GADBannerView *)bannerView
{
    banner_ad_filled = true;
    if (banner_ad_want_visible) {
        show_banner();
    }
}

- (void)adView:(nonnull GADBannerView *)bannerView
    didFailToReceiveAdWithError:(nonnull GADRequestError *)error
{
    banner_ad_filled = false;
    //banner_ad.hidden = YES;
}
#endif

- (void)dealloc
{
    ios_teardown();
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

am_native_window *am_create_native_window(
    am_window_mode mode,
    am_display_orientation orientation,
    int top, int left,
    int width, int height,
    const char *title,
    bool highdpi,
    bool resizable,
    bool borderless,
    bool depth_buffer,
    bool stencil_buffer,
    int msaa_samples)
{
    if (ios_performing_restart) {
        return (am_native_window*)ios_view;
    }
    if (ios_window_created) {
        am_log0("%s", "attempt to create two iOS windows");
        return NULL;
    }
    ios_window_created = true;
    ios_orientation = orientation;
    if (ios_view == nil) {
        am_log0("%s", "ios_view not initialized!");
        return NULL;
    }

    am_metal_use_highdpi = highdpi;
    am_metal_window_depth_buffer = depth_buffer;
    am_metal_window_stencil_buffer = stencil_buffer;
    am_metal_window_msaa_samples = msaa_samples;

    if (depth_buffer && stencil_buffer) {
        ios_view.depthStencilPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    } else if (depth_buffer) {
        ios_view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    } else if (stencil_buffer) {
        ios_view.depthStencilPixelFormat = MTLPixelFormatStencil8;
    }
    if (msaa_samples > 1) {
        while (msaa_samples > 1 && ![ios_view.device supportsTextureSampleCount:msaa_samples]) {
            msaa_samples--;
        }
        ios_view.sampleCount = msaa_samples;
    }
    if (!am_gl_is_initialized()) {
        am_init_gl();
    }
    return (am_native_window*)ios_view;
}

void am_get_native_window_size(am_native_window *window, int *pw, int *ph, int *sw, int *sh) {
    *pw = am_metal_window_pwidth;
    *ph = am_metal_window_pheight;
    *sw = am_metal_window_swidth;
    *sh = am_metal_window_sheight;
}

void am_get_native_window_safe_area_margin(am_native_window *window, 
    int *left, int *right, int *bottom, int *top)
{
    if (ios_view != nil && ios_view.window != nil) {
        UIEdgeInsets safe_insets = ios_view.window.safeAreaInsets;
        *left = (int)safe_insets.left;
        *right = (int)safe_insets.right;
        *bottom = (int)safe_insets.bottom;
        *top = (int)safe_insets.top;
    } else {
        *left = 0;
        *right = 0;
        *bottom = 0;
        *top = 0;
    }
}

bool am_set_native_window_size_and_mode(am_native_window *window, int w, int h, am_window_mode mode) {
    return false;
}

bool am_get_native_window_lock_pointer(am_native_window *window) {
    return false;
}

void am_set_native_window_lock_pointer(am_native_window *window, bool lock) {
}

void am_set_native_window_show_cursor(am_native_window *window, bool show) {
}

bool am_get_native_window_show_cursor(am_native_window *window) {
    return false;
}

void am_destroy_native_window(am_native_window *window) {
}

void am_native_window_bind_framebuffer(am_native_window *window) {
    am_bind_framebuffer(0);
}

void am_native_window_swap_buffers(am_native_window *window) {
    // handled by iOS
}

double am_get_current_time() {
    uint64_t t = mach_absolute_time();
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    return (double)t * (double)timebase.numer / (double)timebase.denom / 1e9;
}

char *am_get_base_path() {
    return am_format("%s/", ios_bundle_path());
}

char *am_get_data_path() {
    NSArray *dirs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    const char *dir = [[dirs objectAtIndex:0] UTF8String];
    return am_format("%s/", dir);
}


void *am_read_resource(const char *filename, int *len, char** errmsg) {
    *errmsg = NULL;
    return am_read_package_resource(package, filename, len, errmsg);
}

void am_copy_video_frame_to_texture() {
}

void am_capture_audio(am_audio_bus *bus) {
}

int am_next_video_capture_frame() {
    return 0;
}

// ---------------- Game Center ---------------------

static bool gamecenter_available = false;

@interface GameCenterLeaderboardViewController : GKGameCenterViewController 
@end

@implementation GameCenterLeaderboardViewController
@end

static GameCenterLeaderboardViewController *leaderboard_view_controller = nil;

@interface GameCenterDelegate: NSObject<GKGameCenterControllerDelegate>
- (void)gameCenterViewControllerDidFinish:(GKGameCenterViewController *)viewController;
@end

static GameCenterDelegate *gamecenter_delegate = nil;

@implementation GameCenterDelegate
- (void)gameCenterViewControllerDidFinish:(GKGameCenterViewController *)viewController {
    ios_paused = false;
    if (ios_view_controller != nil) {
        [ios_view_controller dismissViewControllerAnimated:YES completion:nil];
        if (ios_view != nil && ios_eng != NULL) {
            // reset event data, because sometimes touch end events are missed
            // when the leaderboard view controller is displayed.
            am_find_window((am_native_window*)ios_view)->push(ios_eng->L);
            am_call_amulet(ios_eng->L, "_reset_window_event_data", 1, 0);
        }
    }
}
@end

@interface GameCenterSubmitter : NSObject
+(void)submitScore:(id)data;
@end

@implementation GameCenterSubmitter

+(void)submitScore:(id)data{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    double score = [[data objectForKey:@"score"] doubleValue];
    NSString* leaderboardCategory = [data objectForKey:@"category"];
    GKScore *scoreReporter = [[[GKScore alloc] initWithLeaderboardIdentifier:leaderboardCategory] autorelease];
    scoreReporter.value = (int64_t)score;
    [GKScore reportScores:@[scoreReporter] withCompletionHandler:^(NSError *error)
       {
           if (error) {
                NSLog(@"Game Center Score Error: %@", [error localizedDescription]);
           }
       }
    ];
    [data release];
    [pool release];
}

+(void)submitAchievement:(id)data{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *name = [data objectForKey:@"achievement"];
    GKAchievement *achievement = [[[GKAchievement alloc] initWithIdentifier:name] autorelease];
    achievement.percentComplete = 100.0;
    [GKAchievement reportAchievements:@[achievement] withCompletionHandler:^(NSError *error)
       {
           if (error) {
                NSLog(@"Game Center Achievement Error: %@", [error localizedDescription]);
           }
       }
    ];
    [data release];
    [pool release];
}

@end

static int init_gamecenter(lua_State *L) {
/*
    [[GKLocalPlayer localPlayer] authenticateWithCompletionHandler:^(NSError *error) {
        if (error == nil) {
            gamecenter_available = true;
        } else {
            NSString *description = error.localizedDescription;
            NSString *reason = error.localizedFailureReason;
            am_debug("gamecenter initialisation failed: %s (%s)", description.UTF8String, reason.UTF8String);
            gamecenter_available = false;
        }
    }];
*/

    if ([GKLocalPlayer localPlayer].authenticated == NO) {
        GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];
        [localPlayer setAuthenticateHandler:(^(UIViewController* viewController, NSError *error) {
            if(localPlayer.isAuthenticated) {
                gamecenter_available = true;
            } else {
                //problem with authentication, probably because the user doesn't use Game Center
                //am_debug("%s", "gamecenter authentication failed");
            }
        })]; 
    } else {
        NSLog(@"Already authenticated!");   
    }
    gamecenter_delegate = [[GameCenterDelegate alloc] init];
    [gamecenter_delegate retain];

    return 0;
}

/*
static int destroy_gamecenter(lua_State *L) {
    //fprintf(stderr, "gamecenter_delegate ref_count = %d\n", [gamecenter_delegate retain_count]);
    [gamecenter_delegate release];
    if (leaderboard_view_controller != nil) {
        //fprintf(stderr, "leaderboard_view_controller ref_count = %d\n", [leaderboard_view_controller retain_count]);
        [leaderboard_view_controller release];
    }
    return 0;
}
*/

static int submit_gamecenter_score(lua_State *L) {
    am_check_nargs(L, 2);
    const char *leaderboard = lua_tostring(L, 1);
    if (leaderboard == NULL) {
        return luaL_error(L, "expecting a string in position 1");
    }
    double score = lua_tonumber(L, 2);
    if (gamecenter_available) {
        NSDictionary* data = [[NSDictionary alloc]
            initWithObjectsAndKeys:[NSNumber numberWithDouble:score], @"score",
            [NSString stringWithUTF8String:leaderboard], @"category", nil];
        [NSThread detachNewThreadSelector:@selector(submitScore:) toTarget:[GameCenterSubmitter class] withObject:data];
    }
    return 0;
}

static int submit_gamecenter_achievement(lua_State *L) {
    am_check_nargs(L, 1);
    const char *achievement = lua_tostring(L, 1);
    if (achievement == NULL) {
        return luaL_error(L, "expecting a string in position 1");
    }
    if (gamecenter_available) {
        NSDictionary* data = [[NSDictionary alloc]
            initWithObjectsAndKeys:
            [NSString stringWithUTF8String:achievement], @"achievement", nil];
        [NSThread detachNewThreadSelector:@selector(submitAchievement:) toTarget:[GameCenterSubmitter class] withObject:data];
    }
    return 0;
}

static int show_gamecenter_leaderboard(lua_State *L) {
    am_check_nargs(L, 1);
    const char *leaderboard = lua_tostring(L, 1);
    if (leaderboard == NULL) {
        return luaL_error(L, "expecting a string in position 1");
    }
    //am_debug("showing game center leaderboard %s (%s)", leaderboard, gamecenter_available ? "available" : "not available");
    if (gamecenter_available && ios_view_controller != nil) {
        NSString *category = [NSString stringWithUTF8String:leaderboard];
        if (leaderboard_view_controller == nil) {
            leaderboard_view_controller = [[GameCenterLeaderboardViewController alloc] init];
            [leaderboard_view_controller retain];
            leaderboard_view_controller.gameCenterDelegate = gamecenter_delegate;
        }
        leaderboard_view_controller.leaderboardIdentifier = category;
        [ios_view_controller presentViewController:leaderboard_view_controller animated:YES completion:^{
            ios_paused = true;
        }];
    }
    return 0;
}

static int gamecenter_is_available(lua_State *L) {
    lua_pushboolean(L, gamecenter_available);
    return 1;
}

const char *am_preferred_language() {
    NSString *languageID = [[NSBundle mainBundle] preferredLocalizations].firstObject;
    return [languageID UTF8String];
}

char *am_open_file_dialog() {
    return NULL;
}

static int init_google_banner_ad(lua_State *L) {
#ifdef AM_GOOGLE_ADS
    am_check_nargs(L, 1);
    if (banner_ad != nil) return luaL_error(L, "google ads already initialized");
    if (ios_view == nil) return luaL_error(L, "internal error: ios_view not initialized");
    if (ios_view_controller == nil) return luaL_error(L, "internal error: ios_view_controller not initialized");
    const char *unitid = lua_tostring(L, 1);
    if (unitid == NULL) return luaL_error(L, "expecting a string unitid argument");
    banner_ad = [[GADBannerView alloc] initWithAdSize:kGADAdSizeSmartBannerPortrait];
    banner_ad.hidden = YES;
    banner_ad.frame = CGRectMake(0.0, -banner_ad.frame.size.height,
                              banner_ad.frame.size.width,
                              banner_ad.frame.size.height);
    banner_ad.adUnitID = [NSString stringWithUTF8String:unitid];
    banner_ad.rootViewController = ios_view_controller;
    banner_ad.delegate = banner_delegate;
    [ios_view addSubview:banner_ad];
#endif
    return 0;
}

static int set_google_banner_ad_visible(lua_State *L) {
#ifdef AM_GOOGLE_ADS
    am_check_nargs(L, 1);
    if (banner_ad == nil) return luaL_error(L, "please initialse ads first");
    bool vis = lua_toboolean(L, 1) ? true : false;
    banner_ad_want_visible = vis;
    if (vis && banner_ad_filled) {
        show_banner();
    } else if (!vis) {
        hide_banner();
    }
#endif
    return 0;
}

static int is_google_banner_ad_visible(lua_State *L) {
#ifdef AM_GOOGLE_ADS
    lua_pushboolean(L, banner_ad_is_visible ? 1 : 0);
#else
    lua_pushboolean(L, 0);
#endif
    return 1;
}

static int request_google_banner_ad(lua_State *L) {
#ifdef AM_GOOGLE_ADS
    if (banner_ad == nil) return luaL_error(L, "please initialse ads first");
    [banner_ad loadRequest:[GADRequest request]];
    hide_banner();
    banner_ad_filled = false;
#endif
    return 0;
}

static int get_banner_ad_height(lua_State *L) {
#ifdef AM_GOOGLE_ADS
    if (banner_ad != nil && ios_view != nil) {
        float scale = ios_view.contentScaleFactor;
        float h = banner_ad.frame.size.height * scale;
        lua_pushnumber(L, (lua_Number)h);
    } else {
        lua_pushnumber(L, 0.0);
    }
#else
    lua_pushnumber(L, 0.0);
#endif
    return 1;
}

struct am_iap_product : am_nonatomic_userdata {
    SKProduct *product;
};

static int iap_product_gc(lua_State *L) {
    am_iap_product *product = am_get_userdata(L, am_iap_product, 1);
    [product->product release];
    return 0;
}

static void register_iap_product_mt(lua_State *L) {
    lua_newtable(L);

    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, iap_product_gc, 0);
    lua_setfield(L, -2, "__gc");

    am_register_metatable(L, "iap_product", MT_am_iap_product, 0);
}

@interface RetrieveIAPProductsDelegate: NSObject<SKProductsRequestDelegate>
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response;
- (void)request:(SKRequest *)request didFailWithError:(NSError *)error;
@end

@implementation RetrieveIAPProductsDelegate
- (void)productsRequest:(SKProductsRequest *)request
     didReceiveResponse:(SKProductsResponse *)response
{
    // run on main thread, because calls into lua engine.
    [self performSelectorOnMainThread:@selector(productsRequestMain:) withObject:response waitUntilDone:YES];
    [request release];
}

- (void)productsRequestMain:(SKProductsResponse *)response
{
    if (ios_eng != NULL && ios_eng->L != NULL) {
        lua_State *L = ios_eng->L;
        lua_newtable(L);
        for (SKProduct *product in response.products) {
            if (product != nil) {
                [product retain];
                lua_pushstring(L, [[product productIdentifier] UTF8String]);
                am_new_userdata(L, am_iap_product)->product = product;
                lua_settable(L, -3);
            }
        } 
        am_call_amulet(L, "_iap_retrieve_products_finished", 1, 0);
    }
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error;
{
    [self performSelectorOnMainThread:@selector(productsRequestErrorMain) withObject:nil waitUntilDone:YES];
    [request release];
}

- (void)productsRequestErrorMain
{
    if (ios_eng != NULL && ios_eng->L != NULL) {
        lua_State *L = ios_eng->L;
        lua_pushnil(L);
        am_call_amulet(L, "_iap_retrieve_products_finished", 1, 0);
    }
}

@end

static int retrieve_iap_products(lua_State *L) {
    am_check_nargs(L, 1);
    if (!lua_istable(L, 1)) return luaL_error(L, "expecting a table in position 1");
    int n = lua_objlen(L, 1);
    NSMutableSet *ids = [[NSMutableSet alloc] init];
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, 1, i);
        const char *pid = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (pid != NULL) {
            NSString *id = [NSString stringWithUTF8String:pid];
            [ids addObject:id];
        }
    }
    SKProductsRequest *productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:ids];
    [ids dealloc];
                     
    [productsRequest retain];
    productsRequest.delegate = [[RetrieveIAPProductsDelegate alloc] init];
    [productsRequest start];
    return 0;
}

static int purchase_iap_product(lua_State *L) {
    am_check_nargs(L, 1);
    am_iap_product *product = am_get_userdata(L, am_iap_product, 1);
    SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:product->product];
    payment.quantity = 1;
    [[SKPaymentQueue defaultQueue] addPayment:payment];
    return 0;
}

static int restore_iap_purchases(lua_State *L) {
    [[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
    return 0;
}

static int can_make_iap_payments(lua_State *L) {
    if ([SKPaymentQueue canMakePayments]) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

static int iap_product_local_price(lua_State *L) {
    am_check_nargs(L, 1);
    am_iap_product *product = am_get_userdata(L, am_iap_product, 1);
    NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
    [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
    [numberFormatter setLocale:product->product.priceLocale];
    const char *formattedPrice = [[numberFormatter stringFromNumber:product->product.price] UTF8String];
    [numberFormatter dealloc];
    lua_pushstring(L, formattedPrice);
    return 1;
}

static int ios_request_review(lua_State *L) {
    if([SKStoreReviewController class]){
       [SKStoreReviewController requestReview] ;
    } 
    return 0;
}

static int launch_url(lua_State *L) {
    am_check_nargs(L, 1);
    const char *url = lua_tostring(L, 1);
    if (url == NULL) return 0;
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]] options:@{} completionHandler:nil];
    return 0;
}

lua_State *am_get_global_lua_state() {
    if (ios_eng != NULL) {
        return ios_eng->L;
    }
    return NULL;
}

void am_pause_app() {
    ios_paused = true;
}

void am_resume_app() {
    ios_paused = false;
    if (ios_eng != NULL && ios_eng->L != NULL) {
        am_find_window((am_native_window*)ios_view)->push(ios_eng->L);
        am_call_amulet(ios_eng->L, "_reset_window_event_data", 1, 0);
    }
}

static int show_alert(lua_State *L) {
    if (ios_view_controller == nil) return 0;
    am_check_nargs(L, 1);
    const char *msg = lua_tostring(L, 1);
    if (msg == NULL) {
        msg = "";
    }
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:""]
                               message:[NSString stringWithUTF8String:msg]
                               preferredStyle:UIAlertControllerStyleAlert];

    UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
       handler:^(UIAlertAction * action) {}];
    [alert addAction:defaultAction];
    [ios_view_controller presentViewController:alert animated:YES completion:nil];
    return 0;
}

@interface AMWebViewDelegate : NSObject <WKNavigationDelegate>

@property (nonatomic) BOOL is_finished;
@property (nonatomic) BOOL was_error;

@end

@implementation AMWebViewDelegate

- (void)webView:(WKWebView *)webView 
    didFailNavigation:(WKNavigation *)navigation 
    withError:(NSError *)error
{
    //NSLog(@"------------------------ webkit navigation error");
    self.was_error = YES;
}

- (void)webView:(WKWebView *)webView 
    didFailProvisionalNavigation:(WKNavigation *)navigation 
    withError:(NSError *)error
{
    //NSLog(@"------------------------ webkit navigation provisional error");
    self.was_error = YES;
}

- (void)webView:(WKWebView *)webView
    didFinishNavigation:(WKNavigation *)navigation
{
    //NSLog(@"------------------------ webkit navigation finished");
    self.is_finished = YES;
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    NSURL *url = navigationAction.request.URL;
    if (url.scheme != nil && ![WKWebView handlesURLScheme: url.scheme] && [[UIApplication sharedApplication] canOpenURL:url]) {
        [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
    }
    decisionHandler(WKNavigationActionPolicyAllow);
}

@end

struct am_webview : am_nonatomic_userdata {
    WKWebView *webview;
    AMWebViewDelegate* delegate;
    bool is_visible;
};

static int webview_gc(lua_State *L) {
    am_webview *webview = am_get_userdata(L, am_webview, 1);
    if (webview->is_visible && ios_view != nil) {
        [webview->webview removeFromSuperview];
        webview->is_visible = false;
    }
    [webview->webview release];
    [webview->delegate release];
    return 0;
}

static int create_webview(lua_State *L) {
    am_check_nargs(L, 5);
    const char *url = lua_tostring(L, 1);
    if (url == NULL) {
        return luaL_error(L, "expecting a url in position 1");
    }
    if (ios_view == nil) {
        return luaL_error(L, "create a window before creating a webview");
    }

    float scl = ios_view.contentScaleFactor;
    float x1 = (float)luaL_checknumber(L, 2) / scl;
    float y1 = (float)luaL_checknumber(L, 3) / scl;
    float x2 = (float)luaL_checknumber(L, 4) / scl;
    float y2 = (float)luaL_checknumber(L, 5) / scl;
    float w = (x2 - x1);
    float h = (y2 - y1);
    y1 = ios_view.bounds.size.height - y2;

    CGRect bounds = CGRectMake(x1, y1, w, h);
    WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];
    config.ignoresViewportScaleLimits = FALSE;
    WKWebView* v = [[WKWebView alloc] initWithFrame: bounds configuration: config];
    v.backgroundColor = UIColor.whiteColor;
    [config release];
    AMWebViewDelegate* delegate = [[AMWebViewDelegate alloc] init];
    delegate.is_finished = NO;
    delegate.was_error = NO;
    v.navigationDelegate = delegate;
    NSURLRequest *u = [NSURLRequest requestWithURL: [NSURL URLWithString: [NSString stringWithUTF8String: url]]];
    [v loadRequest: u];

    am_webview *wv = am_new_userdata(L, am_webview);
    wv->webview = v;
    wv->delegate = delegate;
    wv->is_visible = false;

    return 1;
}

static int show_webview(lua_State *L) {
    am_check_nargs(L, 1);
    am_webview *wv = am_get_userdata(L, am_webview, 1);
    if (wv->is_visible || ios_view == nil) return 0;
    [ios_view addSubview: wv->webview];
    wv->is_visible = true;
    return 0;
}

static int hide_webview(lua_State *L) {
    am_check_nargs(L, 1);
    am_webview *wv = am_get_userdata(L, am_webview, 1);
    if (!wv->is_visible) return 0;
    [wv->webview removeFromSuperview];
    wv->is_visible = false;
    return 0;
}

static int webview_is_finished(lua_State *L) {
    am_check_nargs(L, 1);
    am_webview *wv = am_get_userdata(L, am_webview, 1);
    if (wv->delegate != nil) {
        lua_pushboolean(L, [wv->delegate is_finished] == YES ? 1 : 0);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

static int webview_was_error(lua_State *L) {
    am_check_nargs(L, 1);
    am_webview *wv = am_get_userdata(L, am_webview, 1);
    if (wv->delegate != nil) {
        lua_pushboolean(L, [wv->delegate was_error] == YES ? 1 : 0);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

static void register_webview_mt(lua_State *L) {
    lua_newtable(L);

    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, webview_gc, 0);
    lua_setfield(L, -2, "__gc");

    lua_pushcclosure(L, show_webview, 0);
    lua_setfield(L, -2, "show");
    lua_pushcclosure(L, hide_webview, 0);
    lua_setfield(L, -2, "hide");
    lua_pushcclosure(L, webview_is_finished, 0);
    lua_setfield(L, -2, "finished_loading");
    lua_pushcclosure(L, webview_was_error, 0);
    lua_setfield(L, -2, "was_error");

    am_register_metatable(L, "webview", MT_am_webview, 0);
}

void am_open_ios_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"init_gamecenter", init_gamecenter},
        {"gamecenter_available", gamecenter_is_available},
        {"submit_gamecenter_score", submit_gamecenter_score},
        {"submit_gamecenter_achievement", submit_gamecenter_achievement},
        {"show_gamecenter_leaderboard", show_gamecenter_leaderboard},
        //{"destroy_gamecenter", destroy_gamecenter},

        {"ios_store_pref", ios_store_pref},
        {"ios_retrieve_pref", ios_retrieve_pref},

        {"force_touch_available", force_touch_available},

        {"init_google_banner_ad", init_google_banner_ad},
        {"set_google_banner_ad_visible", set_google_banner_ad_visible},
        {"is_google_banner_ad_visible", is_google_banner_ad_visible},
        {"request_google_banner_ad", request_google_banner_ad},
        {"get_banner_ad_height", get_banner_ad_height},

        {"retrieve_iap_products", retrieve_iap_products},
        {"purchase_iap_product", purchase_iap_product},
        {"restore_iap_purchases", restore_iap_purchases},
        {"can_make_iap_payments", can_make_iap_payments},
        {"iap_product_local_price", iap_product_local_price},

        {"ios_request_review", ios_request_review},
        {"show_alert", show_alert},
        {"create_webview", create_webview},

        {"launch_url", launch_url},

        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_iap_product_mt(L);
    register_webview_mt(L);
}

#endif // AM_BACKEND_IOS
