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

#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/tool-window.h"

#include "ui_extended.h"

#include <QtWidgets>

// Colors for the dark theme
#define DARK_FOREGROUND  "#f0f0f0"
#define DARK_BACKGROUND  "#3c4249"
#define DARK_ACTIVE      "#4c5259"  // A little lighter than the background

// Colors for the light theme
#define LIGHT_FOREGROUND "#0f0f0f"
#define LIGHT_BACKGROUND "#c3bdb6"
#define LIGHT_ACTIVE     "#b3ada6"  // A little darker than the background

// The sliders look ugly without some customization
#define SLIDER_STYLE "QSlider::sub-page:horizontal {background: #888}" \
                     "QSlider::handle:horizontal {background: #eee;" \
                               "border: 2px solid #444; border-radius: 4px }"

// The check boxes also look ugly without some customization
#define DARK_CHECKBOX_STYLE \
    "QCheckBox::indicator:checked   { image: url(:/dark/check_yes); " \
                                     "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QCheckBox::indicator:unchecked { image: url(:/dark/check_no); " \
                                     "icon-size: 24px; height: 24 px; width: 24 px }"

#define LIGHT_CHECKBOX_STYLE \
    "QCheckBox::indicator:checked   { image: url(:/light/check_yes); " \
                                     "icon-size: 24px; height: 24 px; width: 24 px }" \
    "QCheckBox::indicator:unchecked { image: url(:/light/check_no); " \
                                     "icon-size: 24px; height: 24 px; width: 24 px }"

ExtendedWindow::ExtendedWindow(EmulatorQtWindow *eW, ToolWindow *tW, const UiEmuAgent *agentPtr) :
    QFrame(eW),
    parentWindow(eW),
    toolWindow(tW),
    batteryAgent  (agentPtr ? agentPtr->battery   : NULL),
    cellularAgent (agentPtr ? agentPtr->cellular  : NULL),
    fingerAgent   (agentPtr ? agentPtr->finger    : NULL),
    telephonyAgent(agentPtr ? agentPtr->telephony : NULL),
    extendedUi(new Ui::ExtendedControls)
{
    Q_INIT_RESOURCE(resources);

    setWindowFlags(Qt::Tool | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);

    extendedUi->setupUi(this);

    // Do any sub-window-specific initialization
    initBattery();
    initCellular();
    initFinger();
    initTelephony();

    move(parentWindow->geometry().right() + 40,
         parentWindow->geometry().top()   + 40 );
}

void ExtendedWindow::completeInitialization()
{
    // Set the theme to dark (by setting it light and doing a switch-over)
    themeIsDark = false;
    on_theme_pushButton_clicked();
}

ExtendedWindow::~ExtendedWindow()
{
    // Do not delete 'extendedState'--we'll want it if
    // this window gets re-created.
}

void ExtendedWindow::closeEvent(QCloseEvent *ce)
{
    toolWindow->extendedIsClosing();
}

// TODO: This will probably not be implemented by a direct button press
void ExtendedWindow::on_theme_pushButton_clicked()
{
    themeIsDark = !themeIsDark;

    QString style;
    style += SLIDER_STYLE;
    if (themeIsDark) {
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
    this->style()->unpolish(extendedUi->stackedWidget);
    this->style()->polish(extendedUi->stackedWidget);
    this->update();

    // Start with the Battery pane active
    adjustTabs(extendedUi->batteryButton, 0);
}

// Tab buttons. Each raises its stacked pane to the top.

void ExtendedWindow::on_batteryButton_clicked()   { adjustTabs(extendedUi->batteryButton,   0); }
void ExtendedWindow::on_telephoneButton_clicked() { adjustTabs(extendedUi->telephoneButton, 1); }
void ExtendedWindow::on_messageButton_clicked()   { adjustTabs(extendedUi->messageButton,   2); }
void ExtendedWindow::on_locationButton_clicked()  { adjustTabs(extendedUi->locationButton,  3); }
void ExtendedWindow::on_cellularButton_clicked()  { adjustTabs(extendedUi->cellularButton,  4); }
void ExtendedWindow::on_fingerButton_clicked()    { adjustTabs(extendedUi->fingerButton,    5); }
void ExtendedWindow::on_sdButton_clicked()        { adjustTabs(extendedUi->sdButton,        6); }

void ExtendedWindow::adjustTabs(QPushButton *thisButton, int thisIndex)
{
    // Make all the tab buttons the same except for the one whose
    // pane is on top.
    QString colorStyle("text-align: left; background-color:");
    colorStyle += themeIsDark ? DARK_BACKGROUND : LIGHT_BACKGROUND;

    extendedUi->batteryButton  ->setStyleSheet(colorStyle);
    extendedUi->cellularButton ->setStyleSheet(colorStyle);
    extendedUi->fingerButton   ->setStyleSheet(colorStyle);
    extendedUi->locationButton ->setStyleSheet(colorStyle);
    extendedUi->messageButton  ->setStyleSheet(colorStyle);
    extendedUi->sdButton       ->setStyleSheet(colorStyle);
    extendedUi->telephoneButton->setStyleSheet(colorStyle);

    QString activeStyle("text-align: left; background-color:");
    activeStyle += themeIsDark ? DARK_ACTIVE : LIGHT_ACTIVE;
    thisButton->setStyleSheet(activeStyle);

    extendedUi->batteryButton  ->setAutoFillBackground(true);
    extendedUi->cellularButton ->setAutoFillBackground(true);
    extendedUi->fingerButton   ->setAutoFillBackground(true);
    extendedUi->locationButton ->setAutoFillBackground(true);
    extendedUi->messageButton  ->setAutoFillBackground(true);
    extendedUi->sdButton       ->setAutoFillBackground(true);
    extendedUi->telephoneButton->setAutoFillBackground(true);

    thisButton->clearFocus(); // It looks better when not highlighted
    extendedUi->stackedWidget->setCurrentIndex(thisIndex);
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
        resName += themeIsDark ? "light/" : "dark/"; // Theme is dark ==> icon is light
        resName += (isEnabled || disabledPropStr.isNull()) ? enabledPropStr : disabledPropStr;
        theButton->setIcon(QIcon(resName));
    }
}
