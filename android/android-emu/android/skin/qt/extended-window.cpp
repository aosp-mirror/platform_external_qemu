/* Copyright (C) 2015-2016 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include "android/skin/qt/extended-window.h"

#include <qnamespace.h>
#include <qstring.h>
#include <QApplication>
#include <QCloseEvent>
#if QT_VERSION >= 0x060000
#else
#include <QDesktopWidget>
#endif  // QT_VERSION
#include <QLayoutItem>
#include <QList>
#include <QPushButton>
#include <QRect>
#include <QSettings>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <utility>

#include "android/android.h"
#include "android/avd/hw-config.h"
#include "android/avd/info.h"
#include "android/avd/util.h"
#include "aemu/base/files/IniFile.h"
#include "android/cmdline-option.h"
#include "android/emulation/AutoDisplays.h"
#include "android/emulation/control/multi_display_agent.h"
#include "android/user-config.h"
#include "android/emulator-window.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/featurecontrol/Features.h"
#include "android/console.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/battery-page.h"
#include "android/skin/qt/extended-pages/bug-report-page.h"
#include "android/skin/qt/extended-pages/camera-page.h"
#include "android/skin/qt/extended-pages/car-data-page.h"
#include "android/skin/qt/extended-pages/car-rotary-page.h"
#include "android/skin/qt/extended-pages/cellular-page.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/dpad-page.h"
#include "android/skin/qt/extended-pages/finger-page.h"
#include "android/skin/qt/extended-pages/google-play-page.h"
#include "android/skin/qt/extended-pages/help-page.h"
#include "android/skin/qt/extended-pages/location-page.h"
#include "android/skin/qt/extended-pages/microphone-page.h"
#include "android/skin/qt/extended-pages/multi-display-page.h"
#include "android/skin/qt/extended-pages/record-and-playback-page.h"
#include "android/skin/qt/extended-pages/record-macro-page.h"
#include "android/skin/qt/extended-pages/record-screen-page.h"
#include "android/skin/qt/extended-pages/rotary-input-page.h"
#include "android/skin/qt/extended-pages/sensor-replay-page.h"
#include "android/skin/qt/extended-pages/settings-page.h"
#include "android/skin/qt/extended-pages/telephony-page.h"
#include "android/skin/qt/extended-pages/tv-remote-page.h"
#include "android/skin/qt/extended-pages/virtual-sensors-page.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/tool-window.h"
#include "android/skin/qt/virtualscene-control-window.h"
#include "android/ui-emu-agent.h"
#include "ui_extended.h"

class QApplication;
class QCloseEvent;
#if QT_VERSION >= 0x060000
#else
class QDesktopWidget;
#endif  // QT_VERSION
class QKeyEvent;
class QPushButton;
class QShowEvent;
class QWidget;

ExtendedWindow::ExtendedWindow(EmulatorQtWindow* eW, ToolWindow* tW)
    : QFrame(nullptr),
      mEmulatorWindow(eW),
      mToolWindow(tW),
      mExtendedUi(new Ui::ExtendedControls),
      mSizeTweaker(this),
      mSidebarButtons(this),
      mPaneInvocationTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::UNKONWN_EMULATOR_UI_EVENT_TYPE,
              android_studio::EmulatorUiEvent::EXTENDED_WINDOW_OPEN)) {
#ifdef __linux__
    // On Linux, a Dialog does not show a Close button and a
    // Tool has a Close button that shows a dot rather than
    // an X. So we use a SubWindow.
    setWindowFlags(Qt::SubWindow | Qt::WindowCloseButtonHint);
#else
    // On Windows, a SubWindow does not show a Close button.
    // A Dialog works for Windows and Mac
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
#endif

    QSettings settings;

    mExtendedUi->setupUi(this);
    mExtendedUi->helpPage->initialize(tW->getShortcutKeyStore());
    mExtendedUi->dpadPage->setEmulatorWindow(mEmulatorWindow);
    mExtendedUi->rotaryInputPage->setEmulatorWindow(mEmulatorWindow);
    mExtendedUi->microphonePage->setEmulatorWindow(mEmulatorWindow);
    mExtendedUi->tvRemotePage->setEmulatorWindow(mEmulatorWindow);
    mExtendedUi->tvRemotePage->setAdbInterface(
            mEmulatorWindow->getAdbInterface());
    mExtendedUi->settingsPage->setAdbInterface(
            mEmulatorWindow->getAdbInterface());

    if (getConsoleAgents()->settings->android_qemu_mode()) {
        mExtendedUi->bugreportPage->setAdbInterface(
                mEmulatorWindow->getAdbInterface());
    }

    if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_ANDROID_AUTO &&
        android::featurecontrol::isEnabled(
                android::featurecontrol::CarRotary) &&
        getConsoleAgents()->settings->android_qemu_mode()) {
        mExtendedUi->carRotaryPage->setAdbInterface(
                mEmulatorWindow->getAdbInterface());
    }

    connect(mExtendedUi->settingsPage, SIGNAL(frameAlwaysChanged(bool)), this,
            SLOT(switchFrameAlways(bool)));

    connect(mExtendedUi->settingsPage, SIGNAL(onTopChanged(bool)), this,
            SLOT(switchOnTop(bool)));

    connect(mExtendedUi->settingsPage,
            SIGNAL(onForwardShortcutsToDeviceChanged(int)), mEmulatorWindow,
            SLOT(setForwardShortcutsToDevice(int)));

    connect(mExtendedUi->settingsPage, SIGNAL(themeChanged(SettingsTheme)),
            this, SLOT(switchToTheme(SettingsTheme)));

    connect(mToolWindow, SIGNAL(themeChanged(SettingsTheme)), this,
            SLOT(switchToTheme(SettingsTheme)), Qt::DirectConnection);

    connect(mExtendedUi->settingsPage, SIGNAL(disableMouseWheelChanged(bool)),
            this, SLOT(disableMouseWheel(bool)));

    connect(mExtendedUi->settingsPage,
            SIGNAL(pauseAvdWhenMinimizedChanged(bool)), this,
            SLOT(pauseAvdWhenMinimized(bool)));

    connect(mExtendedUi->settingsPage,
            SIGNAL(enableClipboardSharingChanged(bool)), mToolWindow,
            SLOT(switchClipboardSharing(bool)));
    connect(mToolWindow, SIGNAL(haveClipboardSharingKnown(bool)),
            mExtendedUi->settingsPage, SLOT(setHaveClipboardSharing(bool)));

    connect(mExtendedUi->recordAndPlaybackPage,
            SIGNAL(ensureVirtualSceneWindowCreated()), mToolWindow,
            SLOT(ensureVirtualSceneWindowCreated()));

    connect(mExtendedUi->virtualSensorsPage, SIGNAL(windowVisible()), this,
            SLOT(hideRotationButtons()));
    // clang-format off
    mPaneButtonMap = {
        {PANE_IDX_CAR,           mExtendedUi->carDataButton},
        {PANE_IDX_CAR_ROTARY,    mExtendedUi->carRotaryButton},
        {PANE_IDX_SENSOR_REPLAY, mExtendedUi->sensorReplayButton},
        {PANE_IDX_LOCATION,      mExtendedUi->locationButton},
        {PANE_IDX_CELLULAR,      mExtendedUi->cellularButton},
        {PANE_IDX_BATTERY,       mExtendedUi->batteryButton},
        {PANE_IDX_CAMERA,        mExtendedUi->cameraButton},
        {PANE_IDX_TELEPHONE,     mExtendedUi->telephoneButton},
        {PANE_IDX_DPAD,          mExtendedUi->dpadButton},
        {PANE_IDX_TV_REMOTE,     mExtendedUi->dpadButton},
        {PANE_IDX_ROTARY,        mExtendedUi->rotaryInputButton},
        {PANE_IDX_MICROPHONE,    mExtendedUi->microphoneButton},
        {PANE_IDX_MULTIDISPLAY,  mExtendedUi->displaysButton},
        {PANE_IDX_FINGER,        mExtendedUi->fingerButton},
        {PANE_IDX_VIRT_SENSORS,  mExtendedUi->virtSensorsButton},
        {PANE_IDX_SNAPSHOT,      mExtendedUi->snapshotButton},
        {PANE_IDX_BUGREPORT,     mExtendedUi->bugreportButton},
        {PANE_IDX_SETTINGS,      mExtendedUi->settingsButton},
        {PANE_IDX_HELP,          mExtendedUi->helpButton},
        {PANE_IDX_RECORD,        mExtendedUi->recordButton},
        {PANE_IDX_GOOGLE_PLAY,   mExtendedUi->googlePlayButton},
    };
    // clang-format on

    setObjectName("ExtendedControls");
    // Use different title for Embedded Emuator in Studio.
    if (getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->qt_hide_window) {
        const auto* cfgIni = reinterpret_cast<const android::base::IniFile*>(
                avdInfo_getConfigIni(getConsoleAgents()->settings->avdInfo()));
        // If key avd.ini.displayname doesn't exists, use getConsoleAgents()->settings->hw()->avd_name
        // by default
        const auto displayName =
                cfgIni->getString("avd.ini.displayname", getConsoleAgents()->settings->hw()->avd_name);
        setWindowTitle(displayName.c_str() + QString(" - Extended Controls"));

    } else {
        setWindowTitle(QString("Extended Controls - ") + getConsoleAgents()->settings->hw()->avd_name +
                       ":" + QString::number(android_serial_number_port));
    }
    if (getConsoleAgents()->settings->has_cmdLineOptions() &&
        getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->no_location_ui) {
        mExtendedUi->locationButton->setVisible(false);
    } else {
        mSidebarButtons.addButton(mExtendedUi->locationButton);
    }

    if (android::featurecontrol::isEnabled(
                android::featurecontrol::MultiDisplay) &&
        !android_foldable_any_folded_area_configured() &&
        !android_foldable_hinge_configured() &&
        !android_foldable_rollable_configured() && !resizableEnabled() &&
        avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) != AVD_TV &&
        avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) != AVD_WEAR &&
        (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) != AVD_ANDROID_AUTO 
                || android::automotive::isMultiDisplaySupported(
                    getConsoleAgents()->settings->avdInfo()))) {
        mSidebarButtons.addButton(mExtendedUi->displaysButton);
        mExtendedUi->displaysButton->setVisible(true);
    } else {
        mExtendedUi->displaysButton->setVisible(false);
    }
    mSidebarButtons.addButton(mExtendedUi->cellularButton);
    mSidebarButtons.addButton(mExtendedUi->batteryButton);
    mSidebarButtons.addButton(mExtendedUi->telephoneButton);
    mSidebarButtons.addButton(mExtendedUi->dpadButton);
    if (getConsoleAgents()->settings->hw()->hw_rotaryInput ||
        avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_WEAR) {
        mSidebarButtons.addButton(mExtendedUi->rotaryInputButton);
    } else {
        mExtendedUi->rotaryInputButton->hide();
    }
    mSidebarButtons.addButton(mExtendedUi->microphoneButton);
    mSidebarButtons.addButton(mExtendedUi->fingerButton);

    // Currently, the camera page only contains options for the virtual scene
    // camera.  Hide the button if the virtual scene camera is not enabled, or
    // if we are using an Android Auto image because that does not have camera
    // support at the moment.
    if (androidHwConfig_hasVirtualSceneCamera(getConsoleAgents()->settings->hw()) &&
        (!getConsoleAgents()->settings->avdInfo() ||
         (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) != AVD_ANDROID_AUTO))) {
        mSidebarButtons.addButton(mExtendedUi->cameraButton);
        mExtendedUi->cameraButton->setVisible(true);
    } else {
        mExtendedUi->cameraButton->setVisible(false);
    }

    mSidebarButtons.addButton(mExtendedUi->virtSensorsButton);

    if (android::featurecontrol::isEnabled(
                android::featurecontrol::GenericSnapshotsUI) &&
        !getConsoleAgents()
                 ->settings->android_cmdLineOptions()
                 ->qt_hide_window) {
        mSidebarButtons.addButton(mExtendedUi->snapshotButton);
        mExtendedUi->snapshotButton->setVisible(true);
    } else {
        mExtendedUi->snapshotButton->setVisible(false);
    }

    mSidebarButtons.addButton(mExtendedUi->bugreportButton);
    mSidebarButtons.addButton(mExtendedUi->settingsButton);
    mSidebarButtons.addButton(mExtendedUi->helpButton);

    if (android::featurecontrol::isEnabled(
                android::featurecontrol::PlayStoreImage) &&
        getConsoleAgents()->settings->hw()->PlayStore_enabled) {
        mSidebarButtons.addButton(mExtendedUi->googlePlayButton);
        mExtendedUi->googlePlayPage->initialize(
                mEmulatorWindow->getAdbInterface());
        mExtendedUi->googlePlayButton->setVisible(true);
    }

    const bool screenRecording = android::featurecontrol::isEnabled(
            android::featurecontrol::ScreenRecording);
    const bool macroUi = android::featurecontrol::isEnabled(
            android::featurecontrol::MacroUi);
    if (screenRecording) {
        mSidebarButtons.addButton(mExtendedUi->recordButton);
        mExtendedUi->recordButton->setVisible(true);

        if (macroUi) {
            mExtendedUi->recordAndPlaybackPage->enableCustomMacros();
        } else {
            mExtendedUi->recordAndPlaybackPage->removeSettingsTab();
        }
    }

    if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_TV) {
        mExtendedUi->locationButton->setVisible(false);
        mExtendedUi->cellularButton->setVisible(false);
        mExtendedUi->virtSensorsButton->setVisible(false);
        mExtendedUi->fingerButton->setVisible(false);
        mExtendedUi->batteryButton->setVisible(false);
        mExtendedUi->telephoneButton->setVisible(false);
    }

    mExtendedUi->carRotaryButton->setVisible(false);

    if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_ANDROID_AUTO) {
        mSidebarButtons.addButton(mExtendedUi->carDataButton);
        mExtendedUi->carDataButton->setVisible(true);
        mExtendedUi->fingerButton->setVisible(false);
        mExtendedUi->batteryButton->setVisible(false);
        mExtendedUi->dpadButton->setVisible(false);
        mExtendedUi->telephoneButton->setVisible(false);

        if (android::featurecontrol::isEnabled(
                    android::featurecontrol::CarVhalReplay)) {
            mSidebarButtons.addButton(mExtendedUi->sensorReplayButton);
            mExtendedUi->sensorReplayButton->setVisible(true);
        } else {
            mExtendedUi->sensorReplayButton->setVisible(false);
        }

        if (android::featurecontrol::isEnabled(
                    android::featurecontrol::CarRotary) &&
            avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()) >= 30 /* Android 11 */) {
            mSidebarButtons.addButton(mExtendedUi->carRotaryButton);
            mExtendedUi->carRotaryButton->setVisible(true);
        } else {
            mExtendedUi->carRotaryButton->setVisible(false);
        }
    }

