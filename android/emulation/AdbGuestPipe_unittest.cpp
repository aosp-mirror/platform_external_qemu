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

#include "android/emulation/AdbGuestPipe.h"

#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/testing/TestAndroidPipeDevice.h"

#include <gtest/gtest.h>

// Because gtest includes Windows headers that declare the ERROR
// macro that prevents LOG(ERROR) from compiling!!
#ifdef _WIN32
#undef ERROR
#endif

#include <memory>
#include <vector>

namespace android {
namespace emulation {

using namespace android::base;

namespace {

// A Mock AdbHostAgent that will be used during testing. It doesn't depend
// on any TCP ports.
class MockAdbHostAgent : public AdbHostAgent {
public:
    enum ConnectionStatus {
        kOk = 0,
        kCannotSendData = -1,
        kCannotReadReply = -2,
    };

    ~MockAdbHostAgent() {
        if (mThread.get()) {
            mThread->wait(nullptr);
        }
    }

    // AdbHostAgent overrides.
    virtual bool init(int adbPort) override {
        mAdbPort = adbPort;
        return true;
    }

    virtual void deinit() override { mAdbPort = -1; }

    virtual void addGuest(AdbGuestAgent* pipe) override {
        mGuests.emplace_back(pipe);
    }

    virtual void deleteGuest(AdbGuestAgent* pipe) override {
        if (mActiveGuest == pipe) {
            mActiveGuest = nullptr;
            mThread->wait(nullptr);
            mThread.reset();
        }
        for (auto it = mGuests.begin(); it != mGuests.end(); ++it) {
            if (it->get() == pipe) {
                mGuests.erase(it);
                return;
            }
        }
        CHECK(false) << "Unknown guest " << pipe;
    }

    // Create a socket pair and a thread that will push |data| into it
    // before trying to read a single byte from one end of the pair.
    // The other end is passed to a new active guest.
    void createFakeConnection(StringView data) {
        CHECK(mActiveGuest == nullptr);
        CHECK(mGuests.size() > 0);
        mActiveGuest = mGuests[0].get();

        mThread.reset(new ConnectorThread(data));
        // Create new self-deleting thread
        // TODO(digit): Create a socket that can be used here.
        if (mActiveGuest->onHostConnection(mThread->releaseOutSocket())) {
            mThread->start();
        }
    }

    // Return number of registered guests.
    size_t numGuests() const { return mGuests.size(); }

private:
    // A small thread that will connect to a given port and send some
    // data through the socket. It will then try to read a single byte
    // before exiting.
    class ConnectorThread : public Thread {
    public:
        ConnectorThread(StringView data) : Thread(), mData(data) {
            int inSocket, outSocket;
            if (socketCreatePair(&inSocket, &outSocket) < 0) {
                PLOG(ERROR) << "Could not create socket pair";
                return;
            }
            // Make the sockets blocking for this test to work.
            socketSetBlocking(inSocket);
            socketSetBlocking(outSocket);

            mInSocket.reset(inSocket);
            mOutSocket.reset(outSocket);
        }

        bool valid() const { return mInSocket.valid() && mOutSocket.valid(); }

        int releaseOutSocket() { return mOutSocket.release(); }

        virtual intptr_t main() override {
            if (mData.size() > 0) {
                if (!socketSendAll(mInSocket.get(), mData.c_str(),
                                   mData.size())) {
                    DPLOG(ERROR) << "I/O error when sending data";
                    return kCannotSendData;
                }
            }
            char buf[1] = {};
            ssize_t len = socketRecv(mInSocket.get(), buf, sizeof(buf));
            if (len < 0) {
                DPLOG(ERROR) << "I/O error when receiving data";
            } else if (len == 0) {
                DLOG(ERROR) << "Disconnected";
            }
            mInSocket.close();
            return 0;
        }

    private:
        ScopedSocket mInSocket;
        ScopedSocket mOutSocket;
        std::string mData;
    };

    int mAdbPort = -1;
    AdbGuestAgent* mActiveGuest = nullptr;
    std::vector<std::unique_ptr<AdbGuestAgent>> mGuests;
    std::unique_ptr<ConnectorThread> mThread;
};

}  // namespace

TEST(AdbGuestPipe, createService) {
    AndroidPipe::Service::resetAll();

    // Create a new service and register it.
    MockAdbHostAgent adbHost;
    auto service = new AdbGuestPipe::Service(&adbHost);
    AndroidPipe::Service::add(service);

    // There shouldn't be any registered guests at that point.
    EXPECT_EQ(0, adbHost.numGuests());

    // Create a new guest
    AndroidPipe::Service::resetAll();
}

TEST(AdbGuestPipe, createOneGuest) {
    // NOTE: This does Service::resetAll() on creation and destruction for us.
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;
    AndroidPipe::Service::add(new AdbGuestPipe::Service(&adbHost));

    // Create a new guest connection from the test device.
    auto guest = TestAndroidPipeDevice::Guest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));
    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);
    EXPECT_EQ(1, adbHost.numGuests());

    char reply[3] = {};
    EXPECT_EQ(2, guest->read(reply, 2));
    EXPECT_STREQ("ok", reply);
    EXPECT_EQ(5, guest->write("start", 5));

    char buffer[kMessage.size() + 1] = {};
    const ssize_t expectedSize = static_cast<ssize_t>(kMessage.size());
    EXPECT_EQ(expectedSize, guest->read(buffer, kMessage.size()));
    EXPECT_STREQ(kMessage.c_str(), buffer);

    EXPECT_EQ(1, guest->write("x", 1));
}

