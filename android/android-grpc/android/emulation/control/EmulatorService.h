// Copyright (C) 2018 The Android Open Source Project
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
#include <memory>  // for unique_ptr, shared_ptr
#include <string>  // for string

#include "android/console.h"  // for AndroidConsoleAgents
#include "android/emulation/control/waterfall/WaterfallService.h"
#include "grpcpp/security/server_credentials.h"  // for ServerCredentials

namespace android {
namespace emulation {
namespace control {
class RtcBridge;

class EmulatorControllerService {
public:
    class Builder;

    virtual ~EmulatorControllerService() {}
    virtual void stop() = 0;
    virtual int port() = 0;
    virtual std::string publicCert() = 0;
};

class EmulatorControllerService::Builder {
public:
    Builder();
    Builder& withConsoleAgents(const AndroidConsoleAgents* const consoleAgents);
    Builder& withCertAndKey(std::string certfile, std::string privateKeyFile);

    Builder& withPort(int port);
    Builder& withRtcBridge(RtcBridge* bridge);
    Builder& withWaterfall(const char* mode);

    // Returns the fully configured and running service, or nullptr if
    // construction failed. Once the service has been started
    // the grpc configuration will be written to disk.
    std::unique_ptr<EmulatorControllerService> build();
    int port();

private:
    const AndroidConsoleAgents* mAgents;
    int mPort{5556};
    WaterfallProvider mWaterfall{WaterfallProvider::adb};
    std::shared_ptr<grpc::ServerCredentials> mCredentials;
    std::string mBindAddress{"0.0.0.0"};
    std::string mCertfile;

    RtcBridge* mBridge{nullptr};
};

}  // namespace control
}  // namespace emulation
}  // namespace android
