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

#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/tool-window.h"

#include "ui_extended.h"

#include <QtWidgets>

ExtendedWindow::ExtendedWindow(EmulatorQtWindow *eW, ToolWindow *tW, const UiEmuAgent *agentPtr) :
    QFrame(eW),
    mParentWindow(eW),
    mToolWindow(tW),
    mBatteryAgent  (agentPtr ? agentPtr->battery   : NULL),
    mCellularAgent (agentPtr ? agentPtr->cellular  : NULL),
    mFingerAgent   (agentPtr ? agentPtr->finger    : NULL),
    mLocationAgent (agentPtr ? agentPtr->location  : NULL),
    mSettingsAgent (agentPtr ? agentPtr->settings  : NULL),
    mTelephonyAgent(agentPtr ? agentPtr->telephony : NULL),
    mLoc_mSecRemaining(-1),
    mLoc_nowPaused(false),
    mLoc_rowToSend(-1),
    mExtendedUi(new Ui::ExtendedControls)
{
    Q_INIT_RESOURCE(resources);

    setWindowFlags(Qt::Tool | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);

    mExtendedUi->setupUi(this);

    // Do any sub-window-specific initialization
    initBattery();
    initCellular();
    initFinger();
    initKbdShorts();
    initLocation();
    initSd();
    initSettings();
    initSms();
    initTelephony();

    move(mParentWindow->geometry().right() + 40,
         mParentWindow->geometry().top()   + 40 );
}

void ExtendedWindow::completeInitialization()
{
    // TODO: Remember the theme setting from before
    // Set the theme to the initial selection
    // (by pretending the theme setting got changed)
    on_set_themeBox_currentIndexChanged(mSettingsState.mTheme);

    // Set the first tab active
    on_locationButton_clicked();
}

ExtendedWindow::~ExtendedWindow()
{
    // Do not delete 'extendedState'--we'll want it if
    // this window gets re-created.
}

void ExtendedWindow::closeEvent(QCloseEvent *ce)
{
    mToolWindow->extendedIsClosing();
}

// Tab buttons. Each raises its stacked pane to the top.
void ExtendedWindow::on_batteryButton_clicked()     { adjustTabs(mExtendedUi->batteryButton,     PANE_IDX_BATTERY); }
void ExtendedWindow::on_cameraButton_clicked()      { adjustTabs(mExtendedUi->cameraButton,      PANE_IDX_CAMERA); }
void ExtendedWindow::on_cellularButton_clicked()    { adjustTabs(mExtendedUi->cellularButton,    PANE_IDX_CELLULAR); }
void ExtendedWindow::on_dpadButton_clicked()        { adjustTabs(mExtendedUi->dpadButton,        PANE_IDX_DPAD); }
void ExtendedWindow::on_fingerButton_clicked()      { adjustTabs(mExtendedUi->fingerButton,      PANE_IDX_FINGER); }
void ExtendedWindow::on_hwSensorsButton_clicked()   { adjustTabs(mExtendedUi->hwSensorsButton,   PANE_IDX_HW_SENSORS); }
void ExtendedWindow::on_kbdShortsButton_clicked()   { adjustTabs(mExtendedUi->kbdShortsButton,   PANE_IDX_KBD_SHORTS); }
void ExtendedWindow::on_locationButton_clicked()    { adjustTabs(mExtendedUi->locationButton,    PANE_IDX_LOCATION); }
void ExtendedWindow::on_messageButton_clicked()     { adjustTabs(mExtendedUi->messageButton,     PANE_IDX_MESSAGE); }
void ExtendedWindow::on_recordScrButton_clicked()   { adjustTabs(mExtendedUi->recordScrButton,   PANE_IDX_RECORD_SCR); }
void ExtendedWindow::on_sdButton_clicked()          { adjustTabs(mExtendedUi->sdButton,          PANE_IDX_SD); }
void ExtendedWindow::on_settingsButton_clicked()    { adjustTabs(mExtendedUi->settingsButton,    PANE_IDX_SETTINGS); }
void ExtendedWindow::on_telephoneButton_clicked()   { adjustTabs(mExtendedUi->telephoneButton,   PANE_IDX_TELEPHONE); }
void ExtendedWindow::on_virtSensorsButton_clicked() { adjustTabs(mExtendedUi->virtSensorsButton, PANE_IDX_VIRT_SENSORS); }

