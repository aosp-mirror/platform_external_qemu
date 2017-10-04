/* Copyright (C) 2017 The Android Open Source Project
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
#include "android/cros.h"
#include "android/utils/debug.h"

#include <stdlib.h>
#include <string.h>

CSerialLine* cros_serial_line;

#define  D(...) VERBOSE_PRINT(events,__VA_ARGS__)

void
cros_send_message(const char*  message)
{
    if (message == NULL)
        return;

    D("sending '%s'", message);

    if (cros_serial_line == NULL) {
        D("missing cros channel, ignored");
        return;
    }

    android_serialline_write( cros_serial_line, (const void*)message, strlen(message) );
    android_serialline_write( cros_serial_line, (const void*)"\n", 1 );
}
