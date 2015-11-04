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

#include "android-qemu2-glue/qemu-control-impl.h"

extern "C" {
    #include "ui/console.h"
}

static void getFrameBuffer(int* w, int* h, int* lineSize, int* bytesPerPixel,
                           uint8_t** frameBufferData) {
    // find the first graphics console
    QemuConsole* con = nullptr;
    for (int i = 0;; i++) {
        QemuConsole* c = qemu_console_lookup_by_index(i);
        if (!c) {
            break;
        }
        if (qemu_console_is_graphic(c)) {
            con = c;
            break;
        }
    }
    if (!con) {
        return;
    }

    DisplaySurface* const ds = qemu_console_surface(con);

    if (w) {
        *w = surface_width(ds);
    }
    if (h) {
        *h = surface_height(ds);
    }
    if (lineSize) {
        *lineSize = surface_stride(ds);
    }
    if (bytesPerPixel) {
        *bytesPerPixel = surface_bytes_per_pixel(ds);
    }
    if (frameBufferData) {
        *frameBufferData = (uint8_t*)surface_data(ds);
    }
}

namespace {
    struct dul_data {
        AndroidDisplayUpdateCallback callback;
        void* opaque;
    };
}

static void on_display_update(DisplayUpdateListener* dul,
        int x, int y, int w, int h) {
    dul_data* data = static_cast<dul_data*>(dul->opaque);
    data->callback(data->opaque, x, y, w, h);
}

static void registerUpdateListener(AndroidDisplayUpdateCallback callback,
                                   void* opaque) {
    const auto listener = new DisplayUpdateListener();
    *listener = DisplayUpdateListener();
    listener->dpy_gfx_update = &on_display_update;

    auto data = new dul_data();
    data->callback = callback;
    data->opaque = opaque;
    listener->opaque = data;

    register_displayupdatelistener(listener);
}


static const QAndroidDisplayAgent displayAgent = {
        .getFrameBuffer = &getFrameBuffer,
        .registerUpdateListener = &registerUpdateListener
};

const QAndroidDisplayAgent* const gQAndroidDisplayAgent = &displayAgent;
