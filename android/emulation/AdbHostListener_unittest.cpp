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

#include "android/emulation/AdbHostListener.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/Thread.h"

#include <gtest/gtest.h>

// Because gtest includes Windows headers that declare the ERROR
// macro that prevents LOG(ERROR) from compiling!!
#ifdef _WIN32
#undef ERROR
#endif

#include <memory>
#include <errno.h>

namespace android {
namespace emulation {

using namespace android::base;

namespace {

// Simple server thread that receives data and stores it in a buffer.
class AdbServerThread : public Thread {
public:
    // Create new thread instance, try to bound to specific TCP |port|,
    // a value of 0 let the system choose a free IPv4 port, which can
    // later be retrieved with port().
    AdbServerThread(int port = 0)
        : Thread(), mSocket(socketTcp4LoopbackServer(port)) {}

    // Returns true if port could be bound.
    bool valid() const { return mSocket.valid(); }

    // Return bound server port.
    int port() const { return socketGetPort(mSocket.get()); }

    // Return buffer content as a StringView.
    StringView view() const { return mString; }

    // Main function simply receives everything and stores it in a string.
    virtual intptr_t main() override {
        // Wait for a single connection.
        int fd = socketAcceptAny(mSocket.get());
        if (fd < 0) {
            LOG(ERROR) << "Could not accept one connection!";
            return -1;
        }

        // Now read data from the connection until it closes.
        size_t size = 0;
        for (;;) {
            size_t capacity = mString.size();
            if (size >= capacity) {
                mString.resize(1024 + capacity * 2);
                capacity = mString.size();
            }
            size_t avail = capacity - size;
            ssize_t len = socketRecv(fd, &mString[size], avail);
            if (len <= 0) {
                break;
            }
            size += len;
        }
        socketClose(fd);

        mString.resize(size);
        return static_cast<intptr_t>(size);
    }

private:
    ScopedSocket mSocket;
    std::string mString;
};

// A thread that will connect to a given TCP4 loopback port, send as much
// data as possible, and report its state through state().
class TestConnectThread : public Thread {
public:
    enum State {
        kInit = 0,
        kConnected,
        kDisconnected,
    };

    explicit TestConnectThread(int port, int id) : mPort(port), mId(id) {}

    State state() const {
        AutoLock lock(mLock);
        return mState;
    }

    void setState(State newState) {
        AutoLock lock(mLock);
        mState = newState;
    }

    virtual intptr_t main() override {
        // Connect
        std::string prefix = StringFormat("[thread %d] ", mId);
        ScopedSocket fd(socketTcp4LoopbackClient(mPort));
        if (!fd.valid()) {
            PLOG(ERROR) << prefix << "Connection failed";
            return -1;
        }
        setState(kConnected);
        uint32_t id = static_cast<uint32_t>(mId);
        ssize_t len = socketSend(fd.get(), &id, sizeof(id));
        if (len == 0) {
            LOG(INFO) << prefix << "End of stream";
        }
        if (len < 0) {
            PLOG(ERROR) << prefix;
        }
        if (len != static_cast<ssize_t>(sizeof(id))) {
            LOG(INFO) << prefix << "Invalid sent byte count=" << len;
        }
        setState(kDisconnected);
        socketShutdownWrites(fd.get());
        socketShutdownReads(fd.get());
        return 0;
    }

private:
    int mPort = -1;
    int mId = -1;
    mutable Lock mLock;
    State mState = kInit;
};

struct TestAdbGuest : public AdbGuestAgent {
    TestAdbGuest(int id) : mId(id), mPrefix(StringFormat("[guest %d] ", id)) {}

    virtual ~TestAdbGuest() { close(); }

    virtual bool onHostConnection(int socket) {
        mSocket = socket;

        uint32_t id = 0;
        ssize_t len = socketRecv(socket, &id, sizeof(id));
        if (len == 0) {
            PLOG(ERROR) << mPrefix << "End of stream";
        } else if (len < 0) {
            PLOG(ERROR) << mPrefix << "I/O Error";
        } else if (len != static_cast<ssize_t>(sizeof(id))) {
            LOG(ERROR) << mPrefix << "Invalid read byte count=" << len;
        }
        return true;
    }

    void close() {
        if (mSocket != -1) {
            socketClose(mSocket);
            mSocket = -1;
        }
    }

