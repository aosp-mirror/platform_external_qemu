
// Copyright (C) 2021 The Android Open Source Project
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

#include <memory>  // for shared_ptr

#include "net/async_data_channel_server.h"  // for AsyncDataChannelServer

namespace android {
namespace base {
class Looper;
}  // namespace base

namespace net {
class AsyncDataChannel;

// A Datachannel server that will provide a qemu connection if it is being
// injected
class HciDataChannelServer : public AsyncDataChannelServer {
public:
    HciDataChannelServer(base::Looper* looper);

    bool StartListening() override;

    void StopListening() override;

    // The qemu channel server cannot be closed.
    void Close() override{};

    // The qemu connection is always open.
    bool Connected() { return true; }

    // Injects a new qemu channel, returning the newly created channel.
    // This re-initializes the channel to /dev/vhci in qemu.
    std::shared_ptr<AsyncDataChannel> injectQemuChannel();

    // The currently active data channel to /dev/vhci in qemu
    std::shared_ptr<AsyncDataChannel> qemuChannel();

private:
    std::shared_ptr<AsyncDataChannel> mQemuChannel;
    bool mListening{true};
    base::Looper* mLooper;
};

}  // namespace net
}  // namespace android
