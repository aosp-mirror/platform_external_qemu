// Copyright (C) 2016 The Android Open Source Project
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
#pragma once

#include "OpenglRender/RenderChannel.h"
#include "RendererImpl.h"

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"

#include <atomic>
#include <memory>

namespace emugl {

class RenderChannelImpl final : public RenderChannel {
public:
    RenderChannelImpl(std::shared_ptr<RendererImpl> renderer);

public:
    // RenderChannel implementation, operations provided for a guest system
    virtual void setEventCallback(EventCallback callback) override final;

    virtual bool write(Buffer&& buffer) override final;
    virtual bool read(Buffer* buffer, CallType type) override final;

    virtual State currentState() const override final {
        return mState.load(std::memory_order_acquire);
    }

    virtual void stop() override final;
    virtual bool isStopped() const override final;

public:
    // These functions are for the RenderThread, they could be called in
    // parallel with the ones from the RenderChannel interface. Make sure the
    // internal state remains consistent all the time.
    void writeToGuest(Buffer&& buf);
    size_t readFromGuest(Buffer::value_type* buf, size_t size,
                         bool blocking);
    void forceStop();

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(RenderChannelImpl);

private:
    void onEvent(bool byGuest);
    State calcState() const;
    void stop(bool byGuest);

private:
    std::shared_ptr<RendererImpl> mRenderer;

    EventCallback mOnEvent;

    // Protects the state recalculation and writes to mState.
    //
    // The correctness condition governing the relationship between mFromGuest,
    // mToGuest, and mState is that we can't reach a potentially stable state
    // (i.e., at the end of a set of invocations of publically-visible
    // operations) in which either:
    //  - mFromGuest().size() < mFromGuest.capacity(), yet state does not have
    //    State::CanWrite, or
    //  - mToGuest().size > 0, yet state does not have State::CanRead.
    // Clients assume they can poll currentState() and have the indicate whether
    // a write or read might possibly succeed, and this correctness condition
    // makes that assumption valid -- if a write or read might succeed,
    // mState is required to eventually indicate this.
    android::base::Lock mStateLock;
    std::atomic<State> mState;

    bool mStopped = false;

    android::base::MessageChannel<Buffer, 1024> mFromGuest;
    android::base::MessageChannel<Buffer, 16> mToGuest;

    Buffer mFromGuestBuffer;
    size_t mFromGuestBufferLeft = 0;
};

}  // namespace emugl
