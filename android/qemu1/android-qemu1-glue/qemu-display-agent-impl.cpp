// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android-qemu1-glue/qemu-control-impl.h"

extern "C" {
    #include "ui/console.h"
}

static void getFrameBuffer(int* w, int* h, int* lineSize, int* bytesPerPixel,
                           uint8_t** frameBufferData) {
    DisplayState* const ds = get_displaystate();

    if (w) {
        *w = ds->surface->width;
    }
    if (h) {
        *h = ds->surface->height;
    }
    if (lineSize) {
        *lineSize = ds->surface->linesize;
    }
    if (bytesPerPixel) {
        *bytesPerPixel = ds->surface->pf.bytes_per_pixel;
    }
    if (frameBufferData) {
        *frameBufferData = ds->surface->data;
    }
}

static void registerUpdateListener(AndroidDisplayUpdateCallback callback,
                                   void* opaque) {
    const auto listener = new DisplayUpdateListener();
    listener->dpy_update = callback;
    listener->opaque = opaque;

    DisplayState* const ds = get_displaystate();
    register_displayupdatelistener(ds, listener);
}


static const QAndroidDisplayAgent displayAgent = {
        .getFrameBuffer = &getFrameBuffer,
        .registerUpdateListener = &registerUpdateListener
};

const QAndroidDisplayAgent* const gQAndroidDisplayAgent = &displayAgent;
