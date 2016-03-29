// Copyright 2016 The Android Open Source Project
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

#include "android/emulation/VmLock.h"

namespace android {

namespace base {
class Looper;
}  // namespace base

// A convenience class used to communicate with the ADB server.
// This has two purposes.
//
// 1) Notify the ADB host server, if any, that this emulator instance has
//    started. This allows it to be listed in 'adb devices'.
//    See AdbServer::notifyServer()
//
// 2) Start listening on a host TCP localhost port for connections from the
//    ADB server, that will be proxyed by the class to the guest adbd through
//    a pipe implementation.
class AdbServer {
public:
    AdbServer() = default;

    virtual ~AdbServer() = default;

    virtual bool setup(int hostTcpPort, android::base::Looper* looper) = 0;

    static AdbServer* createFromPipe(VmLock* vmLock = nullptr);

    // Try to connect to the ADB host server that is supposed to be
    // listening on TCP loopback port |adbServerPort| and tell it that
    // this emulator instance is listening. Return true on success,
    // false/errno on failure.
    static bool notifyHostServer(int adbServerPort, int adbListenPort);
};

}  // namespace android
