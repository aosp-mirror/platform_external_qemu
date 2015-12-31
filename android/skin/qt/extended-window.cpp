/* Copyright (C) 2015 The Android Open Source Project
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

#include "android/main-common.h"
#include "android/skin/keyset.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/tool-window.h"

#include "ui_extended.h"

#include <QtWidgets>

ExtendedWindow::ExtendedWindow(
    EmulatorQtWindow *eW,
    ToolWindow *tW,
    const UiEmuAgent *agentPtr,
    const ShortcutKeyStore<QtUICommand>* shortcuts) :
    QFrame(nullptr),
    mEmulatorWindow(eW),
    mToolWindow(tW),
    mGeoDataLoader(nullptr),
    mEmulatorWindowAgent(agentPtr ? agentPtr->window    : nullptr),
    mLocationAgent      (agentPtr ? agentPtr->location  : nullptr),
    mSensorsAgent       (agentPtr ? agentPtr->sensors   : nullptr),
    mSettingsAgent      (agentPtr ? agentPtr->settings  : nullptr),
    mLoc_mSecRemaining(-1),
    mLoc_nowPlaying(false),
    mLoc_nowLoadingGeoData(false),
    mLoc_rowToSend(-1),
    mCloseRequested(false),
    mExtendedUi(new Ui::ExtendedControls)
{
    Q_INIT_RESOURCE(resources);

    // "Tool" type windows live in another layer on top of everything in OSX, which is undesirable
    // because it means the extended window must be on top of the emulator window. However, on
    // Windows and Linux, "Tool" type windows are the only way to make a window that does not have
    // its own taskbar item.
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif

    setWindowFlags(flag | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);

    mExtendedUi->setupUi(this);
    mExtendedUi->cellular_page->setCellularAgent(agentPtr->cellular);
    mExtendedUi->batteryPage->setBatteryAgent(agentPtr->battery);
    mExtendedUi->telephonyPage->setTelephonyAgent(agentPtr->telephony);
    mExtendedUi->finger_page->setFingerAgent(agentPtr->finger);
    mExtendedUi->helpPage->initializeKeyboardShortcutList(shortcuts);
    mExtendedUi->dpadPage->setUserEventsAgent(agentPtr->userEvents);

    connect(
        mExtendedUi->settingsPage, SIGNAL(themeChanged(SettingsTheme)),
        this, SLOT(switchToTheme(SettingsTheme)));

    // Do any sub-window-specific initialization
    initLocation();
    initVirtualSensors();

    mPaneButtonMap = {
        {PANE_IDX_LOCATION,  mExtendedUi->locationButton},
        {PANE_IDX_CELLULAR,  mExtendedUi->cellularButton},
        {PANE_IDX_BATTERY,   mExtendedUi->batteryButton},
        {PANE_IDX_TELEPHONE, mExtendedUi->telephoneButton},
        {PANE_IDX_DPAD,      mExtendedUi->dpadButton},
        {PANE_IDX_FINGER,    mExtendedUi->fingerButton},
        {PANE_IDX_SETTINGS,  mExtendedUi->settingsButton},
        {PANE_IDX_HELP,      mExtendedUi->helpButton},
    };

    move(mEmulatorWindow->geometry().right() + 40,
         mEmulatorWindow->geometry().top()   + 40 );
}

void ExtendedWindow::showPane(ExtendedWindowPane pane) {
    show();
    adjustTabs(pane);
}

void ExtendedWindow::closeEvent(QCloseEvent *ce)
{
    if (mLoc_nowLoadingGeoData) {
        mCloseRequested = true;
        ce->ignore();
    } else {
        if (mGeoDataLoader) {
            // This might cause lag when closing the extended window
            // when loading a very large KML/GPX file.
            mGeoDataLoader->wait();
        }
        mToolWindow->extendedIsClosing();
        ce->accept();
    }
}

// Tab buttons. Each raises its stacked pane to the top.
void ExtendedWindow::on_batteryButton_clicked()     { adjustTabs(PANE_IDX_BATTERY); }
void ExtendedWindow::on_cellularButton_clicked()    { adjustTabs(PANE_IDX_CELLULAR); }
void ExtendedWindow::on_dpadButton_clicked()        { adjustTabs(PANE_IDX_DPAD); }
void ExtendedWindow::on_fingerButton_clicked()      { adjustTabs(PANE_IDX_FINGER); }
void ExtendedWindow::on_helpButton_clicked()        { adjustTabs(PANE_IDX_HELP); }
void ExtendedWindow::on_locationButton_clicked()    { adjustTabs(PANE_IDX_LOCATION); }
void ExtendedWindow::on_settingsButton_clicked()    { adjustTabs(PANE_IDX_SETTINGS); }
void ExtendedWindow::on_telephoneButton_clicked()   { adjustTabs(PANE_IDX_TELEPHONE); }

void ExtendedWindow::adjustTabs(ExtendedWindowPane thisIndex)
{
    auto it = mPaneButtonMap.find(thisIndex);
    if (it == mPaneButtonMap.end()) {
        return;
    }
    QPushButton* thisButton = it->second;
    SettingsTheme theme = getSelectedTheme();

    // Make all the tab buttons the same except for the one whose
    // pane is on top.
    QString colorStyle("text-align: left; color:");
    colorStyle += (theme == SETTINGS_THEME_DARK) ?
                      DARK_MAJOR_TAB_COLOR : LIGHT_MAJOR_TAB_COLOR;
    colorStyle += "; background-color:";
    colorStyle += (theme == SETTINGS_THEME_DARK) ?
                      DARK_TAB_BKG_COLOR : LIGHT_TAB_BKG_COLOR;

    mExtendedUi->batteryButton    ->setStyleSheet(colorStyle);
    mExtendedUi->cellularButton   ->setStyleSheet(colorStyle);
    mExtendedUi->dpadButton       ->setStyleSheet(colorStyle);
    mExtendedUi->fingerButton     ->setStyleSheet(colorStyle);
    mExtendedUi->helpButton       ->setStyleSheet(colorStyle);
    mExtendedUi->locationButton   ->setStyleSheet(colorStyle);
    mExtendedUi->settingsButton   ->setStyleSheet(colorStyle);
    // Omit "spacer buttons" -- The main style sheet handles them
    mExtendedUi->telephoneButton  ->setStyleSheet(colorStyle);

    QString activeStyle("text-align: left; color:");
    activeStyle += (theme == SETTINGS_THEME_DARK) ?
                      DARK_MAJOR_TAB_COLOR : LIGHT_MAJOR_TAB_COLOR;
    activeStyle += "; background-color:";
    activeStyle += (theme == SETTINGS_THEME_DARK) ?
                      DARK_TAB_SELECTED_COLOR : LIGHT_TAB_SELECTED_COLOR;
    thisButton->setStyleSheet(activeStyle);

    mExtendedUi->batteryButton    ->setAutoFillBackground(true);
    mExtendedUi->cellularButton   ->setAutoFillBackground(true);
    mExtendedUi->dpadButton       ->setAutoFillBackground(true);
    mExtendedUi->fingerButton     ->setAutoFillBackground(true);
    mExtendedUi->helpButton       ->setAutoFillBackground(true);
    mExtendedUi->locationButton   ->setAutoFillBackground(true);
    mExtendedUi->settingsButton   ->setAutoFillBackground(true);
    mExtendedUi->spacer1Button    ->setAutoFillBackground(true);
    mExtendedUi->spacer2Button    ->setAutoFillBackground(true);
    mExtendedUi->telephoneButton  ->setAutoFillBackground(true);

    thisButton->clearFocus(); // It looks better when not highlighted
    mExtendedUi->stackedWidget->setCurrentIndex(static_cast<int>(thisIndex));
}

void ExtendedWindow::switchToTheme(SettingsTheme theme)
{
    // Switch to the icon images that are appropriate for this theme.
    switchAllIconsForTheme(theme);

    // Set the Qt style.

    // The first part is based on the display's pixel density.
    // Most displays give 1.0; high density displays give 2.0.
    double densityFactor = 1.0;
    if (skin_winsys_get_device_pixel_ratio(&densityFactor) != 0) {
        // Failed: use 1.0
        densityFactor = 1.0;
    }
    QString styleString = (densityFactor > 1.5) ? QT_FONTS(HI) : QT_FONTS(LO);


    // The second part is based on the theme
    // Set the style for this theme
    switch (theme) {
        case SETTINGS_THEME_DARK:
            styleString += QT_STYLE(DARK);
            break;
        case SETTINGS_THEME_LIGHT:
        default:
            styleString += QT_STYLE(LIGHT);
            break;
    }

    // Apply this style to the extended window (this),
    // and to the main tool-bar.
    this->setStyleSheet(styleString);
    mToolWindow->setStyleSheet(styleString);

    // Force a re-draw to make the new style take effect
    this->style()->unpolish(mExtendedUi->stackedWidget);
    this->style()->polish(mExtendedUi->stackedWidget);
    this->update();

    // Make the Settings pane active (still)
    adjustTabs(PANE_IDX_SETTINGS);
}

void ExtendedWindow::showEvent(QShowEvent* e) {
    if (!e->spontaneous()) {
        // This function has things that must be performed
        // after the ctor and after show() is called
        switchToTheme(getSelectedTheme());

        // Set the first tab active
        on_locationButton_clicked();
    }
    QFrame::showEvent(e);
}
