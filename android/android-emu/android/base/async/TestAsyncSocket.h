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
#pragma once
#include <gtest/gtest.h>
#include "emulator/net/AsyncSocketAdapter.h"

#include <queue>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include "android/base/Log.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/testing/TestLooper.h"

using namespace android;
using android::base::System;
using android::base::Lock;
using android::base::AutoLock;


namespace emulator {
namespace net {

using Message = std::list<std::string>;
using CallResponse = std::pair<Message, Message>;

// This is a Mock AsyncSocket which basically mimics a socket.
// You can call addRecvs to pair a sequence of messages with a sequence of responses.
class TestAsyncSocketAdapter : public AsyncSocketAdapter {
public:
    TestAsyncSocketAdapter() {}

    void looperPump() {
        while (mLooperRunning) {
            mLooper.runWithTimeoutMs(100);
        }
    }

    ~TestAsyncSocketAdapter(){

    };

    void start() {
        mLooperRunning = true;
        mLooperThread = std::thread([this]() { looperPump(); });
    };

    void terminate() {
        mLooperRunning = false;
        mLooperThread.join();
    }

    void close() override {
        LOG(INFO) << "Close";

        if (mConnected) {
            mLooper.scheduleCallback([this]() {
                if (mListener)
                    mListener->onClose(this, 123);
            });
        }
        mConnected = false;
    }

    uint64_t recv(char* buffer, uint64_t bufferSize) override {
        AutoLock lock(mReadLock);
        // Eof, we have send all our responses.
        if (mResponses.empty() && mRecv.empty()) {
            return 0;
        }

        // Exhausted our messages.. Come back later..
        if (mRecv.empty()) {
            errno = EAGAIN;
            return -1;
        }
        std::string msg = mRecv.front();
        LOG(INFO) << "Recv: " << msg;
        // Note that c_str() is guaranteed to be 0 terminated, so we always
        // have size() + 1 of in our string.
        uint64_t buflen = std::min<uint64_t>(bufferSize, msg.size() + 1);
        memcpy(buffer, msg.c_str(), buflen);
        mRecv.erase(mRecv.begin(), mRecv.begin() + 1);
        return buflen;
    }

    uint64_t send(const char* buffer, uint64_t bufferSize) override {
        AutoLock sendLock(mSendLock);
        std::string msg(buffer, bufferSize);
        if (mResponses.empty()) {
            EXPECT_EQ("Did not expect response: " + msg, msg);
        }

        CallResponse* response = &mResponses.front();
        Message* incoming = &response->first;
        auto expected = incoming->front();
        incoming->pop_front();

        // Check we received expected message.
        std::regex expect(expected);
        if (!std::regex_search(msg, expect))
            EXPECT_EQ(msg, expected);

        // Send out associated response if we got all incoming messages
        if (incoming->empty()) {
            auto response = mResponses.front().second;
            mLooper.scheduleCallback(
                    [this, response]() { sendOutResponses(response); });
            // Move to the next expected values..
            mResponses.pop();
        }
        return bufferSize;
    }

    void sendOutResponses(Message response) {
        {
            AutoLock readLock(mReadLock);
            for (auto msg : response) {
                mRecv.push_back(msg);
            }
        }
        if (mListener)
            mListener->onRead(this);
    }

    bool connect() override {
        if (!mConnected && mListener) {
            mLooper.scheduleCallback([this]() {
                AutoLock lock(mConnectLock);
                mListener->onConnected(this);
                mConnectCv.signal();
            });
        }

        mConnected = true;
        return true;
    }

    bool connectSync(uint64_t timeoutms) override {
        bool connect = this->connect();
        if (!connect)
            return false;

        AutoLock watchLock(mConnectLock);
        auto waituntilus = System::get()->getUnixTimeUs() + timeoutms * 1000;
        while (!connected() && System::get()->getUnixTimeUs() < waituntilus) {
            mConnectCv.timedWait(&mConnectLock, waituntilus);
        }

        return connected();
    }

    bool connected() override { return mConnected; }

    // This register the responses.
    // Expects to receive a sequence of regexes messages.
    // if the sequence is matched the respond object will be
    // send back to the client. For example:
    //
    // addRecvs({"incoming:foo.*"}, {"reply-bar"})
    //
    // After the socket gets a call send("incoming:foo-lalalala")
    // it will respond with a OnRead signal, followed by returning
    // "reply-bar"
    void addRecvs(Message send, Message respond) {
        mResponses.push(std::pair<Message, Message>(send, respond));
    }

    std::vector<std::string> mRecv;
    std::queue<CallResponse> mResponses;

    bool mFinished{false};
    bool mConnected{false};

    Lock mSendLock;
    Lock mReadLock;
    Lock mConnectLock;
    base::ConditionVariable mConnectCv;

    // Looper thread management
    std::atomic<bool> mLooperRunning{false};
    std::thread mLooperThread;
    android::base::TestLooper mLooper;
};  // namespace net
}  // namespace net
}  // namespace emulator