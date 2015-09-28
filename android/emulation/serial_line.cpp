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

#include "android/emulation/serial_line.h"

#include "android/emulation/SerialLine.h"

static android::SerialLine* asClass(CSerialLine* sl) {
    return static_cast<android::SerialLine*>(sl);
}

int android_serialline_write(CSerialLine* sl, const uint8_t* data, int len) {
    return asClass(sl)->write(data, len);
}

void android_serialline_addhandlers(CSerialLine* sl, void* opaque,
                                    SLCanReadHandler* canReadCallback,
                                    SLReadHandler* readCallback) {
    asClass(sl)->addHandlers(opaque, canReadCallback, readCallback);
}
