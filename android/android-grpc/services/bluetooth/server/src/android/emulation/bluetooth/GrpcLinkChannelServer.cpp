// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use connection file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/emulation/bluetooth/GrpcLinkChannelServer.h"

#include <cassert>
#include <functional>

#include "aemu/base/async/Looper.h"
#include "android/emulation/bluetooth/BufferedAsyncDataChannelAdapter.h"

#define DEBUG 1
/* set  for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace emulation {
namespace bluetooth {
GrpcLinkChannelServer::GrpcLinkChannelServer(base::Looper* looper,
                                             size_t buffer)
    : mLooper(looper), mBufferSize(buffer) {
    DD("Created link server %p", this);
}

GrpcLinkChannelServer::~GrpcLinkChannelServer() {
    DD("Deleted linkchannel server: %p", this);
}

bool GrpcLinkChannelServer::StartListening() {
    mListening = true;
    return true;
}

void GrpcLinkChannelServer::StopListening() {
    mListening = false;
}

void GrpcLinkChannelServer::Close() {
    mOpen = false;
}

bool GrpcLinkChannelServer::Connected() {
    return true;
}

std::shared_ptr<BufferedAsyncDataChannel>
GrpcLinkChannelServer::createConnection() {
    DD("Creating connection on %p", this);
    assert(mOpen);
    auto channel =
            BufferedAsyncDataChannel::create(mBufferSize, mLooper);
    if (callback_ && mListening) {
        mLooper->scheduleCallback([this, channel] {
            DD("Invoking callback");
            callback_(channel, this);
        });
    }

    DD("Created connection: %p", channel.get());
    return channel;
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