#ifdef __APPLE__
    for (QWidget* w : findChildren<QWidget*>()) {
        w->setAttribute(Qt::WA_MacShowFocusRect, false);
    }
#endif

    connect(mExtendedUi->location_page, SIGNAL(targetHeadingChanged(double)),
            mExtendedUi->virtualSensorsPage,
            SLOT(setTargetHeadingDegrees(double)));

    const auto enableClipboardSharing =
            settings.value(Ui::Settings::CLIPBOARD_SHARING, true).toBool();
    mToolWindow->switchClipboardSharing(enableClipboardSharing);

    mEmulatorWindow->setPauseAvdWhenMinimized(
            SettingsPage::getPauseAvdWhenMinimized());
}

ExtendedWindow::~ExtendedWindow() {
    mExtendedUi->location_page->requestStopLoadingGeoData();
    if (getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->qt_hide_window &&
        !mFirstShowEvent) {
        auto userConfig = getConsoleAgents()->settings->userConfig();
        QRect geom = geometry();
        auserConfig_setExtendedControlsPos(userConfig, geom.x(), geom.y(),
                                           mHAnchor, mVAnchor);
    }
}

// Translates an ExtendWindowPane value into a human
// readable string. Translation is done by
// turning the identifier into a string and removing
// the _IDX tag.
static std::string translate_idx(ExtendedWindowPane value) {
    std::string s = "";
#define PANE(p)                   \
    case (ExtendedWindowPane::p): \
        s = #p;                   \
        break;
    switch (value) {
        PANE(PANE_IDX_UNKNOWN)
        PANE(PANE_IDX_LOCATION)
        PANE(PANE_IDX_MULTIDISPLAY)
        PANE(PANE_IDX_CELLULAR)
        PANE(PANE_IDX_BATTERY)
        PANE(PANE_IDX_CAMERA)
        PANE(PANE_IDX_TELEPHONE)
        PANE(PANE_IDX_DPAD)
        PANE(PANE_IDX_TV_REMOTE)
        PANE(PANE_IDX_ROTARY)
        PANE(PANE_IDX_MICROPHONE)
        PANE(PANE_IDX_FINGER)
        PANE(PANE_IDX_VIRT_SENSORS)
        PANE(PANE_IDX_SNAPSHOT)
        PANE(PANE_IDX_BUGREPORT)
        PANE(PANE_IDX_RECORD)
        PANE(PANE_IDX_GOOGLE_PLAY)
        PANE(PANE_IDX_SETTINGS)
        PANE(PANE_IDX_HELP)
        PANE(PANE_IDX_CAR)
        PANE(PANE_IDX_CAR_ROTARY)
        PANE(PANE_IDX_SENSOR_REPLAY)
    }
#undef PANE
    // Remove _IDX from the string.
    assert(s.substr(0, 8) == "PANE_IDX");
    return s.replace(4, 4, "");
}

