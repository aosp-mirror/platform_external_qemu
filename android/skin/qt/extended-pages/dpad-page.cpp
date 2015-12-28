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
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/qt-settings.h"
#include <QBitmap>
#include <QSettings>

DPadPage::DPadPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::DPadPage())
{
    mUi->setupUi(this);

    // Set all DPad buttons to encompass only
    // their non-transparent regions

    QPushButton* theButtons[] = { mUi->dpad_leftButton,
                                  mUi->dpad_upButton,
                                  mUi->dpad_rightButton,
                                  mUi->dpad_downButton,
                                  mUi->dpad_selectButton,
                                  mUi->dpad_backButton,
                                  mUi->dpad_playButton,
                                  mUi->dpad_forwardButton };

    for (unsigned int idx = 0; idx < sizeof(theButtons)/sizeof(theButtons[0]); idx++) {
        QPushButton* button = theButtons[idx];
        // Get the button's icon image
        QString iconName = button->property("themeIconName").toString();
        if ( !iconName.isNull() ) {
            // Mask the button to only its non-transparent pixels
            // (Note we care only about the shape of the button
            //  here, so :/dark/ and :/light/ are equivalent.)
            //
            // Caution: This requires that the button icon be displayed
            //          at its natural size.
            QPixmap pixMap(":/dark/" + iconName);
            button->setMask(pixMap.mask());
        }
    }
}

#define ON_PRESSED(buttonName, keyCode) \
    void DPadPage::on_dpad_ ## buttonName ## Button_pressed() \
    { \
        if (mUserEventsAgent) { \
            mUserEventsAgent->sendKey(keyCode, 1); \
        } \
        dpad_setPressed(mUi->dpad_ ## buttonName ## Button); \
    }

#define ON_RELEASED(buttonName, keyCode) \
    void DPadPage::on_dpad_ ## buttonName ## Button_released() \
    { \
        if (mUserEventsAgent) { \
            mUserEventsAgent->sendKey(keyCode, 0); \
        } \
        dpad_setReleased(mUi->dpad_ ## buttonName ## Button); \
    }

ON_PRESSED(back, kKeyCodeRewind);
ON_PRESSED(down, kKeyCodeDpadDown);
ON_PRESSED(forward, kKeyCodeFastForward);
ON_PRESSED(left, kKeyCodeDpadLeft);
ON_PRESSED(play, kKeyCodePlay);
ON_PRESSED(right, kKeyCodeDpadRight);
ON_PRESSED(select, kKeyCodeDpadCenter);
ON_PRESSED(up, kKeyCodeDpadUp);

ON_RELEASED(back, kKeyCodeRewind);
ON_RELEASED(down, kKeyCodeDpadDown);
ON_RELEASED(forward, kKeyCodeFastForward);
ON_RELEASED(left, kKeyCodeDpadLeft);
ON_RELEASED(play, kKeyCodePlay);
ON_RELEASED(right, kKeyCodeDpadRight);
ON_RELEASED(select, kKeyCodeDpadCenter);
ON_RELEASED(up, kKeyCodeDpadUp);

void DPadPage::dpad_setPressed(QPushButton* button)
{
    QSettings settings;
    SettingsTheme theme =
        (SettingsTheme)settings.value(Ui::Settings::UI_THEME, 0).toInt();

    QString iconName = button->property("themeIconNamePressed").toString();
    if ( !iconName.isNull() ) {
        QString resName = ":/";
        resName += (theme == SETTINGS_THEME_DARK) ?
                        DARK_PATH : LIGHT_PATH;
        resName += "/";
        resName += iconName;
        button->setIcon(QIcon(resName));
    }
}

void DPadPage::dpad_setReleased(QPushButton* button)
{
    QSettings settings;
    SettingsTheme theme =
        (SettingsTheme)settings.value(Ui::Settings::UI_THEME, 0).toInt();

    QString iconName = button->property("themeIconName").toString();
    if ( !iconName.isNull() ) {
        QString resName = ":/";
        resName += (theme == SETTINGS_THEME_DARK) ?
                        DARK_PATH : LIGHT_PATH;
        resName += "/";
        resName += iconName;
        button->setIcon(QIcon(resName));
    }
    // De-select the button (it looks better)
    button->clearFocus();
}

void DPadPage::setUserEventsAgent(const QAndroidUserEventAgent* agent) 
{
    mUserEventsAgent = agent;
}