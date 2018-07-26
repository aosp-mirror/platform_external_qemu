// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/location/Route.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/protobuf/LoadSave.h"
#include "android/utils/fd.h"
#include "android/utils/path.h"

#include <fcntl.h>
#include <sys/mman.h>

using android::base::PathUtils;
using android::base::ScopedFd;
using android::base::System;
using android::base::makeCustomScopedPtr;

using android::protobuf::saveProtobuf;
using android::protobuf::ProtobufSaveResult;

namespace android {
namespace location {

Route::Route(const char* name) : mName(name) {

    const auto file =
            ScopedFd(::open(name, O_RDONLY | O_BINARY | O_CLOEXEC, 0755));
    System::FileSize size;
    if (!System::get()->fileSize(file.get(), &size)) {
        return;
    }
    mSize = size;

    const auto fileMap = makeCustomScopedPtr(
            mmap(nullptr, size, PROT_READ, MAP_PRIVATE, file.get(), 0),
            [size](void* ptr) {
                if (ptr != MAP_FAILED) {
                    munmap(ptr, size);
                }
            });
    if (!fileMap || fileMap.get() == MAP_FAILED) {
        return;
    }
    if (!mRoutePb.ParseFromArray(fileMap.get(), size)) {
        return;
    }
    isValid = true;
}

const emulator_location::RouteMetadata* Route::getProtoInfo() {
    return isValid ? &mRoutePb : nullptr;
}


}  // namespace location
}  // namespace android
