// Copyright 2016 The Android Open Source Project
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

#include "android/emulation/AdbServer.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/async/AsyncSocketServer.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/android_pipe.h"

#include <functional>

namespace android {

namespace {

using namespace android::base;

// Technical note:
//
// The ADB pipe bridge is implemented in the following way:
//
// - A class (AdbHostServer) is used to model the state of the connection with
//   the host ADB server. It is in charge of binding to a specific loopback
//   port (e.g. 5555) and waiting for a connection there.
//
// - The guest adbd daemon will try to open several ADB pipe connections
//   due to its implementation, but only one will be used at a given time
//   to communicate with the host. Each such connection is modeled by an
//   AdbPipe object.
//
//   Only one of the AdbPipe instances can be connected to the host. If
//   the host closes the connection, the AdbPipe instance will be deleted
//   and another one will be used for the next connection. Similarly, if
//   the guest closes the connection.
//
// - Threading considerations: AdbPipe operations are run in a host vCPU
//   thread. All vCPUs are synchronized by the emulation engine, so no locking
//   is necessary when this happens.
//
//   However, the AdbHostServer runs in the context of the program's main
//   loop, and thus must synchronize with the vCPUs using the VmLock::lock()
//   and VmLock::unlock() methods.
//
// - A small handshake is used by guest ADB connections, using the following
//   protocol:
//
//      A) Guest adbd opens the pipe.
//      B) adbd sends an 'accept' message (6 bytes).
//      C) AdbPipe waits for an host connection, then responds with
//         'ok' (2 bytes).
//      D) adbd sends a 'start' message (5 bytes).
//      E) after that, all data is proxyed by the AdbPipe between adbd
//         and the host.
//
//   As such, each AdbPipe can be in one of three states:
//
//      - Unconnected: pipe instance was created, but didn't receive
//                     the 'accept' message yet from the guest.
//
//      - Accept: pipe received 'accept' message and is waiting for a host
//                connection.
//
//      - Connection: pipe received the 'start' message and is proxying
//                    all data between adbd and the host ADB server.
//
//      - Closing: the host has closed the socket connection, the guest
//                 has been notified but hasn't called the
//                 AndroidPipeFuncs::close() callback yet.
//

// AdbPipe states. See technical note above for explanations.
enum class ConnectState {
    Unconnected = 0,
    Accept,
    Connected,
    Closing,
};

// A class used to model the host ADB server connection.
using AdbHostServer = android::base::AsyncSocketServer;

class AdbPipe;

// Base interface for the global state used by this implementation.
// See the comments in AdbPipeList below for documentation.
class AdbPipeListBase {
public:
    AdbPipeListBase() = default;
    virtual ~AdbPipeListBase() = default;
    virtual bool addPipe(AdbPipe* pipe) { return false; }
    virtual bool delPipe(AdbPipe* pipe) { return false; }
    virtual int setPendingPipe(AdbPipe* pipe) { return -1; }
};

// A class used to model a single ADB pipe instance.
class AdbPipe {
public:
    // Default constructor is forbidden.
    AdbPipe() = delete;

    // Create new pipe instance. |hwpipe| is the hardware view of the pipe,
    // and |list| is a pointer to the global list.
    AdbPipe(void* hwPipe, AdbPipeListBase* list) : mHwPipe(hwPipe), mList(list) {}

    ~AdbPipe() = default;

    // Connect a pipe to a host socket. On success, takes ownership of
    // |socket| and return true. On failure, return false.
    bool connectTo(int socket) {
        if (mState != ConnectState::Accept) {
            LOG(ERROR) << "Connecting to ADB pipe that isn't waiting for it";
            return false;
        }
        mHostSocket.reset(ThreadLooper::get()->createFdWatch(socket,
                                                             onHostSocketEvent,
                                                             this));
        sendReply(kOk);
        return true;
    }

private:
    // Called when the guest closes the pipe to destroy it.
    // Since there is no buffering involved, we can destroy the instance
    // directly, which will force-close the host socket.
    void closeFromGuest() {
        if (!mList->delPipe(this)) {
            delete this;
        }
    }

