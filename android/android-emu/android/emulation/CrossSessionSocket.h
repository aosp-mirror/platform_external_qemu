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

#pragma once

#include "android/base/StringView.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/sockets/ScopedSocket.h"

#include <vector>

namespace android {
namespace emulation {
class CrossSessionSocket {
public:
    CrossSessionSocket() = default;
    CrossSessionSocket(CrossSessionSocket&& other);
    CrossSessionSocket& operator=(CrossSessionSocket&& other);
    CrossSessionSocket(int fd);
    CrossSessionSocket(android::base::ScopedSocket&& socket);
    void reset();
    bool valid() const;
    // Check if a socket should be recycled for snapshot (if yes recycle it)
    static void recycleSocket(CrossSessionSocket&& socket);
    static void registerForRecycle(int socket);
    static CrossSessionSocket reclaimSocket(int fd);
    // clearRecycleSockets is for testing purpose
    static void clearRecycleSockets();

    enum class DrainBehavior {
        Clear,
        AppendToBuffer
    };
    void drainSocket(DrainBehavior drainBehavior);
    bool hasStaleData() const;
    size_t readStaleData(void* data, size_t dataSize);
    size_t getReadableDataSize() const;
    void onSave(android::base::Stream* stream);
    void onLoad(android::base::Stream* stream);
    int fd() const;
private:
    android::base::ScopedSocket mSocket;
    std::vector<uint8_t> mRecvBuffer;
    size_t mRecvBufferBegin = 0;
    size_t mRecvBufferEnd = 0;
};
} // namespace emulation
} // namespace android
