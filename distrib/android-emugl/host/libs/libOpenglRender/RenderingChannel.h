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

#include "OpenglRender/IRenderingChannel.h"
#include "Renderer.h"

#include "android/base/Compiler.h"
#include "android/base/synchronization/MessageChannel.h"

#include <memory>

namespace emugl {

class RenderingChannel final : public IRenderingChannel {
public:
    RenderingChannel(std::shared_ptr<Renderer> renderer);
    ~RenderingChannel();

public:
    // IRenderingChannel implementation, operations provided for a guest system
    virtual void setEventCallback(EventCallback callback) override final;

    virtual bool write(ChannelBuffer&& buffer) override final;
    virtual ChannelBuffer read(bool blocking) override final;

    virtual void stop() override final;

    virtual bool isStopped() const override final;

public:
    void writeToGuest(ChannelBuffer&& buf);
    size_t readFromGuest(ChannelBuffer::value_type* buf, size_t size, bool blocking);

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(RenderingChannel);

private:
    void onEvent(bool lock);
    Event calcEvents() const;

    static const size_t kChannelCapacity = 1024;

    std::shared_ptr<Renderer> mRenderer;
    EventCallback mOnEvent;
    Event mCurrentEvents = Event::Empty;
    bool mStopped = false;

    android::base::MessageChannel<ChannelBuffer, kChannelCapacity> mFromGuest;
    android::base::MessageChannel<ChannelBuffer, kChannelCapacity> mToGuest;

    ChannelBuffer mFromGuestBuffer;
    size_t mFromGuestBufferLeft = 0;
};

}  // namespace emugl
