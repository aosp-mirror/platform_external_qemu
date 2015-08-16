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
    initLocation();
    initSd();
    initSms();
    initTelephony();

    move(mParentWindow->geometry().right() + 40,
         mParentWindow->geometry().top()   + 40 );
}

void ExtendedWindow::completeInitialization()
{
    // Set the theme to dark (by setting it light and doing a switch-over)
    mThemeIsDark = false;
    on_theme_pushButton_clicked();
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

// TODO: This will probably not be implemented by a direct button press
void ExtendedWindow::on_theme_pushButton_clicked()
{
    mThemeIsDark = !mThemeIsDark;

    QString style;
    style += SLIDER_STYLE;
    if (mThemeIsDark) {
        style += LIGHT_CHECKBOX_STYLE;
        // TODO: Verify that this is the right set of widgets to modify
        style += " QTextEdit, QPlainTextEdit, QTreeView { border: 1px solid " DARK_FOREGROUND " } ";
        style += "*{color:" DARK_FOREGROUND  ";background-color: " DARK_BACKGROUND  "}";
    } else {
        style += DARK_CHECKBOX_STYLE;
        style += " QTextEdit, QPlainTextEdit, QTreeView { border: 1px solid " LIGHT_FOREGROUND " } ";
        style += "*{color:" LIGHT_FOREGROUND ";background-color:" LIGHT_BACKGROUND "}";
    }

    // Switch to the icon images that are appropriate for this theme.
    // Examine every widget.
    QWidgetList wList = QApplication::allWidgets();
    for (int idx = 0; idx < wList.size(); idx++) {
        QPushButton *pB = dynamic_cast<QPushButton*>(wList[idx]);
        if (pB && !pB->icon().isNull()) {
            setButtonEnabled(pB, pB->isEnabled());
        }
    }

    // Apply the updated style sheet and force a re-draw
    this->setStyleSheet(style);
    this->style()->unpolish(mExtendedUi->stackedWidget);
    this->style()->polish(mExtendedUi->stackedWidget);
    this->update();

    // Start with the Battery pane active
    adjustTabs(mExtendedUi->batteryButton, 0);
}

// Tab buttons. Each raises its stacked pane to the top.

void ExtendedWindow::on_batteryButton_clicked()   { adjustTabs(mExtendedUi->batteryButton,   0); }
void ExtendedWindow::on_telephoneButton_clicked() { adjustTabs(mExtendedUi->telephoneButton, 1); }
void ExtendedWindow::on_messageButton_clicked()   { adjustTabs(mExtendedUi->messageButton,   2); }
void ExtendedWindow::on_locationButton_clicked()  { adjustTabs(mExtendedUi->locationButton,  3); }
void ExtendedWindow::on_cellularButton_clicked()  { adjustTabs(mExtendedUi->cellularButton,  4); }
void ExtendedWindow::on_fingerButton_clicked()    { adjustTabs(mExtendedUi->fingerButton,    5); }
void ExtendedWindow::on_sdButton_clicked()        { adjustTabs(mExtendedUi->sdButton,        6); }

void ExtendedWindow::adjustTabs(QPushButton *thisButton, int thisIndex)
{
    // Make all the tab buttons the same except for the one whose
    // pane is on top.
    QString colorStyle("text-align: left; background-color:");
    colorStyle += mThemeIsDark ? DARK_BACKGROUND : LIGHT_BACKGROUND;

    mExtendedUi->batteryButton  ->setStyleSheet(colorStyle);
    mExtendedUi->cellularButton ->setStyleSheet(colorStyle);
    mExtendedUi->fingerButton   ->setStyleSheet(colorStyle);
    mExtendedUi->locationButton ->setStyleSheet(colorStyle);
    mExtendedUi->messageButton  ->setStyleSheet(colorStyle);
    mExtendedUi->sdButton       ->setStyleSheet(colorStyle);
    mExtendedUi->telephoneButton->setStyleSheet(colorStyle);

    QString activeStyle("text-align: left; background-color:");
    activeStyle += mThemeIsDark ? DARK_ACTIVE : LIGHT_ACTIVE;
    thisButton->setStyleSheet(activeStyle);

    mExtendedUi->batteryButton  ->setAutoFillBackground(true);
    mExtendedUi->cellularButton ->setAutoFillBackground(true);
    mExtendedUi->fingerButton   ->setAutoFillBackground(true);
    mExtendedUi->locationButton ->setAutoFillBackground(true);
    mExtendedUi->messageButton  ->setAutoFillBackground(true);
    mExtendedUi->sdButton       ->setAutoFillBackground(true);
    mExtendedUi->telephoneButton->setAutoFillBackground(true);

    thisButton->clearFocus(); // It looks better when not highlighted
    mExtendedUi->stackedWidget->setCurrentIndex(thisIndex);
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

void ExtendedWindow::setButtonEnabled(QPushButton *theButton, bool isEnabled)
{
    theButton->setEnabled(isEnabled);
    // If this button has icon image properties, reset its
    // image.
    QString enabledPropStr  = theButton->property("themeIconName"         ).toString();
    QString disabledPropStr = theButton->property("themeIconName_disabled").toString();
    if ( !enabledPropStr.isNull() ) {
        // It has at least an 'enabled' icon name.
        // Select light/dark and enabled/disabled
        QString resName = ":/";
        resName += mThemeIsDark ? "light/" : "dark/"; // Theme is dark ==> icon is light
        resName += (isEnabled || disabledPropStr.isNull()) ? enabledPropStr : disabledPropStr;
        theButton->setIcon(QIcon(resName));
    }
}
