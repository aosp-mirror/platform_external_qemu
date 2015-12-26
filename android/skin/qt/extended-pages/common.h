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
#include <QPushButton>

// Sets the button state to enabled or disabled, and sets the
// corresponding icon for the button.
void setButtonEnabled(QPushButton*  button, SettingsTheme theme, bool isEnabled);