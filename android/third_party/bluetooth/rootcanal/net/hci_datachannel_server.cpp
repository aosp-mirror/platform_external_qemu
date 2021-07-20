// Copyright 2021 The Android Open Source Project
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

#include "net/hci_datachannel_server.h"

#include <functional>                   // for __base, function

#include "android/base/async/Looper.h"  // for Looper
#include "android/console.h"            // for getConsoleAgents, AndroidCons...
#include "net/qemu_datachannel.h"       // for QemuDataChannel, Looper

namespace android {
namespace net {
HciDataChannelServer::HciDataChannelServer(base::Looper* looper)
    : mLooper(looper) {}

bool HciDataChannelServer::StartListening() {
    if (!mQemuChannel && callback_) {
        mQemuChannel =
                std::make_shared<QemuDataChannel>(getConsoleAgents()->rootcanal,
                                                  mLooper);
        mLooper->scheduleCallback(
                [&]() { callback_(mQemuChannel, this); });
    }

    return true;
}

}  // namespace net
}  // namespace android