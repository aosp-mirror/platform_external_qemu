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
#include "android/skin/qt/extended-window-styles.h"

#include "android/android.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/main-common.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/help-page.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/tool-window.h"
#include "android/ui-emu-agent.h"

#include "ui_extended.h"

#include <QDesktopWidget>
ExtendedWindow::ExtendedWindow(
    EmulatorQtWindow *eW,
    ToolWindow  *tW) :
    QFrame(nullptr),
    mEmulatorWindow(eW),
    mToolWindow(tW),
    mExtendedUi(new Ui::ExtendedControls),
    mSizeTweaker(this),
    mSidebarButtons(this)
{
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
    mExtendedUi->settingsPage->setAdbInterface(
            mEmulatorWindow->getAdbInterface());
    mExtendedUi->bugreportPage->setAdbInterface(
            mEmulatorWindow->getAdbInterface());

    connect(
        mExtendedUi->settingsPage, SIGNAL(frameAlwaysChanged(bool)),
        this, SLOT(switchFrameAlways(bool)));

    connect(
        mExtendedUi->settingsPage, SIGNAL(onTopChanged(bool)),
        this, SLOT(switchOnTop(bool)));

    connect(
        mExtendedUi->settingsPage, SIGNAL(onForwardShortcutsToDeviceChanged(int)),
        mEmulatorWindow, SLOT(setForwardShortcutsToDevice(int)));

    connect(
        mExtendedUi->settingsPage, SIGNAL(themeChanged(SettingsTheme)),
        this, SLOT(switchToTheme(SettingsTheme)));

    connect(mExtendedUi->settingsPage, SIGNAL(disableMouseWheelChanged(bool)),
        this, SLOT(disableMouseWheel(bool)));

    connect(
        mExtendedUi->settingsPage, SIGNAL(enableClipboardSharingChanged(bool)),
        mToolWindow, SLOT(switchClipboardSharing(bool)));
    connect(
        mToolWindow, SIGNAL(haveClipboardSharingKnown(bool)),
        mExtendedUi->settingsPage, SLOT(setHaveClipboardSharing(bool)));

    // clang-format off
    mPaneButtonMap = {
        {PANE_IDX_CAR,           mExtendedUi->carDataButton},
        {PANE_IDX_LOCATION,      mExtendedUi->locationButton},
        {PANE_IDX_CELLULAR,      mExtendedUi->cellularButton},
        {PANE_IDX_BATTERY,       mExtendedUi->batteryButton},
        {PANE_IDX_CAMERA,        mExtendedUi->cameraButton},
        {PANE_IDX_TELEPHONE,     mExtendedUi->telephoneButton},
        {PANE_IDX_DPAD,          mExtendedUi->dpadButton},
        {PANE_IDX_ROTARY,        mExtendedUi->rotaryInputButton},
        {PANE_IDX_MICROPHONE,    mExtendedUi->microphoneButton},
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
    setWindowTitle(QString("Extended controls - ") + android_hw->avd_name
                   + ":" + QString::number(android_serial_number_port));

    mSidebarButtons.addButton(mExtendedUi->locationButton);
    mSidebarButtons.addButton(mExtendedUi->cellularButton);
    mSidebarButtons.addButton(mExtendedUi->batteryButton);
    mSidebarButtons.addButton(mExtendedUi->telephoneButton);
    mSidebarButtons.addButton(mExtendedUi->dpadButton);
    if (android_hw->hw_rotaryInput) {
        mSidebarButtons.addButton(mExtendedUi->rotaryInputButton);
    } else {
        mExtendedUi->rotaryInputButton->hide();
    }
    mSidebarButtons.addButton(mExtendedUi->microphoneButton);
    mSidebarButtons.addButton(mExtendedUi->fingerButton);

    // Currently, the camera page only contains options for the virtual scene
    // camera.  Hide the button if the virtual scene camera is not enabled, or if we are using an Android Auto image because that does not
    // have camera support at the moment.
    if (androidHwConfig_hasVirtualSceneCamera(android_hw) &&
        (!android_avdInfo ||
         (avdInfo_getAvdFlavor(android_avdInfo) != AVD_ANDROID_AUTO))) {
        mSidebarButtons.addButton(mExtendedUi->cameraButton);
        mExtendedUi->cameraButton->setVisible(true);
    } else {
        mExtendedUi->cameraButton->setVisible(false);
    }

    mSidebarButtons.addButton(mExtendedUi->virtSensorsButton);

    if (android::featurecontrol::isEnabled(android::featurecontrol::GenericSnapshotsUI)) {
        mSidebarButtons.addButton(mExtendedUi->snapshotButton);
        mExtendedUi->snapshotButton->setVisible(true);
    } else {
        mExtendedUi->snapshotButton->setVisible(false);
    }

    mSidebarButtons.addButton(mExtendedUi->bugreportButton);
    mSidebarButtons.addButton(mExtendedUi->settingsButton);
    mSidebarButtons.addButton(mExtendedUi->helpButton);

    if (android::featurecontrol::isEnabled(android::featurecontrol::PlayStoreImage) &&
        android_hw->PlayStore_enabled)
    {
        mSidebarButtons.addButton(mExtendedUi->googlePlayButton);
        mExtendedUi->googlePlayPage->initialize(
                mEmulatorWindow->getAdbInterface());
        mExtendedUi->googlePlayButton->setVisible(true);
    }

    const bool screenRecording = android::featurecontrol::isEnabled(
            android::featurecontrol::ScreenRecording);
    if (screenRecording) {
        mSidebarButtons.addButton(mExtendedUi->recordButton);
        mExtendedUi->recordButton->setVisible(true);
    }

    if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
        mSidebarButtons.addButton(mExtendedUi->carDataButton);
        mExtendedUi->carDataButton->setVisible(true);
        mExtendedUi->fingerButton->setVisible(false);
        mExtendedUi->batteryButton->setVisible(false);
        mExtendedUi->dpadButton->setVisible(false);
    }

#ifdef __APPLE__
    for (QWidget* w : findChildren<QWidget*>()) {
        w->setAttribute(Qt::WA_MacShowFocusRect, false);
    }
#endif

    connect(mExtendedUi->location_page,
            SIGNAL(targetHeadingChanged(double)),
            mExtendedUi->virtualSensorsPage,
            SLOT(setTargetHeadingDegrees(double)));

    const auto enableClipboardSharing =
            settings.value(Ui::Settings::CLIPBOARD_SHARING, true).toBool();
    mToolWindow->switchClipboardSharing(enableClipboardSharing);
}

