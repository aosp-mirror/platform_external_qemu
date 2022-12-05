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
#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif
#include "android-qemu2-glue/base/files/QemuFileStream.h"

#include "aemu/base/Log.h"

extern "C" {
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "exec/cpu-common.h"
#include "migration/qemu-file.h"
}

namespace android {
namespace qemu {

QemuFileStream::QemuFileStream(QEMUFile* file) : mFile(file) {
    mSnapshotPb =
            static_cast<emulator_compatible::Snapshot*>(qemu_file_get_pb(file));
}

QemuFileStream::~QemuFileStream() {
    mSnapshotPb = nullptr;
}

void* QemuFileStream::getProtobuf() {
    return mSnapshotPb;
}

ssize_t QemuFileStream::read(void* buffer, size_t len) {
    DCHECK(static_cast<ssize_t>(len) >= 0);
    DCHECK(static_cast<ssize_t>(len) == static_cast<int>(len));
    return static_cast<ssize_t>(
            qemu_get_buffer(mFile,
                            static_cast<uint8_t*>(buffer),
                            static_cast<int>(len)));
}

ssize_t QemuFileStream::write(const void* buffer, size_t len) {
    DCHECK(static_cast<ssize_t>(len) >= 0);
    DCHECK(static_cast<ssize_t>(len) == static_cast<int>(len));
    qemu_put_buffer(mFile,
                    static_cast<const uint8_t*>(buffer),
                    static_cast<int>(len));
    // There is no way to know if all could be written, so always
    // return success here.
    return static_cast<ssize_t>(len);
}

}  // namespace qemu
}  // namespace android