    // Called when the host closes the socket. Tell the guest and change
    // the state to refuse any more data exchange from it.
    void closeFromHost() {
        mState = ConnectState::Closing;
        mGuestCanRead = false;
        mGuestCanWrite = false;
        if (mHostSocket.get()) {
            mHostSocket->dontWantRead();
            mHostSocket->dontWantWrite();
        }
        // Tell guest to close the pipe, and force-close the socket.
        android_pipe_close(mHwPipe);
    }

    void handleCommand() {
        if (mCommand == kAccept) {  // just received an 'accept'
            CHECK(mState == ConnectState::Unconnected);
            mState = ConnectState::Accept;
            // Wait until the pipe is connected to send an 'ok' reply.
            resetCommand();
            int socket = mList->setPendingPipe(this);
            if (socket >= 0) {
                connectTo(socket);
            }
            return;
        }
        if (mCommand == kStart) {  // just received a 'start'
            CHECK(mState == ConnectState::Accept);
            resetCommand();
            mState = ConnectState::Connected;
            // TODO(digit): Anything to do here?
            return;
        }
        CHECK(false) << "Unsupported command [" << mCommand << "]";
    }

    // Called when data arrives from the guest.
    int onGuestSendBuffers(const AndroidPipeBuffer* buffers, int count) {
        if (mState == ConnectState::Closing) {
            // The host has closed the socket, so return an error.
            return PIPE_ERROR_IO;
        }
        if (mState != ConnectState::Connected) {
            // Not connected, means we should be expecting a command from
            // the guest. Simply check that we receive the expected command
            // bytes. There is no framing.
            if (mCommand.empty()) {
                LOG(ERROR) << "Invalid state: receiving command from guest!?";
                return PIPE_ERROR_AGAIN;
            }
            bool match = true;
            int result = 0;
            while (count > 0 && mCommandPos < mCommand.size()) {
                size_t avail = mCommand.size() - mCommandPos;
                int chunk = std::min(buffers[0].size, avail);
                const char* data = reinterpret_cast<const char*>(buffers[0].data);
                for (int n = 0; n < chunk; n++) {
                    if (data[n] != mCommand[mCommandPos + n]) {
                        match = false;
                        break;
                    }
                    result ++;
                }
            }
            if (!match) {
                // The guest data doesn't match the expected command,
                // this is bad. Just return an error. We could send a 'ko'
                // but there is no point in maintaining the connection here.
                LOG(ERROR) << "Unexpected ADB pipe command";
                closeFromHost();
                return PIPE_ERROR_IO;
            }
            if (mCommandPos == mCommand.size()) {
                // A match made in heaven, not change state accordingly.
                mGuestCanWrite = false;
                handleCommand();
            }
            return result;
        } else {
            // Connected state, simply proxy data to the socket.
            int result = 0;
            while (count > 0) {
                size_t size = buffers[0].size;
                const char* data =
                        reinterpret_cast<const char*>(buffers[0].data);

                // NOTE: Sometimes a broken kernel pipe driver might send
                //       a buffer with a size of 0. Ignore them.
                if (size > 0) {
                    ssize_t ret = socketSend(mHostSocket->fd(), data, size);
                    if (ret > 0) {
                        result += ret;
                        data += ret;
                        size -= ret;
                    } else if (ret < 0 && socketLastErrorIsRetry()) {
                        mGuestCanWrite = false;
                        // An error occured. If this is EAGAIN the return
                        // PIPE_ERROR_AGAIN unless some bytes were already
                        // transmitted.
                        if (result > 0) {
                            return result;
                        }
                        mHostSocket->dontWantWrite();
                        return PIPE_ERROR_AGAIN;
                    } else {
                        // I/O error or socket closed from host.
                        closeFromHost();
                        return (result > 0) ? result : PIPE_ERROR_IO;
                    }
                }
                if (!size) {
                    count--;
                    buffers++;
                }
            }
            return result;
        }
    }

