// Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/dpad-page.h"

#include <QtCore/qglobal.h>                         // for Q_OS_MAC
#include <qcoreevent.h>                             // for QEvent (ptr only)
#include <qsize.h>                                  // for operator*
#include <qstring.h>                                // for operator+, operat...
#include <stddef.h>                                 // for NULL
#include <QAbstractButton>                          // for QAbstractButton
#include <QBitmap>                                  // for QBitmap
#if QT_VERSION >= 0x060000
#else
#include <QDesktopWidget>                           // for QDesktopWidget
#endif  // QT_VERSION
#include <QEvent>                                   // for QEvent
#include <QHash>                                    // for QHash
#include <QIcon>                                    // for QIcon
#include <QLabel>                                   // for QLabel
#include <QList>                                    // for QList
#include <QPixmap>                                  // for QPixmap
#include <QPushButton>                              // for QPushButton
#include <QSettings>                                // for QSettings
#include <QSize>                                    // for QSize
#include <QVariant>                                 // for QVariant


#include "host-common/hw-config.h"
#include "android/console.h"                        // for android_hw
#include "android/settings-agent.h"                 // for SettingsTheme
#include "android/skin/event.h"                     // for SkinEvent, (anony...
#include "android/skin/keycode.h"                   // for kKeyCodeDpadCenter
#include "android/skin/qt/emulator-qt-window.h"     // for EmulatorQtWindow
#include "android/skin/qt/extended-pages/common.h"  // for getSelectedTheme
#include "android/skin/qt/stylesheet.h"             // for stylesheetValues

class QObject;
class QPushButton;
class QWidget;

#ifndef __APPLE__
#include <QApplication>
#include <QScreen>
#endif

DPadPage::DPadPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::DPadPage()),
    mEmulatorWindow(NULL)
{
    mUi->setupUi(this);

    if (getConsoleAgents()->settings->hw()->hw_dPad) {
        // This AVD has a D-Pad. Set up the control.
        const struct {
            QPushButton* button;
            SkinKeyCode key_code;
        } buttons[] = {
            {mUi->dpad_leftButton, kKeyCodeDpadLeft},
            {mUi->dpad_upButton, kKeyCodeDpadUp},
            {mUi->dpad_rightButton, kKeyCodeDpadRight},
            {mUi->dpad_downButton, kKeyCodeDpadDown},
            {mUi->dpad_selectButton, kKeyCodeDpadCenter},
            {mUi->dpad_backButton, kKeyCodeRewind},
            {mUi->dpad_playButton, kKeyCodePlaypause},
            {mUi->dpad_forwardButton, kKeyCodeFastForward},
        };

        for (const auto& button_info : buttons) {
            QPushButton* button = button_info.button;
            const SkinKeyCode key_code = button_info.key_code;
            connect(button, &QPushButton::pressed,
                    [button, key_code, this]() { toggleButtonPressed(button, key_code, true); });
            connect(button, &QPushButton::released,
                    [button, key_code, this]() { toggleButtonPressed(button, key_code, false); });
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

void DPadPage::setEmulatorWindow(EmulatorQtWindow* eW)
{
    mEmulatorWindow = eW;
}

void DPadPage::toggleButtonPressed(
    QPushButton* button,
    const SkinKeyCode key_code,
    const bool pressed) {
    if (mEmulatorWindow) {
        SkinEvent skin_event = createSkinEvent(pressed ? kEventKeyDown : kEventKeyUp);
        skin_event.u.key.keycode = key_code;
        skin_event.u.key.mod = 0;
        mEmulatorWindow->queueSkinEvent(std::move(skin_event));
    }

    QSettings settings;
    SettingsTheme theme = getSelectedTheme();

    QString iconName =
        button->property(pressed ? "themeIconNamePressed" : "themeIconName").toString();
    if ( !iconName.isNull() ) {
        QString resName =
            ":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] + "/" + iconName;
        button->setIcon(QIcon(resName));
    }
}

void DPadPage::remaskButtons() {
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
#if QT_VERSION >= 0x060000
            button->setMask(QBitmap::fromPixmap(
                    mask_pixmap.mask().scaled(mask_pixmap.size() * mask_scale)));
#else
            button->setMask(mask_pixmap.mask().scaled(mask_pixmap.size() * mask_scale));
#endif
            button->setStyleSheet("border: none;");
        }
    }
}

bool DPadPage::eventFilter(QObject* o, QEvent* event) {
    if (event->type() == QEvent::ScreenChangeInternal) {
        // When moved across screens, masks on buttons need to
        // be adjusted according to screen density.
        remaskButtons();
    }
    return QWidget::eventFilter(o, event);
}