TEST(AdbGuestPipe, createGuestWithBadAcceptCommand) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;
    AndroidPipe::Service::add(new AdbGuestPipe::Service(&adbHost));

    // Create a new guest connection from the test device.
    auto guest = TestAndroidPipeDevice::Guest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    // This write should fail with PIPE_ERROR_IO because the other end
    // is waiting for an 'accept'.
    EXPECT_EQ(PIPE_ERROR_IO, guest->write("ACCEPT", 6));
}

TEST(AdbGuestPipe, createGuestWithCloseOnAccept) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;
    AndroidPipe::Service::add(new AdbGuestPipe::Service(&adbHost));

    // Create a new guest connection from the test device.
    auto guest = TestAndroidPipeDevice::Guest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    // Force-close the connection.
    guest->close();
}

TEST(AdbGuestPipe, createGuestWithCloseOnReply) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;
    AndroidPipe::Service::add(new AdbGuestPipe::Service(&adbHost));

    // Create a new guest connection from the test device.
    auto guest = TestAndroidPipeDevice::Guest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);
    EXPECT_EQ(1, adbHost.numGuests());

    // Force-close the connection.
    guest->close();
}

TEST(AdbGuestPipe, createGuestWithBadStartCommand) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;
    AndroidPipe::Service::add(new AdbGuestPipe::Service(&adbHost));

    // Create a new guest connection from the test device.
    auto guest = TestAndroidPipeDevice::Guest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);
    EXPECT_EQ(1, adbHost.numGuests());

    char reply[3] = {};
    EXPECT_EQ(2, guest->read(reply, 2));
    EXPECT_STREQ("ok", reply);

    // This write should fail with PIPE_ERROR_IO because the other end
    // is waiting for a 'start'. NOTE: This will result in a broken pipe
    // error for the connector thread waiting for a reply.
    EXPECT_EQ(PIPE_ERROR_IO, guest->write("START", 5));
}

TEST(AdbGuestPipe, createGuestWithCloseOnStart) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;
    AndroidPipe::Service::add(new AdbGuestPipe::Service(&adbHost));

    // Create a new guest connection from the test device.
    auto guest = TestAndroidPipeDevice::Guest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));

    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);
    EXPECT_EQ(1, adbHost.numGuests());

    char reply[3] = {};
    EXPECT_EQ(2, guest->read(reply, 2));
    EXPECT_STREQ("ok", reply);

    // Force-close the connection.
    guest->close();
}

TEST(AdbGuestPipe, createGuestWhichClosesTheConnection) {
    // NOTE: This does Service::resetAll() on creation and destruction for us.
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;
    AndroidPipe::Service::add(new AdbGuestPipe::Service(&adbHost));

    // Create a new guest connection from the test device.
    auto guest = TestAndroidPipeDevice::Guest::create();
    EXPECT_TRUE(guest);
    EXPECT_EQ(0, guest->connect("qemud:adb"));
    EXPECT_EQ(6, guest->write("accept", 6));

    static constexpr StringView kMessage = "Hello World!";
    adbHost.createFakeConnection(kMessage);
    EXPECT_EQ(1, adbHost.numGuests());

    char reply[3] = {};
    EXPECT_EQ(2, guest->read(reply, 2));
    EXPECT_STREQ("ok", reply);
    EXPECT_EQ(5, guest->write("start", 5));

    // Read only partial bytes from the connection.
    char buffer[kMessage.size() + 1] = {};
    const ssize_t expectedSize = static_cast<ssize_t>(kMessage.size()) / 2;
    EXPECT_EQ(expectedSize, guest->read(buffer, expectedSize));

    // Force-close the connection now.
    guest->close();
}

TEST(AdbGuestPipe, createMultipleGuestConnections) {
    TestAndroidPipeDevice testDevice;

    MockAdbHostAgent adbHost;
    AndroidPipe::Service::add(new AdbGuestPipe::Service(&adbHost));

    const int kCount = 8;

    // Create kCount guests that all connect at the same time.
    std::unique_ptr<TestAndroidPipeDevice::Guest> guests[kCount];
    for (int n = 0; n < kCount; ++n) {
        guests[n].reset(TestAndroidPipeDevice::Guest::create());
        EXPECT_TRUE(guests[n].get());
        EXPECT_EQ(0, guests[n]->connect("qemud:adb"));
        EXPECT_EQ(n + 1, adbHost.numGuests());
    }

    // Create kCount threads that will send data to each guest in succession.
    for (int n = 0; n < kCount; ++n) {
        auto guest = guests[n].get();
        EXPECT_EQ(6, guest->write("accept", 6));

        std::string message = StringFormat("Hello %d", n + 1);
        adbHost.createFakeConnection(message);

        char reply[3] = {};
        EXPECT_EQ(2, guest->read(reply, 2));
        EXPECT_STREQ("ok", reply);
        EXPECT_EQ(5, guest->write("start", 5));

        std::string buffer;
        buffer.resize(message.size());
        EXPECT_EQ(static_cast<ssize_t>(message.size()),
                  guest->read(&buffer[0], message.size()));

        EXPECT_EQ(message, buffer);
        EXPECT_EQ(1, guest->write("x", 1));
        guest->close();
    }
}

}  // namespace emulation
}  // namespace android
