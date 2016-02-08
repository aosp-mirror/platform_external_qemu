//Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/settings-agent.h"
#include <QString>
#include <QHash>

namespace Ui {

enum class EmulatorUITheme {
    Dark,
    Light
};

// Returns a string containing the style sheet for the
// given UI theme.
const QString& stylesheetForTheme(SettingsTheme theme);

// Retuns a string containing the style sheet for fonts.
// |hi_density| should be true for Retina displays.
const QString& fontStylesheet(bool hi_density);

const QHash<QString, QString>& stylesheetValues(SettingsTheme theme);
}
