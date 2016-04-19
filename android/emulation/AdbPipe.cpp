// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AdbPipe.h"

#include "android/base/async/AsyncSocketServer.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringView.h"
#include "android/emulation/AdbBridgeDevice.h"

#include <string>

#include <assert.h>

namespace android {
namespace emulation {

using android::base::AsyncSocketServer;
using android::base::Looper;
using android::base::ScopedSocketWatch;
using android::base::StringView;
using std::string;

class AdbPipe : public IAdbPipe {
private:
    // ////////////////////////////////////////////////////////////////////////
    // AdbPipe state machine
    //
    enum class State {
        // The initial unconnected state.
        Unconnected = 0,

        // Connect transtions. Keep latency low.
        GuestAccepted = 1 << 1,
        HostConnected = 1 << 2,

        // Two way communication between guest and bridge established.
        // Keep throughput high in this state.
        Connected = 1 << 5,

        // Disconnect transitions.
        HostDisconnected = 1 << 6,
        GuestDisconnected = 1 << 7,
    };

    static string stateToString(State state) {
        switch (state) {
            case State::Unconnected:
                return "AdbPipe::Unconnected";
            case State::GuestAccepted:
                return "AdbPipe::GuestAccepted";
            case State::HostConnected:
                return "AdbPipe::HostConnected";
            case State::Connected:
                return "AdbPipe::Connected";
            case State::HostDisconnected:
                return "AdbPipe::HostDisconnected";
            case State::GuestDisconnected:
                return "AdbPipe::GuestDisconnected";
            default:
                return "AdbPipe::InvalidState";
        }
    }

    static constexpr StringView kCommandOk = "ok";
    static constexpr StringView kCommandKo = "ko";
    static constexpr StringView kCommandAccept = "accept";
    static constexpr StringView kCommandStart = "start";

    // Shutdown cleanly from any state.
    // Calling thread: Any.
    void teardown() {
        if (mHostSocketWatch) {
            mHostSocketWatch->dontWantRead();
            mHostSocketWatch->dontWantWrite();
            mHostSocketWatch.reset();
            if (mState < State::HostDisconnected) {
                mState = State::HostDisconnected;
            }
        }

        if (mState < State::GuestDisconnected) {
            mGuestCanRead = false;
            mGuestCanWrite = false;
            // This will eventually result in |guestClose| being called, which
            // will finally notify our parent that we're done.
            requestGuestClose();
        }
    }

public:
    AdbPipe(AdbBridgeDevice* parent, Looper* looper, void* hwpipe)
        : IAdbPipe(parent, looper, hwpipe) {}

    // /////////////////////////////////////////////////////////////////////////
    // Implementation of API for AndroidBridgeDevice.

    // Calling thread: vCPU
    bool init() override {
        mGuestCanRead = mGuestCanWrite = true;
        requestGuestWakeOn(mGuestWakeMask);
        return true;
    }

    // Calling thread: Main thread
    bool connectTo(int socket) override {
        if (mState > State::HostConnected) {
            // Too late in our life-cycle.
            return false;
        }

        mHostSocket = socket;
        if (mState == State::GuestAccepted) {
            connectToHostFd();
            mState = State::HostConnected;
            initiateGuestReply(kCommandOk);
        }
        return true;
    }

public:
    // /////////////////////////////////////////////////////////////////////////
    // AndroidPipeFuncs implementation.

    // Calling thread: vCPU
    void guestClose() override {
        mState = State::GuestDisconnected;
        teardown();
        // TODO(pprabhu) Are we OK with this getting called on arbitrary thread?
        mParent->notifyPipeDisconnection(this);
    }

    // Calling thread: vCPU
    int guestSendBuffers(const AndroidPipeBuffer* buffers,
                         int numBuffers) override {
        if (mState == State::Connected) {
            // Fast path.
            return sendBuffersToHost(buffers, numBuffers);
        }

        if (mState > State::Connected) {
            LOG(INFO) << "Guest tried to write in " << stateToString(mState);
            return PIPE_ERROR_IO;
        }

        int ret = PIPE_ERROR_IO;
        switch (mState) {
            case State::Unconnected:
                if (checkCommand(buffers, numBuffers, kCommandAccept, &ret)) {
                    mState = State::GuestAccepted;
                    if (mHostSocket > 0) {
                        connectToHostFd();
                        mState = State::HostConnected;
                        initiateGuestReply(kCommandOk);
                    }
                    return ret;
                }
                break;
            case State::HostConnected:
                if (checkCommand(buffers, numBuffers, kCommandStart, &ret)) {
                    mState = State::Connected;
                    // All subsequent communication will be via the fast path.
                    // We're ready to chat more with the guest.
                    requestGuestWakeOn(mGuestWakeMask);
                    return ret;
                }
                break;
            default:
                break;
        }

        return PIPE_ERROR_IO;
    }