void ExtendedWindow::sendMetricsOnShutDown() {
    mExtendedUi->location_page->sendMetrics();
    mExtendedUi->multiDisplayPage->sendMetrics();
}

// static
void ExtendedWindow::setAgent(const UiEmuAgent* agentPtr) {
    if (agentPtr) {
        BatteryPage::setBatteryAgent(agentPtr->battery);
        CellularPage::setCellularAgent(agentPtr->cellular);
        FingerPage::setFingerAgent(agentPtr->finger);
        if (!getConsoleAgents()
                     ->settings->android_cmdLineOptions()
                     ->no_location_ui) {
            LocationPage::setLocationAgent(agentPtr->location);
        }
        SettingsPage::setHttpProxyAgent(agentPtr->proxy);
        TelephonyPage::setTelephonyAgent(agentPtr->telephony);
        CameraPage::setVirtualSceneAgent(agentPtr->virtualScene);
        VirtualSensorsPage::setSensorsAgent(agentPtr->sensors);
        RecordMacroPage::setAutomationAgent(agentPtr->automation);
        RecordScreenPage::setRecordScreenAgent(agentPtr->record);
        if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_ANDROID_AUTO) {
            CarDataPage::setCarDataAgent(agentPtr->car);
            SensorReplayPage::setAgent(agentPtr->car, agentPtr->location,
                                       agentPtr->sensors);
        }
    }
}

