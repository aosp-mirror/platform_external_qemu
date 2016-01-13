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

#include "android/settings-agent.h"
#include <QIcon>
#include <QPushButton>
#include <QString>

// Sets the button state to enabled or disabled, and sets the
// corresponding icon for the button.
void setButtonEnabled(QPushButton*  button, SettingsTheme theme, bool isEnabled);


// Get the path to the folder where screenshots are saved.
QString getScreenshotSaveDirectory();

// Get the currently active UI theme.
SettingsTheme getSelectedTheme();

// Changes icons on all push buttons to fit the given theme.
void switchAllIconsForTheme(SettingsTheme theme);

// Obtains the given icon for the current theme.
QIcon getIconForCurrentTheme(const QString& icon_name);
