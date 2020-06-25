// Copyright (C) 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/car-rotary-page.h"

#include <QtCore/qglobal.h>                                 // for Q_OS_MAC
#include <qcoreevent.h>                                     // for QEvent (ptr only)
#include <qsize.h>                                          // for operator*
#include <qstring.h>                                        // for operator+, operat...
#include <QBitmap>                                          // for QBitmap
#include <QDesktopWidget>                                   // for QDesktopWidget
#include <QEvent>                                           // for QEvent
#include <QPixmap>                                          // for QPixmap
#include <QPushButton>                                      // for QPushButton
#include <QSize>                                            // for QSize

#include "android/globals.h"                                // for android_hw
#include "android/skin/qt/emulator-qt-window.h"             // for EmulatorQtWindow
#include "android/emulation/control/adb/AdbInterface.h"     // for AdbInterface
#include "android/base/system/System.h"                     // for System

class QObject;
class QPushButton;
class QWidget;

using android::base::System;
using android::emulation::AdbInterface;
using android::emulation::OptionalAdbCommandResult;
using std::string;

static const System::Duration kAdbCommandTimeoutMs = 5000;

#ifndef __APPLE__
#include <QApplication>
#include <QScreen>
#endif

CarRotaryPage::CarRotaryPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::CarRotaryPage()),
    mEmulatorWindow(NULL)
{
    mUi->setupUi(this);

    bool hw_rotary = true;

    if (hw_rotary) {
        const struct {
            QPushButton* button;
            string cmd;
        } buttons[] {
            {mUi->dpad_upButton,  "cmd car_service inject-key 280"},
            {mUi->dpad_downButton, "cmd car_service inject-key 281"},
            {mUi->dpad_leftButton, "cmd car_service inject-key 282"},
            {mUi->dpad_rightButton, "cmd car_service inject-key 283"},
            {mUi->dpad_selectButton, "cmd car_service inject-key 23"},
            {mUi->dpad_backButton, "cmd car_service inject-rotary"}, // counterclockwise
            {mUi->dpad_playButton, "input keyevent 4"}, // back
            {mUi->dpad_forwardButton, "cmd car_service inject-rotary -c true"}, // clockwise
        };

        for (const auto& button_info : buttons) {
            QPushButton* button = button_info.button;
            const string cmd = button_info.cmd;
            connect(button, &QPushButton::pressed,
                    [button, cmd, this]() {toggleButtonPressed(button, cmd); });
            connect(button, &QPushButton::released,
                    [button, cmd, this]() {toggleButtonReleased(button, cmd); });
        }

        // Don't show the overlay that obscures the buttons.
        mUi->dpad_noDPad_mask->hide();
        // Don't show the text saying that the D-Pad is disabled.
        mUi->dpad_noDPad_message->hide();
    } else {
        // This AVD does not have a D-Pad.
        // Overlay a mask and text saying that the D-Pad is disabled.
        mUi->dpad_noDPad_mask->raise();
        mUi->dpad_noDPad_message->raise();
    }

    remaskButtons();
    installEventFilter(this);
}

void CarRotaryPage::setAdbInterface(android::emulation::AdbInterface* adb) {
    mAdb = adb;
}

void CarRotaryPage::setEmulatorWindow(EmulatorQtWindow* eW)
{
    mEmulatorWindow = eW;
}

void CarRotaryPage::toggleButtonPressed(QPushButton* button, const string cmd) {
    // TODO(agathaman): Add logic for long-press
}

void CarRotaryPage::toggleButtonReleased(QPushButton* button, const string cmd) {

    mAdb->runAdbCommand(
            {"shell", cmd},
            [this](const OptionalAdbCommandResult& result) {
                string line;
                if (result && result->output &&
                    getline(*(result->output), line)) {
                    LOG(WARNING) << "Executed cmd complete. Result: " << line;
                } else {
                    LOG(WARNING) << "Executed cmd. Error";
                }
            },
            kAdbCommandTimeoutMs, true);
}

void CarRotaryPage::remaskButtons() {
    for (QPushButton* button : findChildren<QPushButton*>()) {
        const QString icon_name = button->property("themeIconName").toString();
        if (!icon_name.isNull()) {
            // Mask the button to only its non-transparent pixels
            // (Note we care only about the shape of the button
            //  here, so :/dark/ and :/light/ are equivalent.)
            //
            // Caution: This requires that the button icon be displayed
            //          at its natural size.
            const QPixmap mask_pixmap(":/light/" + icon_name);

            // HACK: On windows/linux, on normal-density screens,
            // and on OS X with retina display,
            // the mask will end up being too big for the button, so it needs
            // to be scaled down.
            double mask_scale = 0.5;
#ifndef Q_OS_MAC
            int screen_num = QApplication::desktop()->screenNumber(this);
            if (screen_num >= 0 && screen_num <  QApplication::screens().size()) {
                QScreen *scr = QApplication::screens().at(screen_num);
                double dpr = scr->logicalDotsPerInch() / SizeTweaker::BaselineDpi;
                mask_scale *= dpr;
            }
#endif
            button->setMask(mask_pixmap.mask().scaled(mask_pixmap.size() * mask_scale));
            button->setStyleSheet("border: none;");
        }
    }
}

bool CarRotaryPage::eventFilter(QObject* o, QEvent* event) {
    if (event->type() == QEvent::ScreenChangeInternal) {
        // When moved across screens, masks on buttons need to
        // be adjusted according to screen density.
        remaskButtons();
    }
    return QWidget::eventFilter(o, event);
}