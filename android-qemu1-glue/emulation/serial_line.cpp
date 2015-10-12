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

#include "android-qemu1-glue/emulation/serial_line.h"

#include "android-qemu1-glue/emulation/charpipe.h"
#include "android-qemu1-glue/emulation/CharSerialLine.h"

using android::qemu1::CharSerialLine;

CSerialLine* android_serialline_from_cs(CharDriverState* cs) {
    return new CharSerialLine(cs);
}

CharDriverState* android_serialline_get_cs(CSerialLine* sl) {
    return static_cast<CharSerialLine*>(sl)->state();
}

CharDriverState* android_serialline_release_cs(CSerialLine* sl) {
    CharSerialLine* csl = static_cast<CharSerialLine*>(sl);
    CharDriverState* result = csl->release();
    delete csl;
    return result;
}

CSerialLine* android_serialline_buffer_open(CSerialLine* sl) {
    CharDriverState* cs = static_cast<CharSerialLine*>(sl)->state();
    return new CharSerialLine(qemu_chr_open_buffer(cs));
}

int android_serialline_pipe_open(CSerialLine** pfirst, CSerialLine** psecond) {
    CharDriverState* first_cs = NULL;
    CharDriverState* second_cs = NULL;

    if (qemu_chr_open_charpipe(&first_cs, &second_cs)) {
        *pfirst = NULL;
        *psecond = NULL;
        return -1;
    }

    *pfirst = new CharSerialLine(first_cs);
    *psecond = new CharSerialLine(second_cs);
    return 0;
}
