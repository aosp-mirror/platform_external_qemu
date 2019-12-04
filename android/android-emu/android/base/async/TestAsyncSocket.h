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
#include "emulator/net/AsyncSocketAdapter.h"

#include <string>
#include <thread>
#include <vector>

#include "android/base/Log.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

using namespace android;
using android::base::System;

namespace emulator {
namespace net {
class TestAsyncSocketAdapter : public AsyncSocketAdapter {
public:
    TestAsyncSocketAdapter(std::vector<std::string> recv,
                           bool connected = false)
        : mRecv(recv), mConnected(connected) {
        // Let's not get chatty during unit tests.
        base::setMinLogLevel(base::LOG_INFO);
    }

    void asyncReader() {
        mAsycnRead = true;
        auto t = std::thread([this]() {
            base::AutoLock lock(mReadLock);
            while (mConnected) {
                while (!mRecv.empty()) {
                    mReadLock.unlock();
                    mListener->onRead(this);
                    mReadLock.lock();
                }
                while (mRecv.empty() && mConnected) {
                    mAsycnReadCv.wait(&mReadLock);
                }
            }
        });
        t.detach();
    }
    ~TestAsyncSocketAdapter() = default;
    void close() override {
        if (mListener)
            mListener->onClose(this, 123);
        mConnected = false;
        signalRecv();
    }

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
        signalRecv();
        return bufferSize;
    }

    bool connect() override {
        if (!mConnected && mListener)
            mListener->onConnected(this);

        mConnected = true;
        mCv.signal();
        return true;
    }

    bool connected() override { return mConnected; }

    // This will push out all the messages.
    void signalRecv() {
        if (mAsycnRead)
            mAsycnReadCv.signal();
        else
            mListener->onRead(this);
    }

    // Signal the arrival of a new message..
    void signalRecv(std::string msg) {
        {
            base::AutoLock lock(mReadLock);
            mRecv.push_back(msg);
        }
        signalRecv();
    }

    void signalRecv(std::vector<std::string> msgs) {
        addRecv(msgs);
        signalRecv();
    }

    void addRecv(std::vector<std::string> msgs) {
        base::AutoLock lock(mReadLock);
        for (const auto& msg : msgs)
            mRecv.push_back(msg);
    }
    bool waitForConnect(int timeoutMs = 1000) {
        base::AutoLock lock(mConnectLock);
        if (!mConnected)
            mCv.timedWait(&mConnectLock,
                          System::get()->getUnixTimeUs() + timeoutMs * 1000);
        return mConnected;
    }

    bool waitForFlush(int timeoutMs = 1000) {
        base::AutoLock lock(mConnectLock);
        auto endTime = System::get()->getUnixTimeUs() + timeoutMs * 1000;
        while (!mRecv.empty() && System::get()->getUnixTimeUs() < endTime) {
            // Keep on waiting.
            mCv.timedWait(&mConnectLock, endTime);
        }
        return mRecv.empty();
    }
    std::vector<std::string> mSend;
    std::vector<std::string> mRecv;

    bool mAsycnRead{false};
    bool mConnected{false};
    base::Lock mReadLock;
    base::Lock mSendLock;

    base::Lock mConnectLock;

    base::ConditionVariable mCv;

    base::ConditionVariable mAsycnReadCv;
};
}  // namespace net
}  // namespace emulator