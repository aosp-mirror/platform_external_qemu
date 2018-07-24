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

#include <type_traits>

extern "C" {
#pragma GCC diagnostic push
  #include "qemu/osdep.h"
#pragma GCC diagnostic pop
}

namespace android {
namespace qemu2 {

CharSerialLine::CharSerialLine(Chardev* dev) {
  mBackend = { 0 };
  mBackend.chr = dev;
  dev->be = &mBackend;
}



CharSerialLine::~CharSerialLine() {
    if (mBackend.chr) {
        object_unparent(OBJECT(mBackend.chr));
    }
}

void CharSerialLine::addHandlers(void* opaque, CanReadFunc canReadFunc, ReadFunc readFunc) {
    qemu_chr_fe_set_handlers(&mBackend, canReadFunc, readFunc, NULL, NULL, opaque, NULL, false);
}

int CharSerialLine::write(const uint8_t* data, int len) {
    return qemu_chr_fe_write(&mBackend, data, len);
}

}  // namespace qemu2
}  // namespace android
