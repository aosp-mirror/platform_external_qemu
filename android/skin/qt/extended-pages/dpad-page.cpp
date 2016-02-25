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

#include "android/emulation/control/user_event_agent.h"
#include "android/settings-agent.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/qt-settings.h"
#include <QBitmap>
#include <QSettings>

DPadPage::DPadPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::DPadPage())
{
    mUi->setupUi(this);

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
        {mUi->dpad_playButton, kKeyCodePlay},
        {mUi->dpad_forwardButton, kKeyCodeFastForward},
    };

    for (const auto& button_info : buttons) {
        QPushButton* button = button_info.button;
        const QString icon_name = button->property("themeIconName").toString();
        if (!icon_name.isNull()) {
            // Mask the button to only its non-transparent pixels
            // (Note we care only about the shape of the button
            //  here, so :/dark/ and :/light/ are equivalent.)
            //
            // Caution: This requires that the button icon be displayed
            //          at its natural size.
            const QPixmap mask_pixmap(":/dark/" + icon_name);
            button->setMask(mask_pixmap.mask());
            button->setStyleSheet("border: none;");
        }
        const SkinKeyCode key_code = button_info.key_code;
        connect(button, &QPushButton::pressed,
                [button, key_code, this]() { toggleButtonPressed(button, key_code, true); });
        connect(button, &QPushButton::released,
                [button, key_code, this]() { toggleButtonPressed(button, key_code, false); });
    }
}

void DPadPage::setUserEventsAgent(const QAndroidUserEventAgent* agent)
{
    mUserEventsAgent = agent;
}

void DPadPage::toggleButtonPressed(
    QPushButton* button,
    const SkinKeyCode key_code,
    const bool pressed) {
    if (mUserEventsAgent) {
        mUserEventsAgent->sendKey(key_code, pressed);
    }

    QSettings settings;
    SettingsTheme theme =
        (SettingsTheme)settings.value(Ui::Settings::UI_THEME, 0).toInt();

    QString iconName =
        button->property(pressed ? "themeIconNamePressed" : "themeIconName").toString();
    if ( !iconName.isNull() ) {
        QString resName =
            ":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] + "/" + iconName;
        button->setIcon(QIcon(resName));
    }
}

