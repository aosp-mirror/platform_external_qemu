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

#include "android/emulation/control/WebRtcBridge.h"

#include <fcntl.h>        // for open, O_CREAT, O_WRONLY
#include <gtest/gtest.h>  // for Test, AssertionResult
#include <string.h>       // for memcpy

#ifndef _WIN32
#include <unistd.h>       // for close
#endif

#include <algorithm>   // for min
#include <functional>  // for __base
#include <new>         // for operator new
#include <string>      // for string, to_string
#include <vector>      // for vector

#include "android/base/Log.h"                    // for setMinLogLevel, LOG_...
#include "android/base/Optional.h"               // for Optional
#include "android/base/StringView.h"             // for StringView
#include "android/base/files/PathUtils.h"        // for PathUtils
#include "android/base/testing/TestSystem.h"     // for TestSystem
#include "android/base/testing/TestTempDir.h"    // for TestTempDir
#include "android/base/threads/FunctorThread.h"  // for FunctorThread
#include "emulator/net/AsyncSocketAdapter.h"     // for AsyncSocketEventList...
#include "nlohmann/json.hpp"                     // for json_ref, basic_json...

namespace android {
namespace emulation {
namespace control {

using base::Optional;

class TestAsyncSocketAdapter : public AsyncSocketAdapter {
public:
    TestAsyncSocketAdapter(std::vector<std::string> recv) : mRecv(recv) {
        // Let's not get chatty during unit tests.
        base::setMinLogLevel(base::LOG_ERROR);
    }
    ~TestAsyncSocketAdapter() = default;
    void close() override {
        if (mListener)
            mListener->onClose(this, 123);
        mConnected = false;
    }
    void dispose() override{};

    uint64_t recv(char* buffer, uint64_t bufferSize) override {
        base::AutoLock lock(mReadLock);
        if (mRecv.empty()) {
            return 0;
        }
        std::string msg = mRecv.front();
        // Note that c_str() is guaranteed to be 0 terminated, so we always
        // have size() + 1 of in our string.
        uint64_t buflen = std::min<uint64_t>(bufferSize, msg.size() + 1);
        memcpy(buffer, msg.c_str(), buflen);
        mRecv.erase(mRecv.begin(), mRecv.begin() + 1);
        return buflen;
    }
    uint64_t send(const char* buffer, uint64_t bufferSize) override {
        base::AutoLock sendLock(mSendLock);
        std::string msg(buffer, bufferSize);
        mSend.push_back(msg);
        return bufferSize;
    }

    bool connect() override {
        mConnected = true;
        if (mListener)
            mListener->onConnected(this);
        return true;
    }

    bool connected() override { return mConnected; }

    // This will push out all the messages.
    void signalRecv() { mListener->onRead(this); }

    bool connectSync(uint64_t timeoutms) override { return false; };

    // Signal the arrival of a new message..
    void signalRecv(std::string msg) {
        {
            base::AutoLock lock(mReadLock);
            mRecv.push_back(msg);
        }
        signalRecv();
    }

    std::vector<std::string> mSend;
    std::vector<std::string> mRecv;
    bool mConnected = true;
    Lock mReadLock;
    Lock mSendLock;
};

const char* gModule = nullptr;
int gFps = 0;
bool gRtcRunning = false;

extern "C" {
const char* startSharedMemoryModule(int fps) {
    gModule = "shared_region";
    gFps = fps;
    gRtcRunning = true;
    return gModule;
}

bool stopSharedMemoryModule() {
    gRtcRunning = false;
    return true;
}
}

static const QAndroidRecordScreenAgent sQAndroidRecordScreenAgent = {
        .startSharedMemoryModule = startSharedMemoryModule,
        .stopSharedMemoryModule = stopSharedMemoryModule};

const QAndroidRecordScreenAgent* const gQAndroidRecordScreenAgent =
        &sQAndroidRecordScreenAgent;

static const AndroidConsoleAgents fakeAgents = {
        .record = gQAndroidRecordScreenAgent,
};

json msg1 = {{"topic", "moi"}, {"msg", "hello"}};
json msg2 = {{"topic", "moi"}, {"msg", "world"}};
json msgBye = {{"topic", "moi"}, {"bye", "I'm leaving you.."}};

static void make_subfile(const std::string& dir, const char* file) {
    std::string path = dir;
    path.append("/");
    path.append(file);
    int fd = ::open(path.c_str(), O_WRONLY | O_CREAT, 0755);
    EXPECT_GE(fd, 0) << "Path: " << path.c_str();
    ::close(fd);
}

// A test system with the actual system clock...
class RealTimeTestSystem : public base::TestSystem {
public:
    RealTimeTestSystem() : TestSystem("/foo", System::kProgramBitness) {}
    virtual Duration getUnixTimeUs() const override {
        return hostSystem()->getUnixTimeUs();
    }