// static
void ExtendedWindow::shutDown() {
    if (!getConsoleAgents()
                 ->settings->android_cmdLineOptions()
                 ->no_location_ui) {
        LocationPage::shutDown();
    }
}

void ExtendedWindow::show() {
    // bug: 183660415
    // As reported in KDE desktop environment, a minimize button is shown and
    // we need to show minimized window differently.
    if (isMinimized()) {
        QFrame::showNormal();
    } else {
        QFrame::show();
    }
    mExtendedWindowWasShown = true;

    // Verify that the extended pane is fully visible (otherwise it may be
    // impossible for the user to move it)
#if QT_VERSION >= 0x060000
    QRect screenGeo = this->screen()->geometry();
#else
    QDesktopWidget* desktop =
            static_cast<QApplication*>(QApplication::instance())->desktop();
    int screenNum = desktop->screenNumber(this);  // Screen holding most of this
    QRect screenGeo = desktop->screenGeometry(screenNum);
#endif  // QT_VERSION
    QRect myGeo = geometry();

    bool moved = false;
    // Leave some padding between the window and the edge of the screen.
    // This distance isn't precise--it's mainly to prevent the window from
    // looking like it's a little off screen.
    static const int gap = 10;
    if (myGeo.x() + myGeo.width() > screenGeo.x() + screenGeo.width() - gap) {
        // Right edge is off the screen
        myGeo.setX(screenGeo.x() + screenGeo.width() - myGeo.width() - gap);
        moved = true;
    }
    if (myGeo.y() + myGeo.height() > screenGeo.y() + screenGeo.height() - gap) {
        // Bottom edge is off the screen
        myGeo.setY(screenGeo.y() + screenGeo.height() - myGeo.height() - gap);
        moved = true;
    }
    if (myGeo.x() < screenGeo.x() + gap) {
        // Top edge is off the screen
        myGeo.setX(screenGeo.x() + gap);
        moved = true;
    }
    if (myGeo.y() < screenGeo.y() + gap) {
        // Left edge is off the screen
        myGeo.setY(screenGeo.y() + gap);
        moved = true;
    }
    if (moved) {
        setGeometry(myGeo);
    }
}

