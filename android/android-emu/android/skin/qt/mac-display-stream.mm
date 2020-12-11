#import "android/skin/qt/mac-display-stream.h"

#include <CoreGraphics/CGDisplayStream.h>
#include <IOSurface/IOSurfaceAPI.h>

static CGDisplayStreamRef sref;
/*static NSMutableArray *array = NULL;
struct DataPoint {
    uint64_t timestamp;
    int32_t frame;
    bool used;
    DataPoint() {
        timestamp = 0;
        frame = 0;
        used = false;
    }
} book;*/
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
            // fprintf(stderr, "r %u g %u b %u a %u and fr %u\n", r, g, b, a,
            // fr);
        }
        IOSurfaceUnlock(frameSurface, kIOSurfaceLockReadOnly, NULL);
    }
}

bool startDisplayStream(int x, int y, void (*callback_t)(int, uint64_t)) {
    s_cb = callback_t;
    s_x = x;
    s_y = y;
    // hard-coded. Needs to be queried first.
    size_t output_width = 1440;
    size_t output_height = 900;
    uint32_t pixel_format = 'BGRA';
    CGDirectDisplayID display_ids[2]; /* just a typedef'd uint32_t */
    uint32_t found_displays = 0;
    CGError err = CGGetActiveDisplayList(2, display_ids, &found_displays);
    if (kCGErrorSuccess != err) {
        fprintf(stderr, "Error: failed to retrieve displays.\n");
        return false;
    }
    if (0 == found_displays) {
        fprintf(stderr, "Error: no active displays found.\n");
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
    CGError err = CGDisplayStreamStop(sref);
    if (kCGErrorSuccess != err) {
        exit(EXIT_FAILURE);
    }
}