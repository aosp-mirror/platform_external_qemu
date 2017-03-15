// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AndroidPropertyPipe.h"

#include "android/android-property-pipe.h"
#include "android/emulation/PropertyStore.h"
#include "android/utils/debug.h"

#define D(...) VERBOSE_TID_DPRINT(aprops, __VA_ARGS__)
#define DF(...) VERBOSE_TID_FUNCTION_DPRINT(aprops, __VA_ARGS__)

using android::base::StringView;

namespace android {
namespace emulation {

AndroidPropertyPipe::AndroidPropertyPipe(void* hwPipe, Service* svc)
    : AndroidPipe(hwPipe, svc) {}

AndroidPropertyPipe::~AndroidPropertyPipe() {}

void AndroidPropertyPipe::onGuestClose(PipeCloseReason reason) {}

unsigned AndroidPropertyPipe::onGuestPoll() const {
    // Guest can always write
    return PIPE_POLL_OUT;
}

int AndroidPropertyPipe::onGuestRecv(AndroidPipeBuffer* buffers,
                                     int numBuffers) {
    // Guest is not supposed to read
    return PIPE_ERROR_IO;
}

int AndroidPropertyPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                                     int numBuffers) {
    int result = 0;

    while (numBuffers > 0) {
        // An Android property has the form of:
        //   <key>=<value>
        // Where <key> is the action part of the intent, and
        // <value> is the data part.

        if (buffers->data) {
            // Send this data to the property store.
            PropertyStore::get().write(StringView(
                    reinterpret_cast<char*>(buffers->data),
                    static_cast<int>(buffers->size)));
        }
        result += static_cast<int>(buffers->size);
        buffers++;
        numBuffers--;
    }
    return result;
}

void registerAndroidPropertyPipeService() {
    android::AndroidPipe::Service::add(new AndroidPropertyPipe::Service());
}

////////////////////////////////////////////////////////////////////////////////

AndroidPropertyPipe::Service::Service()
    : AndroidPipe::Service("AndroidPropertyPipe") {}

AndroidPipe* AndroidPropertyPipe::Service::create(void* hwPipe,
                                                  const char* args) {
    return new AndroidPropertyPipe(hwPipe, this);
}

}  // namespace emulation
}  // namespace android

void android_init_android_property_pipe(void) {
    android::emulation::registerAndroidPropertyPipeService();
}
