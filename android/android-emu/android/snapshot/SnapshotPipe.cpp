// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/async/ThreadLooper.h"
#include "android/emulation/AndroidPipe.h"
#include "android/snapshot/interface.h"

#include <assert.h>
#include <atomic>
#include <memory>

namespace {
class SnapshotPipe : public android::AndroidPipe {
public:
    class Service : public android::AndroidPipe::Service {
    public:
        Service() : android::AndroidPipe::Service("SnapshotPipe") {}
        bool canLoad() const override { return true; }

        android::AndroidPipe* create(void* hwPipe, const char* args) override {
            return new SnapshotPipe(hwPipe, this);
        }

        android::AndroidPipe* load(void* hwPipe, const char* args,
                         android::base::Stream* stream) override {
            return new SnapshotPipe(hwPipe, this, stream);
        }

        void preLoad(__attribute__((unused)) android::base::Stream* stream) override {
        }

        void preSave(__attribute__((unused)) android::base::Stream* stream) override {
        }
    };
    SnapshotPipe(void* hwPipe, Service* service,
              android::base::Stream* loadStream = nullptr)
    : AndroidPipe(hwPipe, service) {
        mIsLoad = static_cast<bool>(loadStream);
    }

    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override {
        ((char*)buffers[0].data)[0] = mIsLoad;
        return 1;
    }

    int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        // The guest is supposed to send us a confirm code first. The code is
        // 100 (4 byte integer).
        assert(buffers[0].size >= 4);
        int32_t opCode = *((int32_t*)buffers[0].data);
        switch (static_cast<OP>(opCode)) {
            case OP::Save:
                android::base::ThreadLooper::runOnMainLooper([](){
                        printf("Test snapshot save triggered\n");
                        androidSnapshot_save("test_snapshot");
                        printf("Test snapshot save done\n");
                    });
                break;
            case OP::Load:
                android::base::ThreadLooper::runOnMainLooper([](){
                        printf("Test snapshot load triggered\n");
                        androidSnapshot_load("test_snapshot");
                        printf("Test snapshot load done\n");
                    });
                break;
            default:
                fprintf(stderr, "Error: received bad command from snapshot"
                        " pipe\n");
                assert(0);
        }
        return sizeof(OP);
    }

    unsigned onGuestPoll() const override {
        // TODO
        return PIPE_POLL_IN | PIPE_POLL_OUT;
    }

    void onGuestClose(PipeCloseReason reason) override {}
    void onGuestWantWakeOn(int flags) override {}
private:
    enum class OP : int32_t {
        Save = 0, Load = 1
    };
    static bool mIsLoad;
};

bool SnapshotPipe::mIsLoad = false;

}

namespace android {
namespace snapshot {
void registerSnapshotPipeService() {
    android::AndroidPipe::Service::add(new SnapshotPipe::Service());
}
}
}