void ExtendedWindow::adjustTabs(QPushButton *thisButton, int thisIndex)
{
    // Make all the tab buttons the same except for the one whose
    // pane is on top.
    QString colorStyle("text-align: left; color:");
    colorStyle += (mSettingsState.mTheme == SETTINGS_THEME_DARK) ?
                      DARK_MAJOR_TAB_COLOR : LIGHT_MAJOR_TAB_COLOR;
    colorStyle += "; background-color:";
    colorStyle += (mSettingsState.mTheme == SETTINGS_THEME_DARK) ?
                      DARK_TAB_BKG_COLOR : LIGHT_TAB_BKG_COLOR;

    // Omit "additionalButton" -- The main style sheet handles it
    mExtendedUi->batteryButton    ->setStyleSheet(colorStyle);
    mExtendedUi->cameraButton     ->setStyleSheet(colorStyle);
    mExtendedUi->cellularButton   ->setStyleSheet(colorStyle);
    mExtendedUi->dpadButton       ->setStyleSheet(colorStyle);
    mExtendedUi->fingerButton     ->setStyleSheet(colorStyle);
    mExtendedUi->hwSensorsButton  ->setStyleSheet(colorStyle);
    mExtendedUi->kbdShortsButton  ->setStyleSheet(colorStyle);
    mExtendedUi->locationButton   ->setStyleSheet(colorStyle);
    mExtendedUi->messageButton    ->setStyleSheet(colorStyle);
    mExtendedUi->recordScrButton  ->setStyleSheet(colorStyle);
    mExtendedUi->sdButton         ->setStyleSheet(colorStyle);
    mExtendedUi->settingsButton   ->setStyleSheet(colorStyle);
    // Omit "spacerButton" -- The main style sheet handles it
    mExtendedUi->telephoneButton  ->setStyleSheet(colorStyle);
    mExtendedUi->virtSensorsButton->setStyleSheet(colorStyle);

    QString activeStyle("text-align: left; color:");
    activeStyle += (mSettingsState.mTheme == SETTINGS_THEME_DARK) ?
                      DARK_MAJOR_TAB_COLOR : LIGHT_MAJOR_TAB_COLOR;
    activeStyle += "; background-color:";
    activeStyle += (mSettingsState.mTheme == SETTINGS_THEME_DARK) ?
                      DARK_BKG_COLOR : LIGHT_BKG_COLOR;
    thisButton->setStyleSheet(activeStyle);

    mExtendedUi->additionalButton ->setAutoFillBackground(true);
    mExtendedUi->batteryButton    ->setAutoFillBackground(true);
    mExtendedUi->cameraButton     ->setAutoFillBackground(true);
    mExtendedUi->cellularButton   ->setAutoFillBackground(true);
    mExtendedUi->dpadButton       ->setAutoFillBackground(true);
    mExtendedUi->fingerButton     ->setAutoFillBackground(true);
    mExtendedUi->hwSensorsButton  ->setAutoFillBackground(true);
    mExtendedUi->kbdShortsButton  ->setAutoFillBackground(true);
    mExtendedUi->locationButton   ->setAutoFillBackground(true);
    mExtendedUi->messageButton    ->setAutoFillBackground(true);
    mExtendedUi->recordScrButton  ->setAutoFillBackground(true);
    mExtendedUi->sdButton         ->setAutoFillBackground(true);
    mExtendedUi->settingsButton   ->setAutoFillBackground(true);
    mExtendedUi->spacerButton     ->setAutoFillBackground(true);
    mExtendedUi->telephoneButton  ->setAutoFillBackground(true);
    mExtendedUi->virtSensorsButton->setAutoFillBackground(true);

    thisButton->clearFocus(); // It looks better when not highlighted
    mExtendedUi->stackedWidget->setCurrentIndex(thisIndex);
}

void ExtendedWindow::initKbdShorts()
{
    // This table displays the keyboard shortcuts
    // Make the columns fill the bounding box
    mExtendedUi->shortcutsTableWidget->horizontalHeader()->
            setSectionResizeMode(QHeaderView::Stretch);
    // Make the rows big enough for the text to wrap onto two lines
    mExtendedUi->shortcutsTableWidget->verticalHeader()->
            setDefaultSectionSize(48);
    // QtDesigner sponaneously clears this attribute(!).
    // Set it explicitly.
    mExtendedUi->shortcutsTableWidget->horizontalHeader()->
            setVisible(true);
}

////////////////////////////////////////////////////////////
//
// phoneNumberValidator
//
// Validate the input of a telephone number.
// We allow '+' only in the first position.
// One or more digits (0-9) are required.
// Some additional characters are allowed, but ignored.

QValidator::State phoneNumberValidator::validate(QString &input, int &pos) const
{
    int numDigits = 0;
    const int MAX_DIGITS = 16;

    if (input.length() >= 32) return QValidator::Invalid;

    for (int ii=0; ii<input.length(); ii++) {
        if (input[ii] >= '0' &&  input[ii] <= '9') {
            numDigits++;
            if (numDigits > MAX_DIGITS) return QValidator::Invalid;
        } else if (input[ii] == '+' && ii != 0) {
            // '+' is only allowed as the first character
            return QValidator::Invalid;
        } else if (input[ii] != '-' &&
                   input[ii] != '.' &&
                   input[ii] != '(' &&
                   input[ii] != ')' &&
                   input[ii] != '/' &&
                   input[ii] != ' ' &&
                   input[ii] != '+'   )
        {
            return QValidator::Invalid;
        }
    }

    return ((numDigits > 0) ? QValidator::Acceptable : QValidator::Intermediate);
} // end validate()
