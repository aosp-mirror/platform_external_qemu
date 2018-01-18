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

#include "android/base/StringView.h"
#include "android/base/system/System.h"

#include "google/protobuf/io/zero_copy_stream_impl.h"

#include <functional>

namespace android {
namespace protobuf {

enum class ProtobufLoadResult {
    Success = 0,
    FileNotFound = 1,
    FileMapFailed = 2,
    ProtobufParseFailed = 3,
};

enum class ProtobufSaveResult {
    Success = 0,
    Failure = 1,
};

// We separate the file-related ops from the protobuf related ones because
// 1. Not going to have separate .cpp implementations for templates
// 2. Messy to expose all the file r/w related headers to users of LoadSave.h

using ProtobufLoadCallback =
    std::function<ProtobufLoadResult(void*, android::base::System::FileSize)>;

using ProtobufSaveCallback =
    std::function<ProtobufSaveResult(google::protobuf::io::FileOutputStream& stream,
                                     android::base::System::FileSize*)>;

ProtobufLoadResult loadProtobufFileImpl(android::base::StringView fileName,
                                        android::base::System::FileSize* bytesUsed,
                                        ProtobufLoadCallback loadCb);
ProtobufSaveResult saveProtobufFileImpl(android::base::StringView fileName,
                                        android::base::System::FileSize* bytesUsed,
                                        ProtobufSaveCallback saveCb);

template <class T>
ProtobufLoadResult loadProtobuf(android::base::StringView fileName,
                                T& toLoad,
                                android::base::System::FileSize*
                                bytesUsed = nullptr) {
    return
        loadProtobufFileImpl(
            fileName,
            bytesUsed,
            [&toLoad](void* ptr, android::base::System::FileSize size) {
                if (!toLoad.ParseFromArray(ptr, size)) {
                    return ProtobufLoadResult::ProtobufParseFailed;
                } else {
                    return ProtobufLoadResult::Success;
                }
            });
}

template <class T>
ProtobufSaveResult saveProtobuf(android::base::StringView fileName,
                                const T& toSave,
                                android::base::System::FileSize*
                                bytesUsed = nullptr) {
    return
        saveProtobufFileImpl(
            fileName, bytesUsed,
            [&toSave](google::protobuf::io::FileOutputStream& stream,
                      android::base::System::FileSize* bytesUsed) {
               if (toSave.SerializeToZeroCopyStream(&stream)) {
                   if (bytesUsed) *bytesUsed = (android::base::System::FileSize)stream.ByteCount();
                   return ProtobufSaveResult::Success;
               } else {
                   return ProtobufSaveResult::Failure;
               }
            });
}

} // namespace android
} // namespace protobuf
