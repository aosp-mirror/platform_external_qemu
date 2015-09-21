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

struct QEMUFile;
struct QEMUTimer;

using android::qemu::QemuFileStream;
typedef ::Stream CStream;

CStream* stream_from_qemufile(QEMUFile* file) {
    if (!file) {
        return NULL;
    }
    return reinterpret_cast<CStream*>(new QemuFileStream(file));
}

QemuFileStream* asQemuFileStream(CStream* stream) {
    return reinterpret_cast<QemuFileStream*>(stream);
}

// NOTE: stream_put_qemu_timer() and stream_get_qemu_timer() are temporary
// fixes that will disappear once we remove the use of QEMUTimer instances
// from android/. Including "qemu-common.h" here is problematic, the compiler
// will error complaining about missing endianess macro definitions for some
// obscure reason. Given that this is a temporary fix, just declare these
// two functions here.
extern "C" void timer_put(QEMUFile*, QEMUTimer*);
extern "C" void timer_get(QEMUFile*, QEMUTimer*);

void stream_put_qemu_timer(CStream* stream, QEMUTimer* timer) {
    timer_put(asQemuFileStream(stream)->file(), timer);
}

void stream_get_qemu_timer(CStream* stream, QEMUTimer* timer) {
    timer_get(asQemuFileStream(stream)->file(), timer);
}
