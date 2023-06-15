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

#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/skin/qt/extended-pages/battery-page-grpc.h"
#include "android/skin/qt/extended-pages/battery-page.h"
#include "android/skin/qt/extended-pages/bug-report-page-grpc.h"
#include "android/skin/qt/extended-pages/bug-report-page.h"
#include "android/skin/qt/extended-pages/camera-page-grpc.h"
#include "android/skin/qt/extended-pages/camera-page.h"
#include "android/skin/qt/extended-pages/cellular-page-grpc.h"
#include "android/skin/qt/extended-pages/cellular-page.h"
#include "android/skin/qt/extended-pages/finger-page-grpc.h"
#include "android/skin/qt/extended-pages/finger-page.h"
#include "android/skin/qt/extended-pages/snapshot-page-grpc.h"
#include "android/skin/qt/extended-pages/snapshot-page.h"
#include "ui_extended.h"

#define DEBUG 2
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

using android::emulation::grpc::ui::BatteryPageGrpc;
using android::emulation::grpc::ui::BugreportPageGrpc;
using android::emulation::grpc::ui::SnapshotPageGrpc;
// using

QWidget* replaceWidget(QStackedWidget* stackedWidget,
                       QWidget* page,
                       QWidget* replacement) {
    // Get the index of the location page widget in the stacked widget
    int pageIndex = stackedWidget->indexOf(page);

    // Replace the existing location page widget with the new instance
    stackedWidget->removeWidget(page);
    stackedWidget->insertWidget(pageIndex, replacement);
    return replacement;
}

static void constructDefault(Ui::ExtendedControls* ui,
                             ExtendedWindowPane window) {
    switch (window) {
        case PANE_IDX_UNKNOWN:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_LOCATION:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_MULTIDISPLAY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_CELLULAR:
            ui->cellular_page = replaceWidget(
                    ui->stackedWidget, ui->cellular_page, new CellularPage());
            break;
        case PANE_IDX_BATTERY:
            ui->batteryPage = replaceWidget(ui->stackedWidget, ui->batteryPage,
                                            new BatteryPage());
            break;
        case PANE_IDX_CAMERA:
            ui->cameraPage = replaceWidget(ui->stackedWidget, ui->cameraPage,
                                           new CameraPage());
            break;
        case PANE_IDX_TELEPHONE:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_DPAD:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_TV_REMOTE:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_ROTARY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_MICROPHONE:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_FINGER:
            replaceWidget(ui->stackedWidget, ui->finger_page, new FingerPage());
            break;
        case PANE_IDX_VIRT_SENSORS:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_SNAPSHOT:
            ui->snapshotPage = replaceWidget(
                    ui->stackedWidget, ui->snapshotPage, new SnapshotPage());
            break;
        case PANE_IDX_BUGREPORT:
            ui->bugreportPage = replaceWidget(
                    ui->stackedWidget, ui->bugreportPage, new BugreportPage());
            break;
        case PANE_IDX_RECORD:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_GOOGLE_PLAY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_SETTINGS:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_HELP:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_CAR:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_CAR_ROTARY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_SENSOR_REPLAY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
    }
}

static void constructGrpc(Ui::ExtendedControls* ui, ExtendedWindowPane window) {
    switch (window) {
        case PANE_IDX_UNKNOWN:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_LOCATION:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_MULTIDISPLAY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_CELLULAR:
            ui->cellular_page =
                    replaceWidget(ui->stackedWidget, ui->cellular_page,
                                  new CellularPageGrpc());
            break;
        case PANE_IDX_BATTERY:
            ui->batteryPage = replaceWidget(ui->stackedWidget, ui->batteryPage,
                                            new BatteryPageGrpc());
            break;
        case PANE_IDX_CAMERA:
            ui->cameraPage = replaceWidget(ui->stackedWidget, ui->cameraPage,
                                           new CameraPageGrpc());
            break;
        case PANE_IDX_TELEPHONE:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_DPAD:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_TV_REMOTE:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_ROTARY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_MICROPHONE:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_FINGER:
            replaceWidget(ui->stackedWidget, ui->finger_page, new FingerPageGrpc());
            break;
        case PANE_IDX_VIRT_SENSORS:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_SNAPSHOT:
            ui->snapshotPage =
                    replaceWidget(ui->stackedWidget, ui->snapshotPage,
                                  new SnapshotPageGrpc());
            break;
        case PANE_IDX_BUGREPORT:
            ui->bugreportPage =
                    replaceWidget(ui->stackedWidget, ui->bugreportPage,
                                  new BugreportPageGrpc());
            break;
        case PANE_IDX_RECORD:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_GOOGLE_PLAY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_SETTINGS:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_HELP:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_CAR:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_CAR_ROTARY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
        case PANE_IDX_SENSOR_REPLAY:
            // replaceWidget(ui->stackedWidget, ui->, new xx());
            break;
    }
}

void ExtendedPageFactory::construct(Ui::ExtendedControls* ui,
                                    ExtendedWindowPane window) {
    bool grpc = getConsoleAgents()->settings->android_cmdLineOptions()->grpc_ui;
    if (grpc) {
        DD("Constucting gRPC component for %d", window);
        return constructGrpc(ui, window);
    }

    DD("Constucting default component for %d", window);
    return constructDefault(ui, window);
}
