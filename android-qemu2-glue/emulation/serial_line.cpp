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

#include "android-qemu2-glue/emulation/serial_line.h"

#include "android-qemu2-glue/emulation/charpipe.h"
#include "android-qemu2-glue/emulation/CharSerialLine.h"

using android::qemu2::CharSerialLine;
using android::SerialLine;

CSerialLine* android_serialline_from_cs(Chardev* dev) {
    return new CharSerialLine(dev);
}

Chardev* android_serialline_get_cs(CSerialLine* sl) {
    return static_cast<CharSerialLine*>(sl)->state();
}

static SerialLine* qemu_serialline_buffer_open(SerialLine* sl) {
    Chardev* dev = static_cast<CharSerialLine*>(sl)->state();
    return new CharSerialLine(qemu_chr_open_buffer(dev));
}

static bool qemu_serialline_pipe_open(SerialLine** pfirst,
                                      SerialLine** psecond) {
    Chardev* first_dev = nullptr;
    Chardev* second_dev = nullptr;

    if (qemu_chr_open_charpipe(&first_dev, &second_dev)) {
        *pfirst = nullptr;
        *psecond = nullptr;
        return false;
    }

    *pfirst = new CharSerialLine(first_dev);
    *psecond = new CharSerialLine(second_dev);
    return true;
}

void qemu2_android_serialline_init() {

    static const android::SerialLine::Funcs kQemuSerialLineFuncs = {
        .openBuffer = qemu_serialline_buffer_open,
        .openPipe = qemu_serialline_pipe_open
    };

    android::SerialLine::Funcs::reset(&kQemuSerialLineFuncs);
}
