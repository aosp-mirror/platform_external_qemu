// Copyright 2018 The Android Open Source Project
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

#include "android/utils/compiler.h"
#include "android/avd/info.h"
#include <glib.h>

ANDROID_BEGIN_HEADER

#include "qemu/osdep.h"
#include "sysemu/blockdev.h"

extern int android_drive_share_init(bool wipe_data,
                                    bool read_only,
                                    const char* snapshot_name,
                                    BlockInterfaceType blockDefaultType,
                                    const AvdInfo* avdInfo);

ANDROID_END_HEADER