    // Pretend an executable got launched.
    virtual Optional<std::string> runCommandWithResult(
            const std::vector<std::string>& commandLine,
            System::Duration timeoutMs = System::kInfinite,
            System::ProcessExitCode* outExitCode = nullptr) override {
        *outExitCode = 0;
        return std::string("987659876");
    }
};

// A TestBridge is a WebRtcBridge driven by a fake socket,
// and a fake TestSystem, so you can actually start this thing.
// and pretend to send/receive messages.
class TestBridge : public WebRtcBridge {
public:
    TestBridge(TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({}))
        : WebRtcBridge(socket, &fakeAgents, 24, 1234) {
        // Let's not get weird behavior due to our locks immediately expiring.
        testSys.setLiveUnixTime(true);
        // Make sure we can find goldfish-videobridge
        base::TestTempDir* testDir = testSys.getTempRoot();
        testDir->makeSubDir("foo");

        std::vector<std::string> pathList;
        pathList.push_back(std::string("foo"));
        pathList.push_back(std::string(System::kBinSubDir));
        testDir->makeSubDir(base::PathUtils::recompose(pathList).c_str());

        pathList.push_back(std::string(WebRtcBridge::kVideoBridgeExe));
        std::string programPath = base::PathUtils::recompose(pathList);
        make_subfile(testDir->path(), programPath.c_str());
    }

private:
    RealTimeTestSystem testSys;
};

TEST(WebRtcBridge, connectSaysHi) {
    TestAsyncSocketAdapter* socket =
            new TestAsyncSocketAdapter({"hello", "world"});
    TestBridge bridge(socket);

    // Connecting will send a start message to the video bridge..
    bridge.connect("moi");
    EXPECT_STREQ(
            "{\"from\":\"moi\",\"msg\":\"{\\\"handles\\\":[\\\"\\\"],"
            "\\\"start\\\":\\\"moi\\\"}\"}",
            socket->mSend.front().c_str());
}

TEST(WebRtcBridge, disconnectNotifiesTheBridge) {
    TestAsyncSocketAdapter* socket =
            new TestAsyncSocketAdapter({"hello", "world"});
    TestBridge bridge(socket);

    // Disconnect will send a disconnect message to the video bridge..
    bridge.connect("moi");
    bridge.disconnect("moi");
    EXPECT_STREQ("{\"msg\":\"disconnected\",\"topic\":\"moi\"}",
                 socket->mSend.back().c_str());
}

TEST(WebRtcBridge, nextMessage) {
    // Note that the bridge owns the socket
    TestAsyncSocketAdapter* socket =
            new TestAsyncSocketAdapter({msg1.dump(), msg2.dump()});
    TestBridge bridge(socket);
    std::string msg;

    bridge.connect("moi");

    // Signal data from the socket, it will keep pulling until the whole buffer
    // is empty.
    socket->signalRecv();

    bridge.nextMessage("moi", &msg, 10);
    EXPECT_STREQ("hello", msg.c_str());
}

TEST(WebRtcBridge, nextMessageByeDropsAll) {
    // Note that the bridge owns the socket
    TestAsyncSocketAdapter* socket =
            new TestAsyncSocketAdapter({msg1.dump(), msgBye.dump()});
    TestBridge bridge(socket);
    std::string msg;

    // Connect with id moi, and block and wait for message..
    bridge.connect("moi");

    // Signal data from the socket, it will keep pulling until the whole buffer
    // is empty.
    socket->signalRecv();

    // Bye causes all the events to be dropped.. So farewell!
    bridge.nextMessage("moi", &msg, 10);
    EXPECT_STREQ("{ \"bye\" : \"disconnected\" }", msg.c_str());
}

TEST(WebRtcBridge, nextMessageVideoBridgeClosesDropsAll) {
    // Note that the bridge owns the socket
    TestAsyncSocketAdapter* socket =
            new TestAsyncSocketAdapter({msg1.dump(), msgBye.dump()});

    TestBridge bridge(socket);
    std::string msg;

    // Connect with id moi, and block and wait for message..
    bridge.connect("moi");
    bridge.connect("Garfield");

    socket->close();

    // Bye causes all the events to be dropped.. So farewell!
    bridge.nextMessage("Garfield", &msg, 10);
    EXPECT_STREQ("{ \"bye\" : \"disconnected\" }", msg.c_str());

    bridge.nextMessage("moi", &msg, 10);
    EXPECT_STREQ("{ \"bye\" : \"disconnected\" }", msg.c_str());
}

TEST(WebRtcBridge, nextMessageByeDisconnects) {
    // Note that the bridge owns the socket
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});