void ExtendedWindow::showPane(ExtendedWindowPane pane) {
    show();
    adjustTabs(pane);
}

void ExtendedWindow::connectVirtualSceneWindow(
        VirtualSceneControlWindow* virtualSceneWindow) {
    connect(virtualSceneWindow, SIGNAL(virtualSceneControlsEngaged(bool)),
            mExtendedUi->virtualSensorsPage,
            SLOT(onVirtualSceneControlsEngaged(bool)));

    // The virtual scene control window aggregates metrics when the virtual
    // scene is active. Route orientation changes and virtual sensors page
    // events to the control window. Events are discarded unless the window
    // is active.
    connect(mExtendedUi->virtualSensorsPage,
            SIGNAL(coarseOrientationChanged(SkinRotation)), virtualSceneWindow,
            SLOT(orientationChanged(SkinRotation)));
    connect(mExtendedUi->virtualSensorsPage, SIGNAL(windowVisible()),
            virtualSceneWindow, SLOT(virtualSensorsPageVisible()));
    connect(mExtendedUi->virtualSensorsPage,
            SIGNAL(virtualSensorsInteraction()), virtualSceneWindow,
            SLOT(virtualSensorsInteraction()));
    connect(mExtendedUi->recordAndPlaybackPage,
            SIGNAL(setRecordingStateSignal(bool)), virtualSceneWindow,
            SLOT(setRecordingState(bool)));
    connect(virtualSceneWindow, SIGNAL(on_recOngoingButton_clicked()), this,
            SLOT(showMacroRecordPage()));
}