    // Called when the guest wants to read data from the pipe.
    int onGuestRecvBuffers(AndroidPipeBuffer* buffers, int count) {
        if (mState == ConnectState::Closing) {
            // The host has closed the socket so return an error.
            return PIPE_ERROR_IO;
        }
        if (mState != ConnectState::Connected) {
            // Not connected, means we should be sending a reply to the
            // guest.
            if (mReply.empty()) {
                LOG(ERROR) << "Invalid state: guest reading empty reply!?";
                return PIPE_ERROR_IO;
            }
            int result = 0;
            while (count > 0 && mReplyPos < mReply.size()) {
                size_t avail = mReply.size() - mReplyPos;
                size_t chunk = std::min(avail, buffers[0].size);
                ::memcpy(buffers[0].data, mReply.data() + mReplyPos, chunk);
                mReplyPos += chunk;
                result += chunk;
                count--;
                buffers++;
            }
            if (mReplyPos == mReply.size()) {
                // The reply was sent, switch to Connected state to start
                // proxying data.
                CHECK(mState == ConnectState::Accept);
                mState = ConnectState::Connected;
                // Technically: we're not sure this is true, but we shouldn't
                // prevent the guest to try to write after polling the pipe.
                mGuestCanWrite = true;
                mGuestCanRead = true;
            }
            return result;
        } else {
            // Connected state, try to read data from the socket and proxy
            // it to the guest.
            int result = 0;
            while (count > 0) {
                char* data = reinterpret_cast<char*>(buffers[0].data);
                size_t size = buffers[0].size;

                if (size > 0) {
                    ssize_t ret = socketRecv(mHostSocket->fd(), data, size);
                    if (ret > 0) {
                        result += ret;
                        data += ret;
                        size -= ret;
                    } else if (ret < 0 && socketLastErrorIsRetry()) {
                        mGuestCanRead = false;
                        if (result > 0) {
                            // Some data was read, return it, the next
                            // call will end up in the PIPE_ERROR_AGAIN
                            // code path below.
                            return result;
                        }
                        mHostSocket->dontWantRead();
                        return PIPE_ERROR_AGAIN;
                    } else {
                        // I/O error or socket closed from host.
                        closeFromHost();
                        return (result > 0) ? result : PIPE_ERROR_IO;
                    }
                }
                if (!size) {
                    count--;
                    buffers++;
                }
            }
            return result;
        }
    }

    // Called when the guest wants to poll possible i/o events.
    unsigned onGuestPoll() {
        unsigned result = 0;
        if (mState != ConnectState::Closing) {
            if (mGuestCanRead) {
                result |= PIPE_POLL_IN;
            }
            if (mGuestCanWrite) {
                result |= PIPE_POLL_OUT;
            }
        }
        return result;
    }

    // Called when the guest wants to be woken up under certain i/o events.
    void onGuestWake(int flags) {
        if (mState != ConnectState::Closing) {
            int callFlags = 0;
            if ((flags & PIPE_WAKE_READ) != 0) {
                // Guest wants to be woken up when there is something to send.
                if (mHostSocket.get()) {
                    mHostSocket->wantRead();
                }
                if (mReplyPos < mReply.size()) {
                    callFlags |= PIPE_WAKE_READ;
                }
            }
            if ((flags & PIPE_WAKE_WRITE) != 0) {
                // Guest wants to be woken up when it can send data.
                if (mHostSocket.get()) {
                    mHostSocket->wantWrite();
                }
                if (mCommandPos < mCommand.size()) {
                    callFlags |= PIPE_WAKE_WRITE;
                }
            }
            signalCanWake(callFlags);
        }
    }

    void expectCommand(StringView command) {
        mCommandPos = 0U;
        mCommand = command;
        mGuestCanRead = false;
        mGuestCanWrite = true;
        signalCanWake(PIPE_WAKE_WRITE);
    }

    void resetCommand() {
        mCommandPos = 0U;
        mCommand.clear();
        mGuestCanWrite = false;
    }

    void sendReply(StringView reply) {
        mReply = reply;
        mReplyPos = 0U;
        mGuestCanRead = true;
        mGuestCanWrite = false;
        signalCanWake(PIPE_WAKE_READ);
    }

    void signalCanWake(int flags) {
        if (flags) {
            android_pipe_wake(mHwPipe, flags);
        }
    }