ExtendedWindow::~ExtendedWindow() {
    mExtendedUi->location_page->requestStopLoadingGeoData();
}

// static
void ExtendedWindow::setAgent(const UiEmuAgent* agentPtr) {
    if (agentPtr) {
        BatteryPage::setBatteryAgent(agentPtr->battery);
        CellularPage::setCellularAgent(agentPtr->cellular);
        FingerPage::setFingerAgent(agentPtr->finger);
        LocationPage::setLocationAgent(agentPtr->location);
        SettingsPage::setHttpProxyAgent(agentPtr->proxy);
        TelephonyPage::setTelephonyAgent(agentPtr->telephony);
        CameraPage::setVirtualSceneAgent(agentPtr->virtualScene);
        VirtualSensorsPage::setSensorsAgent(agentPtr->sensors);
        RecordMacroPage::setAutomationAgent(agentPtr->automation);
        RecordScreenPage::setRecordScreenAgent(agentPtr->record);
        if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
            CarDataPage::setCarDataAgent(agentPtr->car);
        }
    }
}

// static
void ExtendedWindow::shutDown() {
    LocationPage::shutDown();
}

void ExtendedWindow::show() {
    QFrame::show();

    // Verify that the extended pane is fully visible (otherwise it may be
    // impossible for the user to move it)
    QDesktopWidget *desktop = static_cast<QApplication*>(
                                     QApplication::instance() )->desktop();
    int screenNum = desktop->screenNumber(this); // Screen holding most of this

    QRect screenGeo = desktop->screenGeometry(screenNum);
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
}