    TestBridge bridge(socket);
    std::string msg;

    // Connect with id moi, and block and wait for message..
    bridge.connect("moi");

    // Signal a new message
    socket->signalRecv(msg1.dump());

    // Bye causes all the events to be dropped.. So farewell!
    bridge.nextMessage("moi", &msg, 10);
    EXPECT_STREQ("hello", msg.c_str());

    // Signal a new message
    socket->signalRecv(msgBye.dump());

    // Bye causes all the events to be dropped.. So farewell!
    bridge.nextMessage("moi", &msg, 10);
    EXPECT_STREQ("{ \"bye\" : \"disconnected\" }", msg.c_str());
}

TEST(WebRtcBridge, unconnectedSaysBye) {
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});

    TestBridge bridge(socket);
    std::string msg;
    EXPECT_FALSE(bridge.nextMessage("unknown", &msg, 1));
    EXPECT_STREQ("{ \"bye\" : \"disconnected\" }", msg.c_str());
}

TEST(WebRtcBridge, connectedTimeoutSaysNothing) {
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});
    WebRtcBridge bridge(socket, &fakeAgents, WebRtcBridge::kMaxFPS, 1234);
    std::string msg;

    // Connect with id moi
    bridge.connect("moi");

    // No messages during timeout will be empty string..
    EXPECT_FALSE(bridge.nextMessage("moi", &msg, 1));
    EXPECT_STREQ("", msg.c_str());
}

TEST(WebRtcBridge, deliverWithoutLoss) {
    // This will expose if you have locking issues and are losing messages.
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});
    TestBridge bridge(socket);
    // Connect with id moi
    bridge.connect("moi");

    // Let's not push more message into our buffer than we can handle. Some
    // build bots are very overworked and can get into a state where the reader
    // thread is not scheduled in time.
    const int maxMessageToPush = WebRtcBridge::kMaxMessageQueueLen - 1;

    // No messages during timeout will be empty string..
    base::FunctorThread messagePusher([&socket] {
        json jsmsg = {{"topic", "moi"}, {"msg", "hello"}};
        for (int i = 0; i < maxMessageToPush; i++) {
            jsmsg["msg"] = std::to_string(i);
            socket->signalRecv(jsmsg.dump());
        }
    });
    base::FunctorThread messageRec([&bridge] {
        for (int i = 0; i < maxMessageToPush; i++) {
            std::string msg;
            std::string expected = std::to_string(i);
            bool fetch = bridge.nextMessage("moi", &msg, 1000);
            EXPECT_TRUE(fetch);
            EXPECT_STREQ(expected.c_str(), msg.c_str());
        }
    });
    messagePusher.start();
    messageRec.start();
    messagePusher.wait(nullptr);
    messageRec.wait(nullptr);
}

