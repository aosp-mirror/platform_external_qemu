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

#include "android-qemu2-glue/emulation/CharSerialLine.h"

extern "C" {
#include "sysemu/char.h"
}

namespace android {
namespace qemu1 {

CharSerialLine::CharSerialLine(CharDriverState* cs) : mCs(cs) { }

CharSerialLine::~CharSerialLine() {
    if (mCs) {
        qemu_chr_delete(mCs);
    }
}

void CharSerialLine::addHandlers(void* opaque, CanReadFunc canReadFunc, ReadFunc readFunc) {
    qemu_chr_add_handlers(mCs, canReadFunc, readFunc, NULL, opaque);
}

int CharSerialLine::write(const uint8_t* data, int len) {
    return qemu_chr_fe_write(mCs, data, len);
}

}  // namespace qemu1
}  // namespace android
