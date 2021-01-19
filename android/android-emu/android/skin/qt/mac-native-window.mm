/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include <Carbon/Carbon.h>
#include <Cocoa/Cocoa.h>
#include <CoreGraphics/CGDisplayStream.h>
#include <IOSurface/IOSurfaceAPI.h>

#import "android/skin/qt/mac-native-window.h"
#include "android/skin/winsys.h"
#include "android/utils/system.h"

#if DEBUG
#include "android/utils/debug.h"
#define D(...) VERBOSE_PRINT(surface, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

void* getNSWindow(void* ns_view) {
    NSView *view = (NSView *)ns_view;
    if (!view) {
        return NULL;
    }
    Class viewClass = [view class];
    Class nsviewClass = [NSView class];
    if ([viewClass isSubclassOfClass:nsviewClass ]) {
        return [view window];
    } else {
        return view;
    }

}

void nsWindowHideWindowButtons(void* ns_window) {
    NSWindow *window = (NSWindow *)ns_window;
    if (!window) {
        return;
    }
    [[window standardWindowButton:NSWindowCloseButton] setHidden:true];
    [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:true];
    [[window standardWindowButton:NSWindowZoomButton] setHidden:true];
    [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
}

int numHeldMouseButtons() {
    return (int) [NSEvent pressedMouseButtons];
}

void nsWindowAdopt(void *ns_parent, void *ns_child) {
    NSWindow *parent = (NSWindow*)ns_parent, *child = (NSWindow*)ns_child;
    [parent addChildWindow:child ordered:NSWindowAbove];
}

bool isOptionKeyHeld() {
    return (NSEventModifierFlagOption & [NSEvent modifierFlags]) != 0;
}

const char* keyboard_host_layout_name_macImpl() {
    TISInputSourceRef inputSource = TISCopyCurrentKeyboardLayoutInputSource();
    NSString* inputSourceID = (NSString*)TISGetInputSourceProperty(
            inputSource, kTISPropertyInputSourceID);
    return [inputSourceID UTF8String];
}

static CGDisplayStreamRef sref = NULL;
static void (*sCallback)(uint8_t*, uint64_t) = NULL;

static void frameAvailable(CGDisplayStreamFrameStatus status,
                           uint64_t displayTime,
                           IOSurfaceRef frameSurface,
                           CGDisplayStreamUpdateRef updateRef) {
    if (kCGDisplayStreamFrameStatusFrameComplete == status &&
        NULL != frameSurface) {
        IOSurfaceLock(frameSurface, kIOSurfaceLockReadOnly, NULL);
        uint8_t* pix = (uint8_t*)IOSurfaceGetBaseAddress(frameSurface);
        if (NULL != pix) {
            sCallback(pix, displayTime);
        }
        IOSurfaceUnlock(frameSurface, kIOSurfaceLockReadOnly, NULL);
    }
}

bool startDisplayStream(void (*callback)(uint8_t*, uint64_t)) {
    sCallback = callback;
    size_t output_width = 1440;
    size_t output_height = 900;
    SkinRect monitor;
    skin_winsys_get_monitor_rect(&monitor);
    output_width = monitor.size.w;
    output_height = monitor.size.h;
    // Query screen width and height
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
    dispatch_queue_t dq = dispatch_queue_create("android-emu-ui-metrics",
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
              frameAvailable(status, time, frame, ref);
            });
    err = CGDisplayStreamStart(sref);
    return kCGErrorSuccess == err;
}

void stopDisplayStream() {
    if (NULL != sref)
        CGDisplayStreamStop(sref);
}