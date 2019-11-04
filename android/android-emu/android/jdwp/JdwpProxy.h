// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/Stream.h"
#include "android/emulation/android_pipe_common.h"

namespace android {
namespace jdwp {
struct JdwpProxy {
    enum Status {
        Uninitialized,
        Connect,
        ConnectOk,
        HandshakeSent,
        HandshakeSentOk,
        HandshakeReplied,
        HandshakeRepliedOk,
        Proxying = HandshakeRepliedOk,
    };
    enum IsJdwp {
        Unknown,
        Yes,
        No,
    };
    struct Buffer {
        static const size_t kBufferSize = 50;
        char mBuffer[kBufferSize];
        size_t mBufferTail = 0;
        size_t mBytesToSkip = 0;
        void readBytes(const AndroidPipeBuffer* buffers,
                       int numBuffers,
                       int* bufferIdx,       // input / output
                       size_t* bufferPst,    // input / output
                       size_t* remainBytes,  // input / output
                       size_t bytesToRead,
                       bool skipData);
    };
    static Status nextStatus(Status status);
    Status mProxyStatus = Status::Uninitialized;
    Status mClientStatus = Status::Uninitialized;
    int mHostId = 0;
    int mGuestId = 0;
    int mGuestPid = 0;
    Buffer mGuestSendBuffer;
    Buffer mGuestRecvBuffer;
    IsJdwp mIsJdwp = IsJdwp::Unknown;
    void onSave(android::base::Stream* stream);
    void onLoad(android::base::Stream* stream);
    void onGuestSendData(const AndroidPipeBuffer* buffers,
                         int numBuffers,
                         size_t actualSentBytes);
    void onGuestRecvData(const AndroidPipeBuffer* buffers,
                         int numBuffers,
                         size_t actualRecvBytes);
};
}  // namespace jdwp
}  // namespace android