    // Called when an i/o event happens on the host socket.
    static void onHostSocketEvent(void* opaque, int fd, unsigned events) {
        auto pipe = reinterpret_cast<AdbPipe*>(opaque);
        if (pipe->mState == ConnectState::Closing) {
            return;
        }
        int wakeFlags = 0;
        if (events & Looper::FdWatch::kEventRead) {
            mGuestCanRead = true;
            wakeFlags |= PIPE_WAKE_READ;
        }
        if (events & Looper::FdWatch::kEventWrite) {
            mGuestCanWrite = true;
            wakeFlags |= PIPE_WAKE_WRITE;
        }
        pipe->signalCanWake(wakeFlags);
    }

    // AndroidPipeFuncs::init
    static void* onPipeInit(void* hwPipe, void* opaque, const char* args) {
        auto list = reinterpret_cast<AdbPipeListBase*>(opaque);
        AdbPipe* pipe = new AdbPipe(hwPipe, list);
        if (!list->addPipe(pipe)) {
            LOG(WARNING) << "Could not handle more open ADB pipes\n";
            delete pipe;
            pipe = NULL;
        }
        return pipe;
    }

    // AndroidPipeFuncs::close
    static void onPipeCloseFromGuest(void* opaque) {
        auto pipe = reinterpret_cast<AdbPipe*>(opaque);
        pipe->closeFromGuest();
    }

    // AndroidPipeFuncs::sendBuffers
    static int onPipeSend(void* opaque, const AndroidPipeBuffer* buffers,
                          int count) {
        auto pipe = reinterpret_cast<AdbPipe*>(opaque);
        return pipe->onGuestSendBuffers(buffers, count);
    }

    // AndroidPipeFuncs::recvBuffers
    static int onPipeRecv(void* opaque, AndroidPipeBuffer* buffers,
                          int count) {
        auto pipe = reinterpret_cast<AdbPipe*>(opaque);
        return pipe->onGuestRecvBuffers(buffers, count);
    }

    // AndroidPipeFuncs::poll
    static unsigned onPipePoll(void* opaque) {
        auto pipe = reinterpret_cast<AdbPipe*>(opaque);
        return pipe->onGuestPoll();
    }

    // AndroidPipeFuncs::wakeOn
    static void onPipeWakeOn(void* opaque, int flags) {
        auto pipe = reinterpret_cast<AdbPipe*>(opaque);
        pipe->onGuestWake(flags);
    }

    static const AndroidPipeFuncs sPipeFuncs;

    static constexpr StringView kAccept = "accept";
    static constexpr StringView kStart = "start";
    static constexpr StringView kOk = "ok";
    static constexpr StringView kKo = "ko";

    void* mHwPipe = 0;
    AdbPipeListBase* mList = nullptr;
    ConnectState mState = ConnectState::Unconnected;
    ScopedSocketWatch mHostSocket;
    bool mGuestCanRead = false;
    bool mGuestCanWrite = false;

    StringView mCommand = {};
    size_t mCommandPos = 0;

    StringView mReply = {};
    size_t mReplyPos = 0;
};

// static
const AndroidPipeFuncs AdbPipe::sPipeFuncs = {
    .init = onPipeInit,
    .close = onPipeCloseFromGuest,
    .sendBuffers = onPipeSend,
    .recvBuffers = onPipeRecv,
    .poll = onPipePoll,
    .wakeOn = onPipeWakeOn,
    .load = nullptr,   // Can't load or save server state.
    .save = nullptr,
};


// A convenience class used to model a global list of guest ADB pipes.
// This must be thread-safe. Usage is the following:
//
// - Create an instance, passing a callback that is used to indicate
//   that the last connected pipe is closing. This is used to tell the
//   AdbHostServer instance to start accepting new host connections.
//
// - When a new guest pipe is created, call addPipe() to add it to the list.
//   If a host connection is pending, this will also connect it to it.
//
// - When the guest closes the pipe, call delPipe(). If the pipe is connected
//   this will close the socket and tell the AdbHostServer to start accepting
//   now host connections. If will also delete the pipe instance.
//
// - When a new host connection happens, call connect(
class AdbPipeList : public AdbPipeListBase {
public:
    // Constructor.
    AdbPipeList() = default;

    // Destructor.
    ~AdbPipeList() = default;

