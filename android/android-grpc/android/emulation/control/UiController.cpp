// Copyright (C) 2020 The Android Open Source Project
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

#include "android/emulation/control/UiController.h"

#include <grpcpp/grpcpp.h>

#include "android/base/async/ThreadLooper.h"
#include "android/settings-agent.h"

#include "ui_controller_service.grpc.pb.h"
#include "ui_controller_service.pb.h"

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google

using namespace android::base;
using ::google::protobuf::Empty;
using grpc::ServerContext;
using grpc::Status;

namespace android {
namespace emulation {
namespace control {

class UiControllerImpl final : public UiController::Service {
public:
    UiControllerImpl(const AndroidConsoleAgents* agents)
        : mAgents(agents) {}

    Status showExtendedControls(ServerContext* context,
                                const Empty* request,
                                Empty* reply) {
        auto agent = mAgents->emu;
        android::base::ThreadLooper::runOnMainLooper(
                [agent]() { agent->startExtendedWindow(); });
        return Status::OK;
    }

    Status closeExtendedControls(ServerContext* context,
                                 const Empty* request,
                                 Empty* reply) {
        auto agent = mAgents->emu;
        android::base::ThreadLooper::runOnMainLooper(
                [agent]() { agent->quitExtendedWindow(); });
        return Status::OK;
    }

    Status setUiTheme(ServerContext* context,
                      const ThemingStyle* request,
                      Empty* reply) {
        auto agent = mAgents->emu;
        android::base::ThreadLooper::runOnMainLooper([agent, request]() {
            if (request->style() == ThemingStyle::LIGHT)
                agent->setUITheme(SETTINGS_THEME_LIGHT);
            else if (request->style() == ThemingStyle::DARK)
                agent->setUITheme(SETTINGS_THEME_DARK);
        });
        return Status::OK;
    }

private:
    const AndroidConsoleAgents* mAgents;
};

grpc::Service* getUiControllerService(const AndroidConsoleAgents* agents) {
    return new UiControllerImpl(agents);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
