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

#include "android/qemu/utils/stream.h"

#include "android/base/files/Stream.h"
#include "android/qemu/base/files/QemuFileStream.h"

extern "C" void timer_put(struct QEMUFile*, struct QEMUTimer*);
extern "C" void timer_get(struct QEMUFile*, struct QEMUTimer*);

using android::qemu::QemuFileStream;
using ::Stream;

Stream* stream_from_qemufile(struct QEMUFile* file) {
    if (!file) {
        return NULL;
    }
    return reinterpret_cast< ::Stream*>(new QemuFileStream(file));
}

QemuFileStream* asQemuFileStream(Stream* stream) {
    return reinterpret_cast<QemuFileStream*>(stream);
}

void stream_put_qemu_timer(Stream* stream, struct QEMUTimer* timer) {
    timer_put(asQemuFileStream(stream)->file(), timer);
}

void stream_get_qemu_timer(Stream* stream, struct QEMUTimer* timer) {
    timer_get(asQemuFileStream(stream)->file(), timer);
}
