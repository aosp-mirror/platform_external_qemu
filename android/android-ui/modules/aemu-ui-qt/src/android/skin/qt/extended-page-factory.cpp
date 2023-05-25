/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "android/skin/qt/extended-page-factory.h"
#include "aemu/base/LayoutResolver.h"           // for resolveLayout
#include "aemu/base/Log.h"                      // for LogStreamVoi...
#include "aemu/base/files/Stream.h"             // for Stream
#include "aemu/base/files/StreamSerializing.h"  // for loadCollection
#include "android/avd/info.h"
#include "android/cmdline-option.h"  // for android_cmdL...
#include "android/console.h"
#include "android/console.h"                             // for android_hw
#include "android/emulation/AutoDisplays.h"              // For AutoDisplays...
#include "android/emulation/control/adb/AdbInterface.h"  // for AdbInterface
#include "android/emulation/resizable_display_config.h"
#include "android/emulator-window.h"  // for emulator_win...
#include "android/hw-sensors.h"       // for android_fold...
#include "android/skin/file.h"        // for SkinLayout
#include "android/skin/qt/extended-pages/snapshot-page-grpc.h"
#include "android/skin/rect.h"             // for SKIN_ROTATION_0
#include "host-common/FeatureControl.h"    // for isEnabled
#include "host-common/Features.h"          // for MultiDisplay
#include "host-common/MultiDisplayPipe.h"  // for MultiDisplay...
#include "host-common/hw-config.h"
#include "host-common/screen-recorder.h"  // for RecorderStates
#include "ui_extended.h"

void ExtendedPageFactory::construct(Ui::ExtendedControls* ui,
                                    ExtendedWindowPane window)

{
    bool grpc = getConsoleAgents()->settings->android_cmdLineOptions()->grpc_ui;

    switch (window) {
        case PANE_IDX_SNAPSHOT: {
            if (grpc) {
                auto snapshotPage =
                        new android::emulation::grpc::ui::SnapshotPageGrpc();

                ui->snapshotPage = replaceWidget(
                        ui->stackedWidget, ui->snapshotPage, snapshotPage);
                snapshotPage->enableConnections();
            } else {
                auto snapshotPage = new SnapshotPage();
                ui->snapshotPage = replaceWidget(
                        ui->stackedWidget, ui->snapshotPage, snapshotPage);
            }
            break;
        }
        default:
            // Nothing
            break;
    }
}

QWidget* ExtendedPageFactory::replaceWidget(QStackedWidget* stackedWidget,
                                            QWidget* page,
                                            QWidget* replacement) {
    // Get the index of the location page widget in the stacked widget
    int pageIndex = stackedWidget->indexOf(page);

    // Replace the existing location page widget with the new instance
    stackedWidget->removeWidget(page);
    stackedWidget->insertWidget(pageIndex, replacement);
    return replacement;
}