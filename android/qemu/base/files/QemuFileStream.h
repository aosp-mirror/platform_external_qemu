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

#ifndef ANDROID_QEMU_BASE_FILES_QEMU_FILE_STREAM_H
#define ANDROID_QEMU_BASE_FILES_QEMU_FILE_STREAM_H

#include "android/base/Compiler.h"
#include "android/base/files/Stream.h"

struct QEMUFile;

namespace android {
namespace qemu {

// A simple wrapper around QEMUFile* that implements the android::base::Stream
// interface. Note that the instance doesn't own the QEMUFile*, i.e. the
// destructor will never close it.
class QemuFileStream : public android::base::Stream {
public:
    explicit QemuFileStream(QEMUFile* file);
    virtual ~QemuFileStream();
    virtual ssize_t read(void* buffer, size_t len);
    virtual ssize_t write(const void* buffer, size_t len);

private:
    QemuFileStream();
    DISALLOW_COPY_AND_ASSIGN(QemuFileStream);

    QEMUFile* mFile;
};

}  // namespace qemu
}  // namespace android

#endif  // ANDROID_QEMU_BASE_FILES_QEMU_FILE_STREAM_H
