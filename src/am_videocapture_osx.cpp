#include "amulet.h"

// Adapted from OpenCV (http://opencv.org/)

#if defined(AM_OSX) && !defined(AM_USE_METAL)

#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>

#import <AVFoundation/AVFoundation.h>

@interface CaptureDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
    int frameid;
    CVImageBufferRef  currentImageBuffer;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
fromConnection:(AVCaptureConnection *)connection;

- (void)updateTexture;
- (int)nextFrame;

@end

static bool video_initialized = false;
static AVCaptureSession            *captureSession = nil;
static AVCaptureDeviceInput        *captureDeviceInput = nil;
static AVCaptureVideoDataOutput    *captureDecompressedVideoOutput = nil;
static AVCaptureDevice             *captureDevice = nil;
static CaptureDelegate		   *captureDelegate = nil;

@implementation CaptureDelegate

- (id)init {
    [super init];
    frameid = 0;
    return self;
}

-(void)dealloc {
    if (frameid) CVBufferRelease(currentImageBuffer);
    [super dealloc];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
fromConnection:(AVCaptureConnection *)connection{

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);

    CVBufferRetain(imageBuffer);
    CVImageBufferRef prevImageBuffer = currentImageBuffer;
    bool needRelease = frameid > 0;

    @synchronized (self) {
        currentImageBuffer = imageBuffer;
        frameid++;
    }

    if (needRelease) {
        CVBufferRelease(prevImageBuffer);
    }

    //am_debug("%s", "captured new frame");
}

-(void) updateTexture {
    if (!frameid) {
        //am_debug("%s", "no image yet");
        return;
    }

    CVPixelBufferRef pixels;

    @synchronized (self){
        pixels = CVBufferRetain(currentImageBuffer);
    }

    CVPixelBufferLockBaseAddress(pixels, 0);
    void* baseaddress = CVPixelBufferGetBaseAddress(pixels);

    int width = (int)CVPixelBufferGetWidth(pixels);
    int height = (int)CVPixelBufferGetHeight(pixels);
    int rowBytes = (int)CVPixelBufferGetBytesPerRow(pixels);

    if (rowBytes != 0) {
        if (rowBytes != width * 4) {
            am_log1("WARNING: captured video frame has %d bytes per row, but is width %d", rowBytes, width);
        }
        if (rowBytes >= width * 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, baseaddress);
        }
    } else {
        am_log1("%s", "WARNING: capture video frame has no data");
    }

    CVPixelBufferUnlockBaseAddress(pixels, 0);
    CVBufferRelease(pixels);
}

-(int) nextFrame {
    return frameid;
}

@end

static bool init_video(int cam_id) {
    NSAutoreleasePool* localpool = [[NSAutoreleasePool alloc] init];
    NSArray* devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    if ([devices count] == 0) {
        am_log1("%s", "WARNING: no video capture devices detected");
        [localpool drain];
        return false;
    }
    if (cam_id >= 0) {
        if (cam_id > [devices count]) {
            am_log1("%s", "WARNING: requested camera id %d doesn't exist, using camera %d", cam_id, 0);
            cam_id = 0;
        }
        captureDevice = [devices objectAtIndex:cam_id];
    } else {
        captureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    }

    if (!captureDevice) {
        am_log1("%s", "WARNING: unable to get capture device");
        [localpool drain];
        return false;
    }

    captureDelegate = [[CaptureDelegate alloc] init];

    NSError* error = NULL;
    captureDeviceInput = [[AVCaptureDeviceInput alloc] initWithDevice:captureDevice error:&error];
    if (captureDeviceInput == nil) {
        if (error != NULL) {
            am_log1("captureDeviceInput is nil: %u: %s", [error code], [[error localizedDescription] UTF8String]);
        } else {
            am_log1("%s", "captureDeviceInput is nil");
        }
        [localpool drain];
        return false;
    }
    captureSession = [[AVCaptureSession alloc] init];

    captureDecompressedVideoOutput = [[AVCaptureVideoDataOutput alloc] init];
    /*
    am_debug("%s", "available formats:");
    NSArray *formats = captureDecompressedVideoOutput.availableVideoCVPixelFormatTypes;
    for (int i = 0; i < formats.count; i++) {
        uint32_t fmt = [[formats objectAtIndex:i] unsignedIntValue];
        char fmts[5];
        char *ptr = (char*)&fmt;
        fmts[0] = ptr[0];
        fmts[1] = ptr[1];
        fmts[2] = ptr[2];
        fmts[3] = ptr[3];
        fmts[4] = 0;
        am_debug("format = %u %x %s", fmt, fmt, fmts);
    }
    */
    dispatch_queue_t queue = dispatch_queue_create("cameraQueue", NULL);
    [captureDecompressedVideoOutput setSampleBufferDelegate:captureDelegate queue:queue];
    dispatch_release(queue);

    NSDictionary *pixelBufferOptions = [NSDictionary dictionaryWithObjectsAndKeys:
                [NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA],
                (id)kCVPixelBufferPixelFormatTypeKey,
                nil];
    captureDecompressedVideoOutput.videoSettings = pixelBufferOptions;
    captureDecompressedVideoOutput.alwaysDiscardsLateVideoFrames = YES;

    captureSession.sessionPreset = AVCaptureSessionPresetMedium;

    [captureSession addInput:captureDeviceInput];
    [captureSession addOutput:captureDecompressedVideoOutput];

    [captureSession startRunning];

    [localpool drain];
    return true;
}

void am_copy_video_frame_to_texture() {
    if (!video_initialized) {
        video_initialized = init_video(-1);
    }
    if (video_initialized) {
        [captureDelegate updateTexture];
    }
}

int am_next_video_capture_frame() {
    if (!video_initialized) {
        video_initialized = init_video(-1);
    }
    if (video_initialized) {
        return [captureDelegate nextFrame];
    } else {
        return 0;
    }
}

#endif // AM_OSX
