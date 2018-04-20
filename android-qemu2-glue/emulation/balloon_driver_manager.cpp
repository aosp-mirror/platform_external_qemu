// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu2-glue/qemu-setup.h"

#include "android/emulation/BalloonDriverManager.h"

extern "C" {
	//#include "qemu/error-report.h"
	#include "qemu/osdep.h"
    #include "qemu-common.h"
    #include "qapi/error.h"
    #include "qapi/qmp/qobject.h"
    #include "qmp-commands.h"
}
#include <stdio.h>

static bool balloon(int64_t target) {
    Error *err = nullptr;
	qmp_balloon(target, &err);
    return err != nullptr;
}

bool qemu_andrid_emulation_start_balloon(int hwRamSize) {
    Error *err = nullptr;
    qmp_query_balloon(&err);
    if (err) {
        //error_report_err(err);
        printf("##### ballon driver is not activated.");
        return false;
    }
    android_start_balloon(hwRamSize, balloon);
    return true;
}


