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

#include "android/base/async/Looper.h"
#include "android/base/files/ScopedFd.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace android {
namespace network {

// This is an abstract class for client and server for Wifi forwarding.
class WifiForwardPeer {
public:
    struct Data {
        uint8_t* data;
        size_t size;
        size_t offset;
    };

    using OnDataAvailableCallback = std::function<void (const uint8_t* data,
                                                        size_t size)>;

    explicit WifiForwardPeer(OnDataAvailableCallback onDataAvailable);
    virtual ~WifiForwardPeer() = default;

    bool init();

    void run();
    void stop();

    // Return a pointer to the currently available data that this peer has
    // read from other peers. Returns nullptr if no data available. After
    // receiving and processing the data the caller should call clear() to
    // indicate that the data is no longer needed. Data will keep piling up
    // until it reaches a maximum threshold, after that all incoming data will
    // be dropped. Therefore it's important to regularly read and clear this
    // data.
    const void* data() const;
    // The size of the available data from peers.
    size_t size() const;
    // Clear the receive buffer of all returne data, indicating that it is
    // no longer of use.
    void clear();

    size_t queue(const uint8_t* data, size_t size);
protected:
    size_t send(const void* data, size_t size);
    size_t receive(void* data, size_t size);
    virtual void start() = 0;
    void threadLoop();
    bool onConnect(int fd);
    void onDisconnect();
    void onFdEvent(int fd, unsigned events);
    static void staticOnFdEvent(void *opaque, int fd, unsigned events);

    android::base::Looper* mLooper;
private:
    enum class WakePipeCommand : uint8_t {
        Quit,
        Send
    };

    void sendQueue();
    static void onWakePipe(void* opaque, int fd, unsigned events);
    void sendWakeCommand(WakePipeCommand command);

    android::base::Looper::FdWatch* mFdWatch = nullptr;
    OnDataAvailableCallback mOnDataAvailable;

    std::vector<uint8_t> mReceiveBuffer;
    std::vector<uint8_t> mTransmitBuffer;
    size_t mTransmitQueuePos = 0;
    size_t mTransmitSendPos = 0;
    std::mutex mTransmitBufferMutex;

    android::base::ScopedFd mWakePipeRead;
    android::base::ScopedFd mWakePipeWrite;
    android::base::Looper::FdWatch* mWakeFdWatch = nullptr;

    std::unique_ptr<std::thread> mThread;
};

}  // namespace network
}  // namespace android

