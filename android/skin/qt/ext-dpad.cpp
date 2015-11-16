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


#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "extended-window.h"
#include "extended-window-styles.h"
#include "ui_extended.h"

#include <QBitmap>

#define ON_PRESSED(buttonName, keyCode) \
    void ExtendedWindow::on_dpad_ ## buttonName ## Button_pressed() \
    { \
        if (mUserEventsAgent) { \
            mUserEventsAgent->sendKey(keyCode, 1); \
        } \
        dpad_setPressed(mExtendedUi->dpad_ ## buttonName ## Button); \
    }

#define ON_RELEASED(buttonName, keyCode) \
    void ExtendedWindow::on_dpad_ ## buttonName ## Button_released() \
    { \
        if (mUserEventsAgent) { \
            mUserEventsAgent->sendKey(keyCode, 0); \
        } \
        dpad_setReleased(mExtendedUi->dpad_ ## buttonName ## Button); \
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

void ExtendedWindow::initDPad()
{
    // Set all DPad buttons to encompass only
    // their non-transparent regions

    QPushButton* theButtons[] = { mExtendedUi->dpad_leftButton,
                                  mExtendedUi->dpad_upButton,
                                  mExtendedUi->dpad_rightButton,
                                  mExtendedUi->dpad_downButton,
                                  mExtendedUi->dpad_selectButton,
                                  mExtendedUi->dpad_backButton,
                                  mExtendedUi->dpad_playButton,
                                  mExtendedUi->dpad_forwardButton };

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

void ExtendedWindow::dpad_setPressed(QPushButton* button)
{
    QString iconName = button->property("themeIconNamePressed").toString();
    if ( !iconName.isNull() ) {
        QString resName = ":/";
        resName += (mSettingsState.mTheme == SETTINGS_THEME_DARK) ?
                        DARK_PATH : LIGHT_PATH;
        resName += "/";
        resName += iconName;
        button->setIcon(QIcon(resName));
    }
}

void ExtendedWindow::dpad_setReleased(QPushButton* button)
{
    QString iconName = button->property("themeIconName").toString();
    if ( !iconName.isNull() ) {
        QString resName = ":/";
        resName += (mSettingsState.mTheme == SETTINGS_THEME_DARK) ?
                        DARK_PATH : LIGHT_PATH;
        resName += "/";
        resName += iconName;
        button->setIcon(QIcon(resName));
    }
    // De-select the button (it looks better)
    button->clearFocus();
}
