// Copyright (C) 2023 The Android Open Source Project
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
#include <QButtonGroup>                              // for QButtonGroup
#include <QString>                                   // for QString
#include <condition_variable>
#include <map>                                       // for map
#include <memory>                                    // for shared_ptr, uniq...
#include <mutex>

#include "android/skin/qt/extended-window-base.h"
#include "android/skin/qt/qt-ui-commands.h"          // for QtUICommand
#include "android/skin/qt/size-tweaker.h"            // for SizeTweaker
#include "host-common/qt_ui_defs.h"

class EmulatorQtWindow;
class QCloseEvent;
class QHideEvent;
class QKeyEvent;
class QObject;
class QPushButton;
class QShowEvent;
class ToolWindow;
class VirtualSceneControlWindow;
class VirtualSensorsPage;
template <class CommandType> class ShortcutKeyStore;

namespace android {
namespace metrics {
 class UiEventTracker;
}
}

using android::metrics::UiEventTracker;

namespace Ui {
    class ExtendedControlsGrpc;
}

class ExtendedWindowGrpc : public ExtendedBaseWindow
{
    Q_OBJECT

public:
    ExtendedWindowGrpc(EmulatorQtWindow* eW, ToolWindow* tW);

    ~ExtendedWindowGrpc();

    void sendMetricsOnShutDown() override;

    void show() override;
    void showPane(ExtendedWindowPane pane) override;

    // Wait until this component has reached the visibility
    // state.
    void waitForVisibility(bool visible) override;

    void connectVirtualSceneWindow(
            VirtualSceneControlWindow* virtualSceneWindow) override;

    VirtualSensorsPage* getVirtualSensorsPage() override;
private slots:
    void switchFrameAlways(bool showFrame);
    void switchOnTop(bool isOntop);
    void switchToTheme(SettingsTheme theme);
    void disableMouseWheel(bool disabled);
    void disablePinchToZoom(bool disabled);
    void pauseAvdWhenMinimized(bool pause);
    void showMacroRecordPage();
    void hideRotationButtons();

    // Master tabs
    void on_batteryButton_clicked();
    void on_cameraButton_clicked();
    void on_bugreportButton_clicked();
    void on_cellularButton_clicked();
    void on_dpadButton_clicked();
    void on_displaysButton_clicked();
    void on_fingerButton_clicked();
    void on_googlePlayButton_clicked();
    void on_helpButton_clicked();
    void on_carDataButton_clicked();
    void on_carRotaryButton_clicked();
    void on_sensorReplayButton_clicked();
    void on_locationButton_clicked();
    void on_microphoneButton_clicked();
    void on_recordButton_clicked();
    void on_rotaryInputButton_clicked();
    void on_settingsButton_clicked();
    void on_snapshotButton_clicked();
    void on_telephoneButton_clicked();
    void on_virtSensorsButton_clicked();

private:
    void closeEvent(QCloseEvent* ce) override;
    void keyPressEvent(QKeyEvent* e) override;
    void adjustTabs(ExtendedWindowPane thisIndex);
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

    EmulatorQtWindow* mEmulatorWindow;
    ToolWindow*  mToolWindow;
    std::map<ExtendedWindowPane, QPushButton*> mPaneButtonMap;
    std::shared_ptr<UiEventTracker> mPaneInvocationTracker;
    const ShortcutKeyStore<QtUICommand>* mQtUIShortcuts;
    std::unique_ptr<Ui::ExtendedControlsGrpc> mExtendedUi;
    bool mFirstShowEvent = true;
    SizeTweaker mSizeTweaker;
    QButtonGroup mSidebarButtons;
    bool mExtendedWindowGrpcWasShown = false;
    int mHAnchor = 0;
    int mVAnchor = 0;
    std::condition_variable mCvVisible;
    std::mutex mMutexVisible;
    bool mVisible{false};
};
