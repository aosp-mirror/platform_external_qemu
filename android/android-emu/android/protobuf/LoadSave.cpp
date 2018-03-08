// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/protobuf/LoadSave.h"

#include "android/base/files/ScopedFd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/utils/fd.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"

#include "google/protobuf/io/zero_copy_stream_impl.h"

#include <fcntl.h>
#include <sys/mman.h>

using android::base::makeCustomScopedPtr;
using android::base::ScopedCPtr;
using android::base::ScopedFd;
using android::base::StringView;
using android::base::System;

namespace android {
namespace protobuf {
namespace internal {

LoadResult loadProtobufFileImpl(StringView fileName,
                                System::FileSize* bytesUsed,
                                LoadCallback loadCb) {
    const auto file = ScopedFd(
            ::open(fileName.c_str(), O_RDONLY | O_BINARY | O_CLOEXEC, 0644));

    System::FileSize size;
    if (!System::get()->fileSize(file.get(), &size)) {
        return LoadResult::FileNotFound;
    }

    if (bytesUsed)
        *bytesUsed = size;

    const auto fileMap = makeCustomScopedPtr(
            mmap(nullptr, size, PROT_READ, MAP_PRIVATE, file.get(), 0),
            [size](void* ptr) {
                if (ptr != MAP_FAILED)
                    munmap(ptr, size);
            });

    if (!fileMap || fileMap.get() == MAP_FAILED) {
        return LoadResult::FileMapFailed;
    }

    return loadCb(fileMap.get(), size) ? LoadResult::Success
                                       : LoadResult::ParsingFailed;
}

SaveResult saveProtobufFileImpl(StringView fileName,
                                System::FileSize* bytesUsed,
                                SaveCallback saveCb) {
    if (bytesUsed)
        *bytesUsed = 0;

    google::protobuf::io::FileOutputStream stream(
            ::open(fileName.c_str(),
                   O_WRONLY | O_BINARY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644));
    stream.SetCloseOnDelete(true);

    if (!saveCb(stream)) {
        return SaveResult::Failure;
    }

    if (bytesUsed) {
        *bytesUsed = System::FileSize(stream.ByteCount());
    }
    return SaveResult::Success;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace android
