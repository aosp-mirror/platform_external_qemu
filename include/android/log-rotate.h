// Copyright (C) 2011 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_LOG_ROTATE_H
#define ANDROID_LOG_ROTATE_H

// This header provides facilities to rotate misc QEMU log files
// use in the Android emulator.

// Setup log rotation, this installed a signal handler for SIGUSR1
// which will trigger a log rotation at runtime.
void qemu_log_rotation_init(void);

// Perform log rotation if needed. For now, this must be called from
// the main event loop, though a future implementation might schedule
// a bottom-half handler instead to achieve the same result.
void qemu_log_rotation_poll(void);

#endif  // ANDROID_LOG_ROTATE_H