void ExtendedWindow::closeEvent(QCloseEvent* e) {
    // Merely hide the window the widget is closed, do not destroy state.
    e->ignore();
    hide();
}

void ExtendedWindow::keyPressEvent(QKeyEvent* e) {
    mToolWindow->handleQtKeyEvent(e, QtKeyEventSource::ExtendedWindow);
}

// Tab buttons. Each raises its stacked pane to the top.
void ExtendedWindow::on_batteryButton_clicked() {
    adjustTabs(PANE_IDX_BATTERY);
}
void ExtendedWindow::on_cameraButton_clicked() {
    adjustTabs(PANE_IDX_CAMERA);
}
void ExtendedWindow::on_bugreportButton_clicked() {
    adjustTabs(PANE_IDX_BUGREPORT);
}
void ExtendedWindow::on_cellularButton_clicked() {
    adjustTabs(PANE_IDX_CELLULAR);
}
void ExtendedWindow::on_dpadButton_clicked() {
    if (android::featurecontrol::isEnabled(android::featurecontrol::TvRemote) &&
        avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_TV) {
        adjustTabs(PANE_IDX_TV_REMOTE);
    } else {
        adjustTabs(PANE_IDX_DPAD);
    }
}
void ExtendedWindow::on_displaysButton_clicked() {
    adjustTabs(PANE_IDX_MULTIDISPLAY);
}
void ExtendedWindow::on_rotaryInputButton_clicked() {
    adjustTabs(PANE_IDX_ROTARY);
}
void ExtendedWindow::on_fingerButton_clicked() {
    adjustTabs(PANE_IDX_FINGER);
}
void ExtendedWindow::on_helpButton_clicked() {
    adjustTabs(PANE_IDX_HELP);
}
void ExtendedWindow::on_locationButton_clicked() {
    adjustTabs(PANE_IDX_LOCATION);
}
void ExtendedWindow::on_microphoneButton_clicked() {
    adjustTabs(PANE_IDX_MICROPHONE);
}
void ExtendedWindow::on_settingsButton_clicked() {
    adjustTabs(PANE_IDX_SETTINGS);
}
void ExtendedWindow::on_telephoneButton_clicked() {
    adjustTabs(PANE_IDX_TELEPHONE);
}
void ExtendedWindow::on_virtSensorsButton_clicked() {
    adjustTabs(PANE_IDX_VIRT_SENSORS);
}
void ExtendedWindow::on_snapshotButton_clicked() {
    adjustTabs(PANE_IDX_SNAPSHOT);
}
void ExtendedWindow::on_recordButton_clicked() {
    adjustTabs(PANE_IDX_RECORD);
}
void ExtendedWindow::on_googlePlayButton_clicked() {
    adjustTabs(PANE_IDX_GOOGLE_PLAY);
}
void ExtendedWindow::on_carDataButton_clicked() {
    adjustTabs(PANE_IDX_CAR);
}
void ExtendedWindow::on_carRotaryButton_clicked() {
    adjustTabs(PANE_IDX_CAR_ROTARY);
}
void ExtendedWindow::on_sensorReplayButton_clicked() {
    adjustTabs(PANE_IDX_SENSOR_REPLAY);
}