void ExtendedWindow::closeEvent(QCloseEvent *e) {
    // Merely hide the window the widget is closed, do not destroy state.
    e->ignore();
    hide();
}

void ExtendedWindow::keyPressEvent(QKeyEvent* e) {
    mToolWindow->handleQtKeyEvent(e, QtKeyEventSource::ExtendedWindow);
}

// Tab buttons. Each raises its stacked pane to the top.
void ExtendedWindow::on_batteryButton_clicked()      { adjustTabs(PANE_IDX_BATTERY); }
void ExtendedWindow::on_cameraButton_clicked()       { adjustTabs(PANE_IDX_CAMERA); }
void ExtendedWindow::on_bugreportButton_clicked()    { adjustTabs(PANE_IDX_BUGREPORT); }
void ExtendedWindow::on_cellularButton_clicked()     { adjustTabs(PANE_IDX_CELLULAR); }
void ExtendedWindow::on_dpadButton_clicked()         { adjustTabs(PANE_IDX_DPAD); }
void ExtendedWindow::on_rotaryInputButton_clicked()  { adjustTabs(PANE_IDX_ROTARY); }
void ExtendedWindow::on_fingerButton_clicked()       { adjustTabs(PANE_IDX_FINGER); }
void ExtendedWindow::on_helpButton_clicked()         { adjustTabs(PANE_IDX_HELP); }
void ExtendedWindow::on_locationButton_clicked()     { adjustTabs(PANE_IDX_LOCATION); }
void ExtendedWindow::on_microphoneButton_clicked()   { adjustTabs(PANE_IDX_MICROPHONE); }
void ExtendedWindow::on_settingsButton_clicked()     { adjustTabs(PANE_IDX_SETTINGS); }
void ExtendedWindow::on_telephoneButton_clicked()    { adjustTabs(PANE_IDX_TELEPHONE); }
void ExtendedWindow::on_virtSensorsButton_clicked()  { adjustTabs(PANE_IDX_VIRT_SENSORS); }
void ExtendedWindow::on_snapshotButton_clicked()     { adjustTabs(PANE_IDX_SNAPSHOT); }
void ExtendedWindow::on_recordButton_clicked()       { adjustTabs(PANE_IDX_RECORD); }
void ExtendedWindow::on_googlePlayButton_clicked()   { adjustTabs(PANE_IDX_GOOGLE_PLAY); }
void ExtendedWindow::on_carDataButton_clicked()      { adjustTabs(PANE_IDX_CAR); }

void ExtendedWindow::adjustTabs(ExtendedWindowPane thisIndex) {
    auto it = mPaneButtonMap.find(thisIndex);
    if (it == mPaneButtonMap.end()) {
        return;
    }
    QPushButton* thisButton = it->second;
    thisButton->toggle();
    thisButton->clearFocus(); // It looks better when not highlighted
    mExtendedUi->stackedWidget->setCurrentIndex(static_cast<int>(thisIndex));
}

void ExtendedWindow::switchFrameAlways(bool showFrame)
{
    mEmulatorWindow->setFrameAlways(showFrame);
}

void ExtendedWindow::switchOnTop(bool isOnTop) {
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

void ExtendedWindow::showEvent(QShowEvent* e) {
    if (mFirstShowEvent && !e->spontaneous()) {
        // This function has things that must be performed
        // after the ctor and after show() is called
        switchToTheme(getSelectedTheme());

        // Set the first tab active
        on_locationButton_clicked();

        mFirstShowEvent = false;

        // There is a gap between the main window and the tool bar. Use the same
        // gap between the tool bar and the extended window.
        move(mToolWindow->geometry().right() + 3 + ToolWindow::TOOL_GAP_FRAMELESS,
             mToolWindow->geometry().top());
    }
    QFrame::showEvent(e);
}
