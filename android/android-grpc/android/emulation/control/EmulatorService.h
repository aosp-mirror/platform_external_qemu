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

#include <memory>
#include "android/console.h"
#include "android/emulation/control/RtcBridge.h"
namespace grpc {
class ServerCredentials;
}

namespace android {
namespace emulation {
namespace control {
class EmulatorControllerService {
public:
    class Builder;
    virtual ~EmulatorControllerService() {}
    virtual void stop() = 0;
};

class EmulatorControllerService::Builder {
public:
    Builder();
    Builder& withConsoleAgents(const AndroidConsoleAgents* const consoleAgents);
    Builder& withCredentials(
            std::shared_ptr<grpc::ServerCredentials> credentials);
    Builder& withPort(int port);
    Builder& withRtcBridge(RtcBridge* bridge);

    // Returns the fully configured and running service, or nullptr if
    // construction failed.
    std::unique_ptr<EmulatorControllerService> build();

private:
    const AndroidConsoleAgents* mAgents;
    int mPort{5556};
    std::shared_ptr<grpc::ServerCredentials> mCredentials;

    RtcBridge* mBridge{nullptr};
};

}  // namespace control
}  // namespace emulation
}  // namespace android
