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
#include "RenderChannelImpl.h"

#include <algorithm>
#include <utility>

#include <assert.h>

namespace emugl {

RenderChannelImpl::RenderChannelImpl(std::shared_ptr<RendererImpl> renderer)
    : mRenderer(renderer) {}

void RenderChannelImpl::setEventCallback(
        RenderChannelImpl::EventCallback callback) {
    mOnEvent = std::move(callback);

    // Reset the current state to make sure the new subscriber gets it.
    mState = State::Empty;
    onEvent(true);
}

bool RenderChannelImpl::write(ChannelBuffer&& buffer) {
    mFromGuest.send(std::move(buffer));
    onEvent(true);
    return true;
}

ChannelBuffer RenderChannelImpl::read(bool blocking) {
    if (!blocking && mToGuest.size() == 0) {
        return {};
    }
    ChannelBuffer buf = mToGuest.receive();
    onEvent(true);
    return buf;
}

void RenderChannelImpl::stop() {
    stop(true);
}

void RenderChannelImpl::forceStop() {
    stop(false);
}

void RenderChannelImpl::stop(bool byGuest) {
    if (mStopped) {
        return;
    }
    mStopped = true;
    mFromGuest.stop();
    mToGuest.stop();
    onEvent(byGuest);
}

bool RenderChannelImpl::isStopped() const {
    return mStopped;
}

void RenderChannelImpl::writeToGuest(ChannelBuffer&& buf) {
    mToGuest.send(std::move(buf));
    onEvent(false);
}

size_t RenderChannelImpl::readFromGuest(ChannelBuffer::value_type* buf,
                                        size_t size,
                                        bool blocking) {
    assert(buf);
    if (size == 0) {
        return 0;
    }

    size_t read = 0;
    const auto bufEnd = buf + size;
    while (buf != bufEnd && !mStopped) {
        if (mFromGuestBufferLeft == 0) {
            if (mFromGuest.size() == 0 && (read > 0 || !blocking)) {
                break;
            }
            mFromGuestBuffer = mFromGuest.receive();
            mFromGuestBufferLeft = mFromGuestBuffer.size();
        }

        const size_t curSize =
                std::min<size_t>(bufEnd - buf, mFromGuestBufferLeft);
        memcpy(buf, mFromGuestBuffer.data() +
                            (mFromGuestBuffer.size() - mFromGuestBufferLeft),
               curSize);

        read += curSize;
        buf += curSize;
        mFromGuestBufferLeft -= curSize;
    }
    onEvent(false);
    return read;
}

void RenderChannelImpl::onEvent(bool byGuest) {
    if (!mOnEvent) {
        return;
    }
    const State newState = calcState();
    if (newState != mState) {
        mState = newState;
        mOnEvent(newState,
                 byGuest ? EventSource::Client : EventSource::RenderChannel);
    }
}

RenderChannel::State RenderChannelImpl::calcState() const {
    State state = State::Empty;
    if (mStopped) {
        state = State::Stopped;
    } else {
        if (mFromGuest.size() < mFromGuest.capacity()) {
            state |= State::CanWrite;
        }
        if (mToGuest.size() > 0) {
            state |= State::CanRead;
        }
    }
    return state;
}

}  // namespace emugl
