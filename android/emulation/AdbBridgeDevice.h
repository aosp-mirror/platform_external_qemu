// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/async/AsyncSocketServer.h"
#include "android/base/async/Looper.h"
#include "android/emulation/AdbPipe.h"

#include <memory>
#include <vector>

namespace android {
namespace emulation {

class IAdbBridgeDevice {
public:
    IAdbBridgeDevice(android::base::Looper* looper,
                     int adbServerPort,
                     IAdbPipeFactory* adbPipeFactory)
        : mLooper(looper),
          mAdbServerPort(adbServerPort),
          mAdbPipeFactory(adbPipeFactory) {}
    virtual ~IAdbBridgeDevice(){};

    // Initialize this object, start briding ADB communication. The bridge
    // listens to incoming connection from adb server on |bridgePort|.
    virtual bool init(int bridgePort) = 0;
    // Cleanup this object so that it can be re-init'ed on a different
    // bridgePort.
    virtual void deInit() = 0;

    // Called by Adb pipe to notify us that that it's connection has been
    // disconnected completely.
    virtual void notifyPipeDisconnection(IAdbPipe* pipe) = 0;

    // AndroidPipeFuncs API to create a new ADB Pipe.
    virtual IAdbPipe* pipeInit(void* hwPipe, const char* args) = 0;

    // Static functions for AndriodPipeFuns C API.
    static void* onPipeInit(void* hwPipe, void* opaque, const char* args) {
        IAdbBridgeDevice* device = static_cast<IAdbBridgeDevice*>(opaque);
        return device->pipeInit(hwPipe, args);
    }

protected:
    android::base::Looper* const mLooper;
    int mAdbServerPort;
    std::unique_ptr<IAdbPipeFactory> mAdbPipeFactory;
};

class AdbBridgeDevice : public IAdbBridgeDevice {
public:
    AdbBridgeDevice(android::base::Looper* looper,
                    int adbServerPort,
                    IAdbPipeFactory* adbPipeFactory = new AdbPipeFactory())
        : IAdbBridgeDevice(looper, adbServerPort, adbPipeFactory) {}

    bool init(int bridgePort) override;
    void deInit() override;
    IAdbPipe* pipeInit(void* hwPipe, const char* args) override;

    void notifyPipeDisconnection(IAdbPipe* pipe) override;

private:
    static const size_t kMaxAdbPipes;

    bool onHostServerConnected(int port);
    bool notifyHostServer();

    int mBridgePort = -1;
    std::unique_ptr<android::base::AsyncSocketServer> mServer;
    std::vector<std::unique_ptr<IAdbPipe>> mAdbPipes;
};

}  // namespace emulation
}  // namespace android
