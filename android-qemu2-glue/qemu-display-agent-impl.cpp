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

#include "android-qemu2-glue/display.h"
#include "android-qemu2-glue/qemu-control-impl.h"

extern "C" {
#include "qemu/osdep.h"
#include "ui/console.h"
}

#include <string.h>

#include <algorithm>
#include <vector>

static void initFrameBufferNoWindow(QFrameBuffer* qf) {
    android_display_init_no_window(qf);
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

struct CallbackInfo {
    AndroidDisplayUpdateCallback cb;
    void* opaque;
};

struct AndroidDisplayChangeListener : public DisplayChangeListener {
    AndroidDisplayChangeListener(AndroidDisplayUpdateCallback callback,
                                 void* opaque) {
        memset(this, 0, sizeof(*this));
        mCallbacks.push_back({callback, opaque});
        this->ops = &kOps;
        register_displaychangelistener(this);
    }

    ~AndroidDisplayChangeListener() {
        unregister_displaychangelistener(this);
    }

    std::vector<CallbackInfo> mCallbacks;

    void attachCallback(AndroidDisplayUpdateCallback callback, void* opaque) {
        mCallbacks.push_back({callback, opaque});
    }

    void detachCallback(AndroidDisplayUpdateCallback callback) {
        mCallbacks.erase(std::remove_if(mCallbacks.begin(), mCallbacks.end(),
                                        [callback](const CallbackInfo& c) {
                                            return c.cb == callback;
                                        }),
                         mCallbacks.end());
    }

    static void onDisplayUpdate(DisplayChangeListener* dcl,
                                int x, int y, int w, int h) {
        auto adcl = reinterpret_cast<AndroidDisplayChangeListener*>(dcl);
        for (auto info : adcl->mCallbacks) {
            info.cb(info.opaque, x, y, w, h);
        }
    }

    static const DisplayChangeListenerOps kOps;
};

// static
const DisplayChangeListenerOps AndroidDisplayChangeListener::kOps = {
    .dpy_name = "qemu2 display",
    .dpy_refresh = nullptr,
    .dpy_gfx_update = &onDisplayUpdate,
};

}  // namespace

static AndroidDisplayChangeListener* s_listener = nullptr;

static void registerUpdateListener(AndroidDisplayUpdateCallback callback,
                                   void* opaque) {
    if (!s_listener) {
        s_listener = new AndroidDisplayChangeListener(callback, opaque);
    } else {
        s_listener->attachCallback(callback, opaque);
    }
}

static void unregisterUpdateListener(AndroidDisplayUpdateCallback callback) {
    assert(s_listener);

    s_listener->detachCallback(callback);
}

static const QAndroidDisplayAgent displayAgent = {
        .getFrameBuffer = &getFrameBuffer,
        .registerUpdateListener = &registerUpdateListener,
        .unregisterUpdateListener = &unregisterUpdateListener,
        .initFrameBufferNoWindow = &initFrameBufferNoWindow,
};

const QAndroidDisplayAgent* const gQAndroidDisplayAgent = &displayAgent;
