/* Copyright (C) 2018 The Android Open Source Project
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
#pragma once

#include "android/emulation/control/vm_operations.h"

#ifdef _WIN32
extern "C" int qt_main(int, char**) { return 0; }
extern "C" void qemu_system_shutdown_request(QemuShutdownCause reason) {}
#endif  // _WIN32
