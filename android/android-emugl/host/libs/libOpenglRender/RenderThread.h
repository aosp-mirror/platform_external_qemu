/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include "android/base/files/MemStream.h"
#include "android/base/Optional.h"
#include "android/emulation/address_space_graphics_types.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "emugl/common/mutex.h"
#include "emugl/common/thread.h"

#include <atomic>
#include <memory>

namespace emugl {

class RenderChannelImpl;
class RendererImpl;
class ReadBuffer;
class RingStream;

// A class used to model a thread of the RenderServer. Each one of them
// handles a single guest client / protocol byte stream.
class RenderThread : public emugl::Thread {
    using MemStream = android::base::MemStream;

public:
    // Create a new RenderThread instance.
    RenderThread(RenderChannelImpl* channel,
                 android::base::Stream* loadStream = nullptr);

    // Create a new RenderThread instance tied to the address space device.
    RenderThread(
        struct asg_context context,
        android::base::Stream* loadStream,
        android::emulation::asg::ConsumerCallbacks callbacks);
    virtual ~RenderThread();

    // Returns true iff the thread has finished.
    bool isFinished() const { return mFinished.load(std::memory_order_relaxed); }

    void pausePreSnapshot();
    void resume();
    void save(android::base::Stream* stream);

    // Calls |mOnTeardown| callback to notify the main() thread function
    // the thread is going to be torn down.
    void notifyTeardown();

private:
    intptr_t main() override;
    void setFinished();
    void setOnTeardownLocked(std::function<void()> callback = []{}) {
        mOnTeardown = std::move(callback);
    }

    // Snapshot support.
    enum class SnapshotState {
        Empty,
        StartSaving,
        StartLoading,
        InProgress,
        Finished,
    };

    // Whether using RenderChannel or a ring buffer.
    enum TransportMode {
        Channel,
        Ring,
    };

    template <class OpImpl>
    void snapshotOperation(android::base::AutoLock* lock, OpImpl&& impl);

    struct SnapshotObjects;

    bool doSnapshotOperation(const SnapshotObjects& objects,
                             SnapshotState operation);
    void waitForSnapshotCompletion(android::base::AutoLock* lock);
    void loadImpl(android::base::AutoLock* lock, const SnapshotObjects& objects);
    void saveImpl(android::base::AutoLock* lock, const SnapshotObjects& objects);

    bool isPausedForSnapshotLocked() const;

    RenderChannelImpl* mChannel = nullptr;
    std::unique_ptr<RingStream> mRingStream;
    TransportMode mTransportMode = TransportMode::Channel;

    SnapshotState mState = SnapshotState::Empty;
    std::atomic<bool> mFinished { false };
    android::base::Lock mLock;
    android::base::ConditionVariable mCondVar;
    android::base::Optional<android::base::MemStream> mStream;

    bool mRunInLimitedMode = false;

    std::function<void()> mOnTeardown = []{};
};

}  // namespace emugl