    bool setupHostServer(int adbPort) {
        mAdbHostServer.reset(AdbHostServer::createTcpLoopbackServer(
                adbPort,
                std::bind(&AdbPipeList::onHostConnection, this, std::placeholders::_1),
                AdbHostServer::kIPv4AndOptionalIPv6,
                ThreadLooper::get()));
        return mAdbHostServer.get() != nullptr;
    }

    // Add a given AdbPipe instance |pipe| to the list. On success, return
    // true and transfers ownership of the pipe to the list. On failure
    // return false.
    bool addPipe(AdbPipe* pipe) override {
        bool result = false;
        AutoLock lock(mLock);
        for (int n = 0; n < kMaxPipes; ++n) {
            if (!mPipes[n].get()) {
                mPipes[n].reset(pipe);
                mPendingPipes[n] = false;
                result = true;
                break;
            }
        }
        return result;
    }

    // Remove an AdbPipe instance |pipe| from the list. On success, delete
    // the pipe and tell the AdbServer to reopen connections, then return true.
    // On failure, return false (and the caller should delete the pipe or assert).
    bool delPipe(AdbPipe* pipe) override {
        AutoLock lock(mLock);
        if (pipe == mConnectedPipe) {
            mAdbHostServer->startListening();
            mConnectedPipe = nullptr;
        }
        for (int n = 0; n < kMaxPipes; ++n) {
            if (mPipes[n].get() == pipe) {
                mPipes[n].reset();
                mPendingPipes[n] = false;
                return true;
            }
        }
        return false;
    }

    // Call this to indicate that a given pipe is now waiting for host
    // connections. If a pending host connection exits, it will be connected
    // to it automatically and its socket descriptor will be returned.
    // otherwise, the result is -1.
    int setPendingPipe(AdbPipe* pipe) {
        AutoLock lock(mLock);
        bool found = false;
        for (int n = 0; n < kMaxPipes; ++n) {
            if (mPipes[n].get() == pipe) {
                CHECK(!mPendingPipes[n]) << "Pipe is already pending ???";
                mPendingPipes[n] = true;
                found = true;
                break;
            }
        }
        CHECK(found) << "Unknown pending pipe " << pipe << " ???";
        return tryConnectionLocked();
    }

private:
    // This is called when a new host connection happens.
    bool onHostConnection(int socket) {
        AutoLock lock(mLock);
        if (mPendingHostSocket >= 0) {
            // There is already a pending connection, weird!!
            return false;
        }
        mPendingHostSocket = socket;
        tryConnectionLocked();
        // TODO(digit): tell the connected pipe that it is now connected??
        //XXX XXX;
        return true;
    }

    // Try to connect one pending pipe and a host socket. On success,
    // set mConnectedPipe and return the socket file descriptor which becomes
    // owned by the caller. On failure just return -1. Assume the mutex is
    // locked.
    int tryConnectionLocked() {
        if (mPendingHostSocket < 0) {
            // No pending host connection.
            return -1;
        }
        if (!mConnectedPipe) {
            // Find a pending pipe.
            for (int n = 0; n < kMaxPipes; ++n) {
                if (mPendingPipes[n]) {
                    mConnectedPipe = mPipes[n].get();
                    mPendingPipes[n] = false;
                    break;
                }
            }
            if (!mConnectedPipe) {
                // Could not find a pending pipe, will have to retry later.
                return -1;
            }
        }
        int result = mPendingHostSocket;
        mPendingHostSocket = -1;
        return result;
    }

    static constexpr int kMaxPipes = 4;

    Lock mLock;
    int mPendingHostSocket = -1;
    AdbPipe* mConnectedPipe = nullptr;
    std::unique_ptr<AdbPipe> mPipes[kMaxPipes];
    bool mPendingPipes[kMaxPipes] = {};
    std::unique_ptr<AdbHostServer> mAdbHostServer;
};

}  // namespace

// static
bool AdbServer::notifyHostServer(int hostAdbPort, int adbListenPort) {
    int fd = socketTcpLoopbackClient(hostAdbPort);
    if (fd < 0) {
        return false;
    }
    std::string message = StringFormat("host:emulator:%d", adbListenPort);
    std::string handshake = StringFormat("%04x%s", message.size(), message.c_str());
    bool result = socketSendAll(fd, handshake.c_str(), handshake.size());
    socketClose(fd);
    return result;
}

}  // namespace android
