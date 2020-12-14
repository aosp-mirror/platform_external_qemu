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
static ExtendedWindowPane convertToExtendedWindowPane(
        PaneEntry::PaneIndex index) {
    switch (index) {
        case PaneEntry::LOCATION:
            return PANE_IDX_LOCATION;
        case PaneEntry::MULTIDISPLAY:
            return PANE_IDX_MULTIDISPLAY;
        case PaneEntry::CELLULAR:
            return PANE_IDX_CELLULAR;
        case PaneEntry::BATTERY:
            return PANE_IDX_BATTERY;
        case PaneEntry::CAMERA:
            return PANE_IDX_CAMERA;
        case PaneEntry::TELEPHONE:
            return PANE_IDX_TELEPHONE;
        case PaneEntry::DPAD:
            return PANE_IDX_DPAD;
        case PaneEntry::TV_REMOTE:
            return PANE_IDX_TV_REMOTE;
        case PaneEntry::ROTARY:
            return PANE_IDX_ROTARY;
        case PaneEntry::MICROPHONE:
            return PANE_IDX_MICROPHONE;
        case PaneEntry::FINGER:
            return PANE_IDX_FINGER;
        case PaneEntry::VIRT_SENSORS:
            return PANE_IDX_VIRT_SENSORS;
        case PaneEntry::SNAPSHOT:
            return PANE_IDX_SNAPSHOT;
        case PaneEntry::BUGREPORT:
            return PANE_IDX_BUGREPORT;
        case PaneEntry::RECORD:
            return PANE_IDX_RECORD;
        case PaneEntry::GOOGLE_PLAY:
            return PANE_IDX_GOOGLE_PLAY;
        case PaneEntry::SETTINGS:
            return PANE_IDX_SETTINGS;
        case PaneEntry::HELP:
            return PANE_IDX_HELP;
        case PaneEntry::CAR:
            return PANE_IDX_CAR;
        case PaneEntry::CAR_ROTARY:
            return PANE_IDX_CAR_ROTARY;
        case PaneEntry::SENSOR_REPLAY:
            return PANE_IDX_SENSOR_REPLAY;
        default:
            return PANE_IDX_UNKNOWN;
    }
}

class UiControllerImpl final : public UiController::Service {
public:
    UiControllerImpl(const AndroidConsoleAgents* agents)
        : mAgents(agents) {}

    Status showExtendedControls(ServerContext* context,
                                const PaneEntry* paneEntry,
                                ExtendedControlsStatus* reply) {
        auto agent = mAgents->emu;
        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [agent, reply, paneEntry]() {
                    bool ret = agent->startExtendedWindow(
                            convertToExtendedWindowPane(paneEntry->index()));
                    reply->set_visibilitychanged(ret);
                });
        return Status::OK;
    }

    Status closeExtendedControls(ServerContext* context,
                                 const Empty* request,
                                 ExtendedControlsStatus* reply) {
        auto agent = mAgents->emu;
        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [agent, reply]() {
                    bool ret = agent->quitExtendedWindow();
                    reply->set_visibilitychanged(ret);
                });
        return Status::OK;
    }

    Status setUiTheme(ServerContext* context,
                      const ThemingStyle* request,
                      Empty* reply) {
        auto agent = mAgents->emu;
        android::base::ThreadLooper::runOnMainLooper([agent, request]() {
            if (request->style() == ThemingStyle::LIGHT)
                agent->setUiTheme(SETTINGS_THEME_STUDIO_LIGHT);
            else if (request->style() == ThemingStyle::DARK)
                agent->setUiTheme(SETTINGS_THEME_STUDIO_DARK);
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
