// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#pragma once

// adb_key_preset presets one adb key in the guest.
// This is to stop the guest from pop up the host key confirmation window,
// to make it easier for automated tests.
//
// This function is not thread-safe. Please only call it in the main thread.
// There can only be one preset adb key simultaneously.
void adb_key_preset(const char* adbKey);

// adb_key_preset_init_service initializes the adb key preset service.
// It will be called automatically by adb_key_preset.
// This function is not thread-safe. Please only call it in the main thread.
void adb_key_preset_init_service();
