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

#pragma once

#include "android/base/FunctionView.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"

namespace google {
namespace protobuf {
namespace io {

class ZeroCopyOutputStream;

}  // namespace io
}  // namespace protobuf
}  // namespace google

namespace android {
namespace protobuf {

enum class LoadResult {
    Success,
    FileNotFound,
    FileMapFailed,
    ParsingFailed,
};

enum class SaveResult {
    Success,
    Failure,
};

namespace internal {

// We separate the file-related ops from the protobuf related ones because
// 1. Not going to have separate .cpp implementations for templates
// 2. Messy to expose all the file r/w related headers to users of LoadSave.h

using LoadCallback =
        base::FunctionView<bool(void* data, base::System::FileSize size)>;

using SaveCallback =
        base::FunctionView<bool(google::protobuf::io::ZeroCopyOutputStream&)>;

LoadResult loadProtobufFileImpl(base::StringView fileName,
                                base::System::FileSize* bytesUsed,
                                LoadCallback loadCb);
SaveResult saveProtobufFileImpl(base::StringView fileName,
                                base::System::FileSize* bytesUsed,
                                SaveCallback saveCb);

}  // namespace internal

template <class T>
LoadResult loadProtobuf(base::StringView fileName,
                        T& toLoad,
                        base::System::FileSize* bytesUsed = nullptr) {
    return internal::loadProtobufFileImpl(
            fileName, bytesUsed,
            [&toLoad](void* ptr, base::System::FileSize size) {
                return toLoad.ParseFromArray(ptr, size);
            });
}

template <class T>
SaveResult saveProtobuf(base::StringView fileName,
                        const T& toSave,
                        base::System::FileSize* bytesUsed = nullptr) {
    return internal::saveProtobufFileImpl(
            fileName, bytesUsed,
            [&toSave](google::protobuf::io::ZeroCopyOutputStream& stream) {
                return toSave.SerializeToZeroCopyStream(&stream);
            });
}

}  // namespace protobuf
}  // namespace android
