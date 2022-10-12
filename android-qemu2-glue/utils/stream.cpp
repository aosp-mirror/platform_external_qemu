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

#include "android-qemu2-glue/utils/stream.h"

#include "aemu/base/files/Stream.h"
#include "android-qemu2-glue/base/files/QemuFileStream.h"

using android::qemu::QemuFileStream;
typedef ::Stream CStream;

CStream* stream_from_qemufile(QEMUFile* file) {
    if (!file) {
        return NULL;
    }
    return reinterpret_cast<CStream*>(new QemuFileStream(file));
}