void ExtendedWindow::adjustTabs(ExtendedWindowPane thisIndex) {
    auto it = mPaneButtonMap.find(thisIndex);
    if (it == mPaneButtonMap.end()) {
        return;
    }

    if (mExtendedWindowWasShown &&
        mExtendedUi->stackedWidget->currentIndex() != thisIndex) {
        mPaneInvocationTracker->increment(translate_idx(thisIndex));
    }
    QPushButton* thisButton = it->second;
    thisButton->toggle();
    thisButton->clearFocus();  // It looks better when not highlighted
    mExtendedUi->stackedWidget->setCurrentIndex(static_cast<int>(thisIndex));
}

void ExtendedWindow::switchFrameAlways(bool showFrame) {
    mEmulatorWindow->setFrameAlways(showFrame);
}

void ExtendedWindow::switchOnTop(bool isOnTop) {
    if (getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->qt_hide_window) {
        setFrameOnTop(this, isOnTop);
    }
    mEmulatorWindow->setOnTop(isOnTop);
    mToolWindow->notifySwitchOnTop();
}

void ExtendedWindow::switchToTheme(SettingsTheme theme) {
    // Switch to the icon images that are appropriate for this theme.
    adjustAllButtonsForTheme(theme);

    // Set the Qt style.

    // The first part is based on the display's pixel density.
    // Most displays give 1.0; high density displays give 2.0.
    const double densityFactor = devicePixelRatioF();
    QString styleString = Ui::fontStylesheet(densityFactor > 1.5);

    // The second part is based on the theme
    // Set the style for this theme
    styleString += Ui::stylesheetForTheme(theme);

    // Apply this style to the extended window (this),
    // and to the main tool-bar.
    this->setStyleSheet(styleString);
    mToolWindow->updateTheme(styleString);
    mExtendedUi->rotaryInputPage->updateTheme();
    mExtendedUi->location_page->updateTheme();
    mExtendedUi->multiDisplayPage->updateTheme(styleString);
    mExtendedUi->bugreportPage->updateTheme();
    mExtendedUi->recordAndPlaybackPage->updateTheme();

    // Force a re-draw to make the new style take effect
    this->style()->unpolish(mExtendedUi->stackedWidget);
    this->style()->polish(mExtendedUi->stackedWidget);
    this->update();

    // Make the Settings pane active (still)
    adjustTabs(PANE_IDX_SETTINGS);
}

