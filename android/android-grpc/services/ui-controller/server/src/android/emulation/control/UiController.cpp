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

#include <grpcpp/grpcpp.h>  // for Status, ServerCo...
#include <functional>       // for __base

#include "aemu/base/async/ThreadLooper.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"
#include "host-common/qt_ui_defs.h"
#include "host-common/window_agent.h"
#include "android/emulation/control/ServiceUtils.h"
#include "ui_controller_service.grpc.pb.h"
#include "ui_controller_service.pb.h"

namespace google::protobuf {
class Empty;
}  // namespace google::protobuf

using android::emulation::control::getUserConfig;
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
    explicit UiControllerImpl(const AndroidConsoleAgents* agents)
            : mAgents(agents) {
        mAgents->emu->initUI();
    }

    Status showExtendedControls(ServerContext* /*context*/,
                                const PaneEntry* paneEntry,
                                ExtendedControlsStatus* reply) override {
        const auto* agent = mAgents->emu;

        if (paneEntry->has_position()) {
            auto position = paneEntry->position();
            HorizontalAnchor hAnchor = HorizontalAnchor::LEFT;
            VerticalAnchor vAnchor = VerticalAnchor::TOP;
            if (position.horizontalanchor() == WindowPosition::HCENTER) {
                hAnchor = HorizontalAnchor::HCENTER;
            } else if (position.horizontalanchor() == WindowPosition::RIGHT) {
                hAnchor = HorizontalAnchor::RIGHT;
            }

            if (position.verticalanchor() == WindowPosition::VCENTER) {
                vAnchor = VerticalAnchor::VCENTER;
            } else if (position.verticalanchor() == WindowPosition::BOTTOM) {
                vAnchor = VerticalAnchor::BOTTOM;
            }
            agent->moveExtendedWindow(position.x(), position.y(), hAnchor,
                                      vAnchor);
        }
        bool ret = agent->startExtendedWindow(
                convertToExtendedWindowPane(paneEntry->index()));
        reply->set_visibilitychanged(ret);

        agent->waitForExtendedWindowVisibility(true);
        android::base::System::get()->sleepMs(500);
        return Status::OK;
    }

    Status closeExtendedControls(ServerContext* /*context*/,
                                 const Empty* /*request*/,
                                 ExtendedControlsStatus* reply) override {
        const auto* agent = mAgents->emu;
        bool ret = agent->quitExtendedWindow();
        reply->set_visibilitychanged(ret);
        agent->waitForExtendedWindowVisibility(false);
        return Status::OK;
    }

    Status setUiTheme(ServerContext* context,
                      const ThemingStyle* request,
                      Empty* reply) override {
        const auto* agent = mAgents->emu;
        android::base::ThreadLooper::runOnMainLooper(
                [agent, style = request->style()]() {
                    switch (style) {
                        case ThemingStyle::LIGHT:
                            agent->setUiTheme(SETTINGS_THEME_STUDIO_LIGHT);
                            break;
                        case ThemingStyle::CONTRAST:
                            agent->setUiTheme(SETTINGS_THEME_STUDIO_CONTRAST);
                            break;
                        case ThemingStyle::DARK:
                            agent->setUiTheme(SETTINGS_THEME_STUDIO_DARK);
                            break;
                        default:
                            derror("setUiTheme has been called with an unknown studio theme, ignoring.");
                    }
                });
        return Status::OK;
    }

    Status getUserConfig(ServerContext* context,
                         const Empty* request,
                         UserConfig* reply) override {
        auto userEntries = reply->mutable_entries();
        auto userCnf = ::getUserConfig();
        for (auto const& entry : userCnf) {
            auto response_entry = userEntries->Add();
            response_entry->set_key(entry.first);
            response_entry->set_value(entry.second);
        };
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
