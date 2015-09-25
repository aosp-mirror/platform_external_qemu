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

#include "ui_extended.h"

#include "android/settings-agent.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "extended-window.h"
#include "extended-window-styles.h"

#include <QtWidgets>

void ExtendedWindow::initSettings()
{ }

void ExtendedWindow::on_set_themeBox_currentIndexChanged(int index)
{
    // Select either the light or dark theme
    SettingsTheme theme = (SettingsTheme)index;

    if (theme < 0 || theme >= SETTINGS_THEME_NUM_ENTRIES) {
        // Out of range--ignore
        return;
    }

    // Save a valid selection
    mSettingsState.mTheme = theme;

    // Switch to the icon images that are appropriate for this theme.
    // Examine every widget.
    QWidgetList wList = QApplication::allWidgets();
    for (int idx = 0; idx < wList.size(); idx++) {
        QPushButton *pB = dynamic_cast<QPushButton*>(wList[idx]);
        if (pB && !pB->icon().isNull()) {
            setButtonEnabled(pB, pB->isEnabled());
        }
    }

    // Set the style for this theme
    switch (theme) {
        case SETTINGS_THEME_DARK:
            this->setStyleSheet(QT_STYLE(DARK));
            break;
        case SETTINGS_THEME_LIGHT:
        default:
            this->setStyleSheet(QT_STYLE(LIGHT));
            break;
    }

    // Force a re-draw to make the new style take effect
    this->style()->unpolish(mExtendedUi->stackedWidget);
    this->style()->polish(mExtendedUi->stackedWidget);
    this->update();

    // Make the Settings pane active (still)
    adjustTabs(mExtendedUi->settingsButton, PANE_IDX_SETTINGS);
}


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
        resName += (mSettingsState.mTheme == SETTINGS_THEME_DARK) ?
                          "light/" : "dark/"; // Theme is dark ==> icon is light
        resName += (isEnabled || disabledPropStr.isNull()) ? enabledPropStr : disabledPropStr;
        theButton->setIcon(QIcon(resName));
    }
}
