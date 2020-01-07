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

#include <atomic>
#include <mutex>
#include <queue>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include "android/base/Log.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/testing/TestLooper.h"

using namespace android;
using android::base::AutoLock;
using android::base::Lock;
using android::base::System;

namespace emulator {
namespace net {

using Message = std::list<std::string>;
using CallResponse = std::pair<Message, Message>;

// This is a Mock AsyncSocket which basically mimics a socket.
// You can call addRecvs to pair a sequence of messages with a sequence of
// responses.
class TestAsyncSocketAdapter : public AsyncSocketAdapter {
public:
    TestAsyncSocketAdapter()
        : mLooperThread(&TestAsyncSocketAdapter::looperPump, this) {
        start();
    }

    ~TestAsyncSocketAdapter() { dispose(); }

    void looperPump() {
        {
            std::unique_lock<std::mutex> lock(mLooperLock);
            mLooperRunning = true;
            mLooperCv.notify_one();
        }
        std::function<void()> callback;
        while (mLooperRunning) {
            if (mLooper.receive(&callback))
                callback();
        }
    }

    void close() override {
        AutoLock watchLock(mConnectLock);
        if (!mConnected)
            return;

        mLooper.send([this]() {
            AutoLock watchLock(mConnectLock);
            if (mListener)
                mListener->onClose(this, 123);

            mConnected = false;
            mCloseCv.signal();
        });

        while (mConnected)
            mCloseCv.wait(&mConnectLock);
    }

    uint64_t recv(char* buffer, uint64_t bufferSize) override {
        AutoLock lock(mReadLock);
        {
            AutoLock lock(mResponsesLock);
            // Eof, we have send all our responses.
            if (mResponses.empty() && mRecv.empty()) {
                LOG(INFO) << "EOF";
                return 0;
            }

            // Exhausted our messages.. Come back later..
            if (mRecv.empty()) {
                errno = EAGAIN;
                return -1;
            }
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
        AutoLock sendLock(mSendLock);
        std::string msg(buffer, bufferSize);
        AutoLock lock(mResponsesLock);
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
            mLooper.send([this, response]() { sendOutResponses(response); });
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
        if (!mConnected) {
            mLooper.send([this]() {
                AutoLock lock(mConnectLock);
                if (mListener)
                    mListener->onConnected(this);

                mConnected = true;
                mConnectCv.signal();
            });
        }

        return true;
    }

    bool connectSync(uint64_t timeoutms = 1000) override {
        AutoLock watchLock(mConnectLock);
        if (connected())
            return true;
        connect();
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
        AutoLock lock(mResponsesLock);
        mResponses.push(std::pair<Message, Message>(send, respond));
    }

    void dispose() override {
        std::unique_lock<std::mutex> lock(mLooperLock);
        if (mDisposed)
            return;

        mLooperRunning = false;
        mLooper.stop();
        mLooperThread.join();
        mConnected = false;
        mConnectCv.signal();
        mCloseCv.signal();
        mDisposed = true;
    }

private:
    void start() {
        std::unique_lock<std::mutex> lock(mLooperLock);
        while (!mLooperRunning)
            mLooperCv.wait(lock);
    };

    std::vector<std::string> mRecv;
    std::queue<CallResponse> mResponses;
    std::atomic<bool> mConnected{false};
    bool mDisposed{false};

    Lock mSendLock;
    Lock mReadLock;
    Lock mResponsesLock;
    Lock mConnectLock;
    base::ConditionVariable mConnectCv;
    base::ConditionVariable mCloseCv;

    // Looper thread management
    std::atomic<bool> mLooperRunning{false};
    std::mutex mLooperLock;
    std::condition_variable mLooperCv;
    base::MessageChannel<std::function<void()>, 32> mLooper;

    std::thread mLooperThread;

};  // namespace net
}  // namespace net
}  // namespace emulator