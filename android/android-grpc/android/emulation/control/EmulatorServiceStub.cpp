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


#include "android/emulation/control/EmulatorService.h"

namespace grpc {
class ServerCredentials {};
}

namespace android {
namespace emulation {
namespace control {

class NopEmulatorControllerServiceImpl : public EmulatorControllerService {
public:
    void stop() override {}

private:
};

using Builder = EmulatorControllerService::Builder;

Builder::Builder() {}

Builder& Builder::withConsoleAgents(
        const AndroidConsoleAgents* const consoleAgents) {
    return *this;
}
Builder& Builder::withCredentials(
        std::shared_ptr<grpc::ServerCredentials> credentials) {
    return *this;
}

Builder& Builder::withPort(int port) {
    return *this;
}

std::unique_ptr<EmulatorControllerService> Builder::build() {
    return std::unique_ptr<EmulatorControllerService>(new NopEmulatorControllerServiceImpl());
}
}
}
}  // namespace android
