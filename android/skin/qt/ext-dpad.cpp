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

#define ON_CLICKED(buttonName, key, modifiers) \
    void ExtendedWindow::on_dpad_ ## buttonName ## Button_clicked() \
    { \
        mParentWindow->simulateKeyPress(key, modifiers); \
    }

#define ON_PRESSED(buttonName) \
    void ExtendedWindow::on_dpad_ ## buttonName ## Button_pressed() \
    { \
        dpad_setPressed(mExtendedUi->dpad_ ## buttonName ## Button); \
    }

#define ON_RELEASED(buttonName) \
    void ExtendedWindow::on_dpad_ ## buttonName ## Button_released() \
    { \
        dpad_setReleased(mExtendedUi->dpad_ ## buttonName ## Button); \
    }

ON_CLICKED(back,    KEY_REWIND,      0);
ON_CLICKED(down,    KEY_KP2,         0);
ON_CLICKED(forward, KEY_FASTFORWARD, 0);
ON_CLICKED(left,    KEY_KP4,         0);
ON_CLICKED(play,    KEY_PLAY,        0);
ON_CLICKED(right,   KEY_KP6,         0);
ON_CLICKED(select,  KEY_KP5,         0);
ON_CLICKED(up,      KEY_KP8,         0);

ON_PRESSED(back);
ON_PRESSED(down);
ON_PRESSED(forward);
ON_PRESSED(left);
ON_PRESSED(play);
ON_PRESSED(right);
ON_PRESSED(select);
ON_PRESSED(up);

ON_RELEASED(back);
ON_RELEASED(down);
ON_RELEASED(forward);
ON_RELEASED(left);
ON_RELEASED(play);
ON_RELEASED(right);
ON_RELEASED(select);
ON_RELEASED(up);

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
