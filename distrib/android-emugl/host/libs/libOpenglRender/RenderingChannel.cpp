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
#include "RenderingChannel.h"

#include <algorithm>
#include <utility>

namespace emugl {

RenderingChannel::RenderingChannel(std::shared_ptr<Renderer> renderer)
    : mRenderer(renderer) {}

RenderingChannel::~RenderingChannel() = default;

void RenderingChannel::setEventCallback(
        IRenderingChannel::EventCallback callback) {
    mOnEvent = std::move(callback);

    // Reset the current events state to make sure the new subscriber gets the
    // existing state
    mCurrentEvents = Event::Empty;
    onEvent();
}

bool RenderingChannel::write(ChannelBuffer&& buffer) {
    mFromGuest.send(std::move(buffer));
    onEvent();
    return true;
}

ChannelBuffer RenderingChannel::read() {
    ChannelBuffer buf = mToGuest.receive();
    onEvent();
    return buf;
}

void RenderingChannel::stop() {
    if (mStopped) {
        return;
    }
    mStopped = true;
    mFromGuest.stop();
    mToGuest.stop();
    onEvent();
}

bool RenderingChannel::writeToGuest(const ChannelBuffer::value_type* buf,
                                    size_t size) {
    ChannelBuffer buffer(buf, buf + size);
    mToGuest.send(std::move(buffer));
    onEvent();
    return true;
}

size_t RenderingChannel::readFromGuest(ChannelBuffer::value_type* buf,
                                       size_t size, bool blocking) {
    if (!buf || size == 0) {
        return 0;
    }

    size_t read = 0;
    const auto bufEnd = buf + size;
    while (buf != bufEnd && !mStopped) {
        if (mFromGuestBufferStart == mFromGuestBuffer.size()) {
            if (mFromGuest.size() == 0 && (read > 0 || !blocking)) {
                return read;
            }
            mFromGuestBuffer = mFromGuest.receive();
            mFromGuestBufferStart = 0;
        }

        const size_t curSize = std::min<size_t>(
                bufEnd - buf, mFromGuestBuffer.size() - mFromGuestBufferStart);
        memcpy(buf, mFromGuestBuffer.data() + mFromGuestBufferStart, curSize);

        read += curSize;
        buf += curSize;
        mFromGuestBufferStart += curSize;
    }
    onEvent();
    return read;
}

void RenderingChannel::onEvent() {
    if (!mOnEvent) {
        return;
    }
    Event newEvent = calcEvents();
    if (newEvent != mCurrentEvents) {
        mCurrentEvents = newEvent;
        mOnEvent(newEvent);
    }
}

IRenderingChannel::Event RenderingChannel::calcEvents() const {
    Event event = Event::Empty;
    if (mStopped) {
        event = Event::Stopped;
    } else {
        if (mFromGuest.size() < mFromGuest.capacity()) {
            event |= Event::Write;
        }
        if (mToGuest.size() > 0) {
            event |= Event::Read;
        }
    }
    return event;
}

}  // namespace emugl
