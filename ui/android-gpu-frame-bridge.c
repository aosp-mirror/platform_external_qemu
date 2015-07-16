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

#include "android/gpu-frame-bridge.h"

#include "qemu/main-loop.h"
#include "qemu/thread.h"
#include "qemu/event_notifier.h"

#include <glib.h>

#include <string.h>

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define D(...)  printf(__VA_ARGS__), fflush(stdout)
#else
#define D(...)  ((void)0)
#endif

// A small structure used to hold a GPU Frame in memory.
typedef struct {
    int width;
    int height;
    void* pixels;
} GpuFrame;

static GpuFrame* gpu_frame_new(int w, int h, const void* pixels) {
    GpuFrame* frame = g_new0(GpuFrame, 1);
    frame->width = w;
    frame->height = h;
    frame->pixels = g_memdup(pixels, w * h * 4);
    return frame;
}

static void gpu_frame_free(GpuFrame* frame) {
    if (frame) {
        g_free(frame->pixels);
        g_free(frame);
    }
}

// Maximum number of frames in the bridge. Trying to post more frames
// than that will block.
#define MAX_GPU_FRAMES 16

typedef struct {
    bool init;
    QemuMutex lock;
    QemuCond can_write;
    EventNotifier can_read;
    int num_frames;
    GpuFrame* frames[MAX_GPU_FRAMES];
    GpuFrameBridgeCallback* callback;
    void* callback_opaque;
} GpuBridge;

static GpuBridge s_bridge = {
    .init = false,
    .num_frames = 0,
    .frames = { NULL, },
};

// This function is called from an EmuGL thread to post a new GPU frame.
static void emugl_frame_post(void* context,
                             int width,
                             int height,
                             int ydir,
                             int format,
                             int type,
                             unsigned char* pixels) {
    GpuBridge *bridge = &s_bridge;
    qemu_mutex_lock(&bridge->lock);
    while (bridge->num_frames >= MAX_GPU_FRAMES) {
        D("%s: frame array full\n", __FUNCTION__);
        qemu_cond_wait(&bridge->can_write, &bridge->lock);
    }
    bridge->frames[bridge->num_frames] = gpu_frame_new(width, height, pixels);
    bridge->num_frames += 1;
    qemu_mutex_unlock(&bridge->lock);
    event_notifier_set(&bridge->can_read);
}

// Called from the main loop whenever a new frame was notified from
// emugl_frame_post().
static void read_frame(EventNotifier* e) {
    GpuBridge* bridge = &s_bridge;

    if (event_notifier_test_and_clear(&bridge->can_read)) {
        qemu_mutex_lock(&bridge->lock);
        // Beware of spurious wakeups.
        if (bridge->num_frames > 0) {
            GpuFrame* frame = bridge->frames[0];
            memmove(bridge->frames, bridge->frames + 1,
                    (bridge->num_frames - 1) * sizeof(bridge->frames[0]));

            if (bridge->num_frames == MAX_GPU_FRAMES) {
                D("%s: frame array no longer full\n", __FUNCTION__);
                qemu_cond_signal(&bridge->can_write);
            }

            D("%s: new frame %dx%d\n", __FUNCTION__, frame->width,
              frame->height);
            bridge->callback(bridge->callback_opaque,
                            frame->width,
                            frame->height,
                            frame->pixels);
            gpu_frame_free(frame);

            bridge->num_frames -= 1;
            if (bridge->num_frames > 0) {
                event_notifier_set(&bridge->can_read);
            }
        } else {
            D("%s: spurious wakeup\n", __FUNCTION__);
        }
        qemu_mutex_unlock(&bridge->lock);
    }
}

void android_gpu_frame_bridge_init(GpuFrameBridgeCallback *callback,
                                   void *callback_opaque)
{
    GpuBridge *bridge = &s_bridge;

    if (bridge->init) {
        return;
    }

    bridge->init = true;
    bridge->callback = callback;
    bridge->callback_opaque = callback_opaque;
    qemu_mutex_init(&bridge->lock);
    qemu_cond_init(&bridge->can_write);
    event_notifier_init(&bridge->can_read, 0);
    event_notifier_set_handler(&bridge->can_read, read_frame);

    // Ensure EmuGL will call
    android_setPostCallback(&emugl_frame_post, bridge);
}
