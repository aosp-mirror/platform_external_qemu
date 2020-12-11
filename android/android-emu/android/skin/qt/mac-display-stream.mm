#import "android/skin/qt/mac-display-stream.h"
#include <CoreGraphics/CGDisplayStream.h>
#include <IOSurface/IOSurfaceAPI.h>
#include "android/skin/winsys.h"

#if DEBUG
#include "android/utils/debug.h"
#define D(...) VERBOSE_PRINT(surface, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

static CGDisplayStreamRef sref = NULL;
static int s_x = 0;
static int s_y = 0;
static void (*s_cb)(int frame, uint64_t timeStamp) = NULL;

static void s_frameAvailable(CGDisplayStreamFrameStatus status,
                             uint64_t displayTime,
                             IOSurfaceRef frameSurface,
                             CGDisplayStreamUpdateRef updateRef) {
    if (kCGDisplayStreamFrameStatusFrameComplete == status &&
        NULL != frameSurface) {
        size_t bpp = IOSurfaceGetBytesPerElement(frameSurface);
        size_t bpr = IOSurfaceGetBytesPerRow(frameSurface);
        IOSurfaceLock(frameSurface, kIOSurfaceLockReadOnly, NULL);

        uint8_t* pix = (uint8_t*)IOSurfaceGetBaseAddress(frameSurface);
        if (NULL != pix) {
            int32_t r = pix[s_y * bpr + s_x * bpp];
            int32_t g = pix[s_y * bpr + s_x * bpp + 1];
            int32_t b = pix[s_y * bpr + s_x * bpp + 2];
            int fr = r + (g << 8) + (b << 16);
            if (s_cb != NULL) {
                s_cb(fr, displayTime);
            }
        }
        IOSurfaceUnlock(frameSurface, kIOSurfaceLockReadOnly, NULL);
    }
}

bool startDisplayStream(int x, int y, void (*callback_t)(int, uint64_t)) {
    s_cb = callback_t;
    s_x = x;
    s_y = y;
    // Query screen width and height
    size_t output_width = 1440;
    size_t output_height = 900;
    SkinRect monitor;
    skin_winsys_get_monitor_rect(&monitor);
    output_width = monitor.size.w;
    output_height = monitor.size.h;
    uint32_t pixel_format = 'BGRA';
    CGDirectDisplayID display_ids[2]; /* just a typedef'd uint32_t */
    uint32_t found_displays = 0;
    CGError err = CGGetActiveDisplayList(2, display_ids, &found_displays);
    if (kCGErrorSuccess != err) {
        D("Error: failed to retrieve displays.\n");
        return false;
    }
    if (0 == found_displays) {
        D("Error: no active displays found.\n");
        return false;
    }
    dispatch_queue_t dq = dispatch_queue_create("android-emu-visual-metrics",
                                                DISPATCH_QUEUE_SERIAL);
    sref = CGDisplayStreamCreateWithDispatchQueue(
            display_ids[0], output_width, output_height, pixel_format, NULL, dq,
            ^(CGDisplayStreamFrameStatus
                      status, /* kCGDisplayStreamFrameComplete, *FrameIdle,
                               *FrameBlank, *Stopped */
              uint64_t time,  /* Mach absolute time when the event occurred. */
              IOSurfaceRef
                      frame, /* opaque pixel buffer, can be backed by GL, CL,
                                etc.. This may be NULL in some cases. See the
                                docs if you want to keep access to this. */
              CGDisplayStreamUpdateRef ref) {
              s_frameAvailable(status, time, frame, ref);
            });
    err = CGDisplayStreamStart(sref);
    if (kCGErrorSuccess != err) {
        return false;
    } else {
        return true;
    }
}

void stopDisplayStream() {
    if (sref != NULL)
        CGDisplayStreamStop(sref);
}