TEST(WebRtcBridge, OutOfOrderConnectDisconnect) {
    // This should not crash, and that we can handle
    // any order of connects, disconnects from any place.
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});

    TestBridge bridge(socket);
    bridge.connect("moi");

    base::FunctorThread messagePusher([&socket] {
        json jsmsg = {{"bye", "farewell"}};
        for (int j = 0; j < 10; j++) {
            for (int i = 0; i < 10; i++) {
                jsmsg["topic"] = std::to_string(i);
                socket->signalRecv(jsmsg.dump());
            }
        }
    });
    base::FunctorThread messageRec([&bridge] {
        for (int j = 0; j < 10; j++) {
            for (int i = 0; i < 10; i++) {
                std::string me = std::to_string(i);
                bridge.connect(me);
                bridge.disconnect(me);
            }
        }
    });
    messagePusher.start();
    messageRec.start();
    messagePusher.wait(nullptr);
    messageRec.wait(nullptr);
}

TEST(WebRtcBridge, startChangesState) {
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});
    socket->close();
    TestBridge bridge(socket);
    EXPECT_EQ(bridge.state(), RtcBridge::BridgeState::Disconnected);
    EXPECT_TRUE(bridge.start());
    EXPECT_EQ(bridge.state(), RtcBridge::BridgeState::Connected);
}

TEST(WebRtcBridge, startStopSharedMemoryModule) {
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});
    socket->close();
    TestBridge bridge(socket);
    EXPECT_FALSE(socket->connected());
    gRtcRunning = false;
    gFps = 0;
    EXPECT_TRUE(bridge.start());

    // Activated rtc module, and a connection should have been opened.
    EXPECT_TRUE(gRtcRunning);
    EXPECT_TRUE(socket->connected());

    EXPECT_NE(0, gFps);
    EXPECT_NE(nullptr, gModule);
    bridge.terminate();

    // The rtc engine should have stopped, and socket closed.
    EXPECT_FALSE(gRtcRunning);
    EXPECT_FALSE(socket->connected());
}

#ifndef NDEBUG
TEST(WebRtcBridge, DISABLED_connectDisconnectParty) {
#else
TEST(WebRtcBridge, connectDisconnectParty) {
#endif
    // This should not crash, nor leak memory.
    // Looks like this drives ASAN off its rocker.
    TestAsyncSocketAdapter* socket = new TestAsyncSocketAdapter({});
    TestBridge bridge(socket);
    const int partyGoers = 50;
    const int times = 10;
    std::vector<std::unique_ptr<base::FunctorThread>> partyThreads;
    for (int i = 0; i < partyGoers; i++) {
        base::FunctorThread* t = new base::FunctorThread([&bridge, i] {
            std::string id = std::to_string(i);
            for (int j = 1; j < times; j++) {
                std::string msg;
                bridge.connect(id);
                bridge.nextMessage(id, &msg, 5);
                bridge.disconnect(id);
            }
        });
        partyThreads.push_back(std::unique_ptr<base::FunctorThread>(t));
    }

    // No messages during timeout will be empty string..
    base::FunctorThread messagePusher([&socket] {
        for (int j = 0; j < times; j++) {
            for (int i = 0; i < partyGoers; i++) {
                std::string id = std::to_string(i);
                json jsmsg = {{"topic", id}, {"bye", "farewell"}};
                for (int j = 0; j < 10; j++) {
                    for (int i = 0; i < 10; i++) {
                        jsmsg["topic"] = std::to_string(i);
                        socket->signalRecv(jsmsg.dump());
                    }
                }
            }
        };
    });

    messagePusher.start();
    for (auto thr = partyThreads.begin(); thr != partyThreads.end(); ++thr) {
        thr->get()->start();
    }
    messagePusher.wait(nullptr);

    // Lot's of threads, lots of waiters.. Be patient :-)
    for (auto thr = partyThreads.begin(); thr != partyThreads.end(); ++thr) {
        thr->get()->wait(nullptr);
    }
}
}  // namespace control
}  // namespace emulation
}  // namespace android
