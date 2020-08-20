// Copyright (C) 2019 The Android Open Source Project
// Copyright (C) 2019 Google Inc.
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
#include "android/base/Tracing.h"

#include "android/base/containers/BufferQueue.h"
#include "android/base/containers/SmallVector.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/base/threads/Thread.h"
#include "android/base/threads/ThreadStore.h"

#include "perfetto-tracing-only.h"

#include <string>
#include <vector>

#include <fcntl.h>

namespace android {
namespace base {

using TraceSinkBuffer = SmallFixedVector<uint8_t, 4096>;

class MultithreadedTraceSink {
public:
    MultithreadedTraceSink(const char* filename) :
        mFilename(filename),
        mFileHandle(fopen(filename, "wb")),
        mToSink(1024, mLock), mThread([this] { threadFunction(); }) {
        mThread.start();
    }

    ~MultithreadedTraceSink() {
        mExiting = true;
        mThread.wait();
        fclose(mFileHandle);
    }

    void push(uint8_t* src, size_t bytes) {
        TraceSinkBuffer res(src, src + bytes);
        AutoLock lock(mLock);
        mToSink.pushLocked(std::move(res));
    }

private:
    void threadFunction() {
        const uint64_t kBlockIntervalUs = 1000000;
        TraceSinkBuffer currentBufferToWrite;

        while (!mExiting || mToSink.canPopLocked()) {
            auto currTime = System::get()->getUnixTimeUs();
            AutoLock lock(mLock);
            auto result = mToSink.popLockedBefore(&currentBufferToWrite, currTime + kBlockIntervalUs);
            if (result == BufferQueueResult::Ok) {
                fwrite(currentBufferToWrite.data(), currentBufferToWrite.size(), 1, mFileHandle);
            }
        }
    }

    mutable Lock mLock;
    const char* mFilename;
    FILE* mFileHandle;
    BufferQueue<TraceSinkBuffer> mToSink;
    bool mExiting = false;
    FunctorThread mThread;
};

static MultithreadedTraceSink* sTraceSink = nullptr;

static perfetto::TraceWriterOperations sAndroidEmuTraceWriterOperations = {
    .openSink = []() {
        sTraceSink = new MultithreadedTraceSink(perfetto::queryTraceConfig().filename);
    },
    .closeSink = []() {
        delete sTraceSink;
    },
    .writeToSink = [](uint8_t* src, size_t bytes) {
        sTraceSink->push(src, bytes);
    },
};

void setGuestTime(uint64_t t) {
    perfetto::setTraceConfig([t](perfetto::TraceConfig& config) {
        config.guestStartTime = t;
        config.useGuestStartTime = true;
    });
}

const bool* tracingDisabledPtr = nullptr;

void initializeTracing() {
    perfetto::initialize(&tracingDisabledPtr);
}

void enableTracing() {
    if (perfetto::queryTraceConfig().tracingDisabled) {
        perfetto::setTraceConfig([](perfetto::TraceConfig& config) {
            config.ops = sAndroidEmuTraceWriterOperations;
        });
        perfetto::enableTracing();
    }
}

void disableTracing() {
    if (!perfetto::queryTraceConfig().tracingDisabled) {
        perfetto::disableTracing();
    }
}

bool shouldEnableTracing() {
    return !(perfetto::queryTraceConfig().tracingDisabled);
}

#ifdef __cplusplus
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))
#else
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), 1 ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), 0 ))
#endif

__attribute__((always_inline)) void beginTrace(const char* name) {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    perfetto::beginTrace(name);
}

__attribute__((always_inline)) void endTrace() {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    perfetto::endTrace();
}

__attribute__((always_inline)) void traceCounter(const char* name, int64_t value) {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    perfetto::traceCounter(name, value);
}

ScopedTrace::ScopedTrace(const char* name) {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    perfetto::beginTrace(name);
}

ScopedTrace::~ScopedTrace() {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    perfetto::endTrace();
}

} // namespace base
} // namespace android