    int socket() const { return mSocket; }

private:
    int mId = -1;
    int mSocket = -1;
    std::string mPrefix;
};

}  // namespace

TEST(AdbHostListener, notifyAdbServer) {
    // Bind to random port to listen to
    AdbServerThread serverThread;
    ASSERT_TRUE(serverThread.valid());

    const int emulatorPort = 7648;  // Don't use default (5554) here.
    int clientPort = serverThread.port();
    serverThread.start();

    // Send a message to the server thread.
    EXPECT_TRUE(AdbHostListener::notifyAdbServer(clientPort, emulatorPort));
    intptr_t bufferSize = 0;
    EXPECT_TRUE(serverThread.wait(&bufferSize));

    // Verify message content.
    constexpr StringView kExpected = "0012host:emulator:7648";
    EXPECT_EQ(kExpected.size(), static_cast<size_t>(bufferSize));
    EXPECT_EQ(kExpected.size(), serverThread.view().size());
    EXPECT_STREQ(kExpected.c_str(), serverThread.view().c_str());
}

TEST(AdbHostListener, init) {
    std::unique_ptr<AdbHostListener> testServer(new AdbHostListener());
    EXPECT_EQ(-1, testServer->port());

    const int adbPort = 0;  // Bind to random port.
    EXPECT_TRUE(testServer->init(adbPort));
    EXPECT_NE(-1, testServer->port());

    testServer->deinit();
    EXPECT_EQ(-1, testServer->port());
}

TEST(AdbHostListener, addGuestWithSingleGuest) {
    // Start a mock server that listens for notification messages
    // on a random port.
    AdbServerThread adbServerThread;
    const int adbClientPort = adbServerThread.port();
    adbServerThread.start();

    // Create a AdbHostListener instance that will notify the mock server
    // of new opened guests. This also binds to a random port waiting for
    // connections from an ADB Server.
    std::unique_ptr<AdbHostListener> testServer(
            new AdbHostListener(adbClientPort));

    EXPECT_TRUE(testServer->init(0));  // Bind to random port.
    const int adbPort = testServer->port();
    EXPECT_NE(-1, adbPort);

    // Create a new test guest instance, and add it to the AdbHostListener
    // instance. Since no-one has connected to |adbPort| yet, the guest
    // will not be active yet.
    TestAdbGuest* testGuest = new TestAdbGuest(1);
    testServer->addGuest(testGuest);
    EXPECT_EQ(-1, testGuest->socket());  // No connection yet.

    // Create a thread that connects to |adbPort|, this will later
    // activate the guest, and send a notification to the mock server
    // as well.
    TestConnectThread testThread(adbPort, 1);
    EXPECT_EQ(TestConnectThread::kInit, testThread.state());
    testThread.start();

    // Run the looper for 10 ms.
    EXPECT_EQ(ETIMEDOUT, ThreadLooper::get()->runWithTimeoutMs(10));

    // Check that the thread has ended, and that the guest was activated.
    EXPECT_EQ(TestConnectThread::kDisconnected, testThread.state());
    EXPECT_NE(-1, testGuest->socket());  // Must be connected.

    // Check that the mock server received the right notification.
    EXPECT_TRUE(adbServerThread.wait(nullptr));
    std::string message = StringFormat("host:emulator:%d", adbPort);
    std::string expected =
            StringFormat("%04x%s", message.size(), message.c_str());
    EXPECT_EQ(expected, adbServerThread.view());

    // Delete the guest, and disconnect the connection.
    testServer->deleteGuest(testGuest);

    // Wait for the connector thread and check its state.
    intptr_t status = 0;
    EXPECT_TRUE(testThread.wait(&status));
    EXPECT_EQ(0, status);
    EXPECT_EQ(TestConnectThread::kDisconnected, testThread.state());

    testServer->deinit();
    EXPECT_EQ(-1, testServer->port());
}

TEST(AdbHostListener, addGuestWithMultipleGuests) {
    // Create new AdbHostListener that binds on a random port
    std::unique_ptr<AdbHostListener> testServer(new AdbHostListener());
    EXPECT_TRUE(testServer->init(0));
    const int adbPort = testServer->port();
    EXPECT_NE(-1, adbPort);

    constexpr int kMaxGuests = 8;

    // Create kMaxGuests and add them to the server immediately.
    TestAdbGuest* guests[kMaxGuests] = {};
    for (int n = 0; n < kMaxGuests; ++n) {
        guests[n] = new TestAdbGuest(n + 1);
        testServer->addGuest(guests[n]);
    }

    // Check that none of them is activated after this.
    for (int n = 0; n < kMaxGuests; ++n) {
        EXPECT_EQ(-1, guests[n]->socket());
    }

    // Create kMaxGuests connector threads, each one will connect and
    // consume one of the guests, in order of addition in the previous
    // loop.
    for (int n = 0; n < kMaxGuests; ++n) {
        TestConnectThread thread(adbPort, n + 1);
        EXPECT_EQ(TestConnectThread::kInit, thread.state());
        thread.start();

        EXPECT_EQ(ETIMEDOUT, ThreadLooper::get()->runWithTimeoutMs(10));

        EXPECT_EQ(TestConnectThread::kDisconnected, thread.state());
        EXPECT_NE(-1, guests[n]->socket());

        testServer->deleteGuest(guests[n]);

        intptr_t status = 0;
        EXPECT_TRUE(thread.wait(&status));
        EXPECT_EQ(0, status);
        EXPECT_EQ(TestConnectThread::kDisconnected, thread.state());
    }
}

}  // namespace emulation
}  // namespace base
