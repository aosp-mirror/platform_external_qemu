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

#pragma once

#include "aemu/base/Compiler.h"
#include "aemu/base/files/Stream.h"

#include "emulator_compatible.pb.h"
#include "google/protobuf/any.pb.h"

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

    virtual void* getProtobuf();

    QEMUFile* file() const { return mFile; }

private:
    QemuFileStream();
    DISALLOW_COPY_AND_ASSIGN(QemuFileStream);

    QEMUFile* mFile;
    emulator_compatible::Snapshot* mSnapshotPb = nullptr;
};

}  // namespace qemu
}  // namespace android