    // Calling thread: vCPU
    int guestRecvBuffers(AndroidPipeBuffer* buffers, int numBuffers) override {
        if (mState == State::Connected) {
            // fast path.
            return recvBuffersFromHost(buffers, numBuffers);
        }

        if (mState < State::Connected) {
            int ret;
            if (!completeGuestReply(buffers, numBuffers, &ret)) {
                LOG(INFO) << "Failed to read guest's reply.";
                ret = PIPE_ERROR_IO;
            }
            requestGuestWakeOn(mGuestWakeMask & PIPE_WAKE_WRITE);
            return ret;
        }

        // fallthrough: Incorrect state.
        LOG(INFO) << "Guest tried to read data in " << stateToString(mState);
        return PIPE_ERROR_IO;
    }

    // Calling thread: vCPU
    unsigned guestPoll() override {
        if (mState > State::Connected) {
            return 0;
        }

        unsigned result = 0;
        result |= (mGuestCanRead) ? PIPE_POLL_IN : 0;
        result |= (mGuestCanWrite) ? PIPE_POLL_OUT : 0;
        return result;
    }

    // Calling thread: vCPU
    void guestWakeOn(int flags) override {
        mGuestWakeMask = flags;
        if (!mHostSocketWatch) {
            return;
        }

        if (mGuestWakeMask & PIPE_WAKE_READ) {
            mHostSocketWatch->wantRead();
        }
        if (mGuestWakeMask & PIPE_WAKE_WRITE) {
            mHostSocketWatch->wantWrite();
        }
    }

private:
    // Calling thread: Main thread.
    void hostSocketEvent(unsigned events) {
        if (mState > State::Connected) {
            return;
        }

        int flags;
        if (events & Looper::FdWatch::kEventRead) {
            mGuestCanRead = true;
            flags |= (mGuestWakeMask & PIPE_WAKE_READ);
        }
        if (events & Looper::FdWatch::kEventWrite) {
            mGuestCanWrite = true;
            flags |= (mGuestWakeMask & PIPE_WAKE_WRITE);
        }
        requestGuestWakeOn(flags);
    }

    // /////////////////////////////////////////////////////////////////////////
    // Data pushing functions -- used to push data between the guest and the ADB
    // server, and also to reply to guest directly from this device.
    // The focus here is on throughput.
    int sendBuffersToHost(const AndroidPipeBuffer* buffers, int numBuffers) {
        int result = 0;
        for (int count = 0; count < numBuffers; ++count) {
            const auto& buffer = buffers[count];
            auto size = buffer.size;
            auto data = reinterpret_cast<const char*>(buffer.data);

            while (size > 0) {
                auto ret = android::base::socketSend(mHostSocket, data, size);
                if (ret > 0) {
                    result += ret;
                    data += ret;
                    size -= ret;
                    continue;
                }

                if (ret < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
                    // We'll unblock guest writes once the host socket tells us
                    // it's ready again.
                    mGuestCanWrite = false;
                    // If we've already written some data, report how much.
                    // Guest should retry for the remaining.
                    if (result > 0) {
                        return result;
                    }
                    return PIPE_ERROR_AGAIN;
                }

                // I/O error or socket closed from host.
                teardown();
                return (result > 0) ? result : PIPE_ERROR_IO;
            }
        }
        return result;
    }

    int recvBuffersFromHost(AndroidPipeBuffer* buffers, int numBuffers) {
        int result = 0;
        for (int count = 0; count < numBuffers; ++count) {
            auto& buffer = buffers[count];
            auto data = reinterpret_cast<char*>(buffer.data);
            size_t size = buffer.size;

            while (size > 0) {
                auto ret = android::base::socketRecv(mHostSocket, data, size);
                if (ret > 0) {
                    result += ret;
                    data += ret;
                    size -= ret;
                    continue;
                }

                if (ret < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
                    // We'll unblock guest reads once the host socket tells us
                    // it's ready again.
                    mGuestCanRead = false;
                    // If we've already read some data, return that.
                    if (result > 0) {
                        return result;
                    }
                    return PIPE_ERROR_AGAIN;
                }

                // I/O error or socket closed from host.
                teardown();
                return (result > 0) ? result : PIPE_ERROR_IO;
            }
        }
        return result;
    }

