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

#include "android/main-common.h"
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

    // Set the UI widget state in case this function
    // was called artificially.
    mExtendedUi->set_themeBox->setCurrentIndex(index);

    // Save a valid selection
    mSettingsState.mTheme = theme;
    user_config_set_ui_theme(theme);

    // Switch to the icon images that are appropriate for this theme.
    switchAllIconsForTheme(theme);

    // Set the style for this theme
    switch (theme) {
        case SETTINGS_THEME_DARK:
            this->setStyleSheet(QT_STYLE(DARK));
            mToolWindow->setStyleSheet(QT_STYLE(DARK));
            break;
        case SETTINGS_THEME_LIGHT:
        default:
            this->setStyleSheet(QT_STYLE(LIGHT));
            mToolWindow->setStyleSheet(QT_STYLE(LIGHT));
            break;
    }

    // Force a re-draw to make the new style take effect
    this->style()->unpolish(mExtendedUi->stackedWidget);
    this->style()->polish(mExtendedUi->stackedWidget);
    this->update();

    // Make the Settings pane active (still)
    adjustTabs(mExtendedUi->settingsButton, PANE_IDX_SETTINGS);
}

// static member function
void ExtendedWindow::switchAllIconsForTheme(SettingsTheme theme)
{
    // Switch to the icon images that are appropriate for this theme.
    // Examine every widget.
    QWidgetList wList = QApplication::allWidgets();
    for (int idx = 0; idx < wList.size(); idx++) {
        QPushButton *pB = dynamic_cast<QPushButton*>(wList[idx]);
        if (pB && !pB->icon().isNull()) {
            setButtonEnabled(pB, theme, pB->isEnabled());
        }
    }
}

// static member function
void ExtendedWindow::setButtonEnabled(QPushButton*  theButton,
                                      SettingsTheme theme,
                                      bool          isEnabled)
{
    theButton->setEnabled(isEnabled);
    // If this button has icon image properties, reset its
    // image.
    QString enabledPropStr  = theButton->property("themeIconName"         ).toString();
    QString disabledPropStr = theButton->property("themeIconName_disabled").toString();

    // Get the resource name based on the light/dark theme
    QString resName = ":/";
    resName += (theme == SETTINGS_THEME_DARK) ? DARK_PATH : LIGHT_PATH;
    resName += "/";

    if (!isEnabled  &&  !disabledPropStr.isNull()) {
        // The button is disabled and has a specific
        // icon for when it is disabled.
        // Apply this icon and tell Qt to use it
        // directly (don't gray it out) even though
        // it is disabled.
        resName += disabledPropStr;
        QIcon theIcon(resName);
        theIcon.addPixmap(QPixmap(resName), QIcon::Disabled);
        theButton->setIcon(theIcon);
    } else if ( !enabledPropStr.isNull() ) {
        // Use the 'enabled' version of the icon
        // (Let Qt grey the icon if it wants to)
        resName += enabledPropStr;
        QIcon theIcon(resName);
        theButton->setIcon(theIcon);
    }
}
