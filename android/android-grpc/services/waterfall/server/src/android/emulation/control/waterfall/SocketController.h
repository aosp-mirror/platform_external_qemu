// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <grpcpp/grpcpp.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "aemu/base/async/Looper.h"
#include "aemu/base/synchronization/ConditionVariable.h"
#include "aemu/base/synchronization/Lock.h"
#include "android/emulation/control/waterfall/WaterfallServiceLibrary.h"
#include "control_socket.pb.h"
#include "waterfall.grpc.pb.h"

namespace android {
namespace emulation {
namespace control {

using Stub = waterfall::Waterfall::Stub;

/**
 * A ControlSocket is a WaterfallGuest <-> Host connection that implements the
 * socket_control protocol desribed in the protobuf socket_control.proto file.
 *
 * The socket will listen on the unix socket: "sockets/h2o".
 */
class ControlSocket {
public:
    // A socket with a ::waterfall stub that will properly clean itself
    // when deleted.
    class WaterfallSocket {
        friend ControlSocket;

    public:
        int getFd() { return mFd; }
        Stub* getStub() { return mStub.get(); }
        ~WaterfallSocket();
        bool isOpen();

    private:
        WaterfallSocket(int fd, int id, ControlSocket* controller);
        int mFd;
        int mId;
        ControlSocket* mController;
        std::unique_ptr<Stub> mStub;
        std::shared_ptr<grpc::ChannelInterface> mChannel;
    };

    ControlSocket();

    // Opens up a new waterfall socket, or nullptr if no connection
    // can be established before the timeout.
    ControlSocket::WaterfallSocket* open(int timeoutInMs = 500);

    // Poll callback.
    void onRead(int fd);

private:
    void onAccept();
    void clearConnections();
    void readMessageFromControlSocket(int fd);
    void readFirstMessage(int fd);
    void registerControlSocket(int fd);
    void registerSocket(int fd, int id);
    void closeByFd(int fd);
    void closeById(int fd);
    bool isIdOpen(int id);
    bool initialize();
    bool readMessage(int fd, SocketControl* msg);
    bool sendMessage(SocketControl_Sort sort, int id);

    int mDomainSocket;
    int mControlSocket;

    std::unordered_map<int, int> mIdToFd;
    std::unordered_map<int, int> mFdToId;
    std::unordered_map<int, std::unique_ptr<base::Looper::FdWatch>> mFdToWatch;
    std::unordered_set<int> mAvailableIds;

    base::Looper* mLooper;
    base::Lock mMapLock;
    base::ConditionVariable mCv;
    std::unique_ptr<base::Looper::FdWatch> mFdWatch;
    std::atomic<uint32_t> mFdCounter{1};

    const int kControlSocketId = 0;
};

// A ControlSocketLibrary can be used to borrow stubs that are using the
// ControlSocket protocol to establish a gRPC channel between the
// waterfall service inside the guest and the emulator host.
class ControlSocketLibrary : public WaterfallServiceLibrary {
public:
    ControlSocketLibrary() {}

    virtual Stub* borrow() override;
    virtual void restore(Stub*) override;
    void erase(Stub*);

private:
    std::unordered_map<Stub*, std::shared_ptr<ControlSocket::WaterfallSocket>>
            mStubToSock;
    std::unordered_set<Stub*> mAvailableStubs;
    ControlSocket mControlSocket;
    base::Lock mMapLock;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