    // /////////////////////////////////////////////////////////////////////////
    // Command handling functions.
    // Assumption Warning: We assume that the guest can read/write the commands
    // in one call. Since all our command/replys are < 10 bytes, this is a
    // reasonable assumption. If we find the case to be otherwise, we'll
    // complain loudly and bail.

    bool checkCommand(const AndroidPipeBuffer* buffers,
                      int numBuffers,
                      StringView command,
                      int* outNumBytesRead) {
        assert(outNumBytesRead);
        int bufIndex = 0;
        // Compare the whole command + trailing '\0'
        auto len = command.size() + 1;
        auto commandCstr = command.c_str();
        while (len > 0 && bufIndex < numBuffers) {
            const auto& buffer = buffers[bufIndex];
            auto compareLen = std::min(len, buffer.size);
            len -= compareLen;
            if (::memcmp(commandCstr, buffer.data, compareLen)) {
                *outNumBytesRead = command.size() - len;
                return false;
            }
            commandCstr += compareLen;
        }
        *outNumBytesRead = command.size() + 1 - len;
        if (len == 0) {
            // We finished scanning the command and found no mis-match.
            return true;
        } else {
            // We didn't find enough data to compare with the command.
            // This is unexpected.
            LOG(ERROR) << "Guest sent insufficient data to compare against "
                          "command! Increase your guest ADB buffer size to at "
                          "least allow commands.";
            return false;
        }
    }

    void initiateGuestReply(StringView reply) {
        mReply = reply;
        mGuestCanRead = true;
        mGuestCanWrite = false;
        requestGuestWakeOn(mGuestWakeMask & PIPE_WAKE_READ);
    }

    bool completeGuestReply(AndroidPipeBuffer* buffers,
                            int numBuffers,
                            int* outNumBytesWritten) {
        assert(outNumBytesWritten);

        int bufIndex = 0;
        // Write the whole command + trailing '\0'
        auto len = mReply.size() + 1;
        auto replyCStr = mReply.c_str();
        while (len > 0 && bufIndex < numBuffers) {
            auto& buffer = buffers[bufIndex];
            auto copyLen = std::min(len, buffer.size);
            ::memcpy(buffer.data, replyCStr, copyLen);
            len -= copyLen;
            replyCStr += copyLen;
        }
        *outNumBytesWritten = mReply.size() + 1 - len;

        mReply = kCommandKo;
        if (len == 0) {
            // We finished copying the reply.
            return true;
        } else {
            // We didn't find enough space to copy in the reply.
            // This is unexpected.
            LOG(ERROR) << "Guest requested insufficient data to copy in reply! "
                          "Increase your guest ADB buffer size to at least "
                          "allow replies.";
            return false;
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    // Boilerplate + Helper functions.
    void connectToHostFd() {
        mHostSocketWatch.reset(mLooper->createFdWatch(
                mHostSocket, &AdbPipe::onHostSocketEvent, this));
        if (mGuestWakeMask & PIPE_WAKE_READ) {
            mHostSocketWatch->wantRead();
        }
        if (mGuestWakeMask & PIPE_WAKE_WRITE) {
            mHostSocketWatch->wantWrite();
        }
    }

    static void onHostSocketEvent(void* opaque, int fd, unsigned events) {
        auto thisPtr = static_cast<AdbPipe*>(opaque);
        if (thisPtr->mHostSocket != fd) {
            return;
        }
        thisPtr->hostSocketEvent(events);
    }

private:
    State mState = State::Unconnected;
    StringView mReply = kCommandKo;

    bool mGuestCanRead = false;
    bool mGuestCanWrite = false;
    int mGuestWakeMask = PIPE_WAKE_READ | PIPE_WAKE_WRITE;

    int mHostSocket = 0;
    ScopedSocketWatch mHostSocketWatch;
};

IAdbPipe* AdbPipeFactory::create(AdbBridgeDevice* parent,
                                 Looper* looper,
                                 void* hwpipe) {
    return new AdbPipe(parent, looper, hwpipe);
}

}  // namespace emulation
}  // namespace android