void ExtendedWindow::disableMouseWheel(bool disabled) {
    mEmulatorWindow->setIgnoreWheelEvent(disabled);
}

void ExtendedWindow::pauseAvdWhenMinimized(bool pause) {
    mEmulatorWindow->setPauseAvdWhenMinimized(pause);
}

void ExtendedWindow::showEvent(QShowEvent* e) {
    if (mFirstShowEvent && !e->spontaneous()) {
        bool moved = false;
        if (getConsoleAgents()
                    ->settings->android_cmdLineOptions()
                    ->qt_hide_window) {
            const QIcon icon(":/all/android_studio_icon");
            setWindowIcon(icon);
            auto userConfig = getConsoleAgents()->settings->userConfig();
            int x, y;
            if (auserConfig_getExtendedControlsPos(userConfig, &x, &y,
                                                   &mHAnchor, &mVAnchor)) {
                VerticalAnchor v = (VerticalAnchor)mVAnchor;
                HorizontalAnchor h = (HorizontalAnchor)mHAnchor;
                auto size = geometry();
                if (h == HCENTER) {
                    x -= (size.width() / 2);
                } else if (h == RIGHT) {
                    x -= size.width();
                }

                if (v == VCENTER) {
                    y -= (size.height() / 2);
                } else if (v == BOTTOM) {
                    y -= size.height();
                }

                move(x, y);
                moved = true;
            }
        }

        // This function has things that must be performed
        // after the ctor and after show() is called
        switchToTheme(getSelectedTheme());

        // Set the first tab active

        // Programatically click the top sidebar button to get the correct page
        // to show.
        for (int i = 0; i < mExtendedUi->verticalLayout->count(); ++i) {
            auto* pushBtn = reinterpret_cast<QPushButton*>(
                    mExtendedUi->verticalLayout->itemAt(i)->widget());
            if (pushBtn->isVisible()) {
                pushBtn->click();
                break;
            }
        }

        mFirstShowEvent = false;
        if (!moved) {
            // There is a gap between the main window and the tool bar. Use the
            // same gap between the tool bar and the extended window.
            move(mToolWindow->geometry().right() + 3 +
                         ToolWindow::TOOL_GAP_FRAMELESS,
                 mToolWindow->geometry().top());
        }
    }
    QFrame::showEvent(e);
    {
        std::lock_guard<std::mutex> lk(mMutexVisible);
        mVisible = true;
    }
    mCvVisible.notify_all();
}

void ExtendedWindow::hideEvent(QHideEvent* e) {
    QFrame::hideEvent(e);
    {
        std::lock_guard<std::mutex> lk(mMutexVisible);
        mVisible = false;
    }
    mCvVisible.notify_all();
}

void ExtendedWindow::waitForVisibility(bool visible) {
    std::unique_lock lk(mMutexVisible);
    mCvVisible.wait(lk, [&] { return visible == mVisible; });
}
void ExtendedWindow::showMacroRecordPage() {
    show();
    on_recordButton_clicked();
    mExtendedUi->recordAndPlaybackPage->focusMacroRecordTab();
}

void ExtendedWindow::hideRotationButtons() {
    mExtendedUi->virtualSensorsPage->hideRotationButtons(
            mToolWindow->getUiEmuAgent()
                    ->multiDisplay->isMultiDisplayEnabled());
}
