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

#include "android/error-messages.h"

extern const char* const kHaxVcpuSyncFailed =
        "Unfortunately, VirtualBox 4.3.30+ does not allow multiple hypervisors "
        "to co-exist.  In order for VirtualBox and the Android Emulator to "
        "co-exist, VirtualBox must change back to shared use.  Please ask "
        "VirtualBox to consider this change here: "
        "https://www.virtualbox.org/ticket/14294";
