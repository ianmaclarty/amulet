#include "amulet.h"

#ifdef AM_BACKEND_IOS

#import <objc/runtime.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

#include <mach/mach_time.h>

int main(int argc, char *argv[]) {
    am_debug("%s", "Hello!!");
    sleep(5);
    return 0;
}

am_native_window *am_create_native_window(
    am_window_mode mode,
    int top, int left,
    int width, int height,
    const char *title,
    bool resizable,
    bool borderless,
    bool depth_buffer,
    bool stencil_buffer,
    int msaa_samples)
{
    return NULL;
}

void am_get_native_window_size(am_native_window *window, int *w, int *h) {
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
    return 0;
}

void *am_read_resource(const char *filename, int *len, char** errmsg) {
    return NULL;
}





#endif // AM_BACKEND_IOS
