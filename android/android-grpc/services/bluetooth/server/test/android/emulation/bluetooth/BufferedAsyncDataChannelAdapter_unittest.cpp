
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
#include "android/emulation/bluetooth/BufferedAsyncDataChannelAdapter.h"

#include <errno.h>        // for EAGAIN, errno
#include <gtest/gtest.h>  // for Message, TestPartResult
#include <condition_variable>
#include <memory>  // for unique_ptr
#include <mutex>
#include <string>       // for string, operator==, bas...
#include <thread>       // for thread
#include <type_traits>  // for remove_extent_t

#include "aemu/base/async/Looper.h"        // for Looper, Looper::Duration
#include "android/base/testing/TestEvent.h"   // for TestEvent
#include "android/base/testing/TestLooper.h"  // for TestLooper

namespace android {
namespace emulation {
namespace bluetooth {

class BufferedAsyncDataChannelAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        mLooper = std::unique_ptr<base::TestLooper>(new base::TestLooper());
    }

    void TearDown() override { mLooper.reset(); }

    void pumpLooperUntilEvent(TestEvent& event) {
        constexpr base::Looper::Duration kTimeoutMs = 1000;
        constexpr base::Looper::Duration kStep = 50;

        base::Looper::Duration current = mLooper->nowMs();
        const base::Looper::Duration deadline = mLooper->nowMs() + kTimeoutMs;

        while (!event.isSignaled() && current + kStep < deadline) {
            mLooper->runOneIterationWithDeadlineMs(current + kStep);
            current = mLooper->nowMs();
        }

        event.wait();
    }

    void emptyPump() {
        // Empty out the pump;
        mLooper->runOneIterationWithDeadlineMs(mLooper->nowMs() + 100);
    }

    std::unique_ptr<base::TestLooper> mLooper;
};

TEST_F(BufferedAsyncDataChannelAdapterTest, no_data_should_result_in_EAGAIN) {
    constexpr size_t size = 1024;
    uint8_t buf[512];
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    EXPECT_EQ(channel->Recv(buf, sizeof(buf)), -1);
    EXPECT_EQ(errno, EAGAIN);
}

TEST_F(BufferedAsyncDataChannelAdapterTest, no_write_buffer_full_send) {
    constexpr size_t size = 1024;
    std::string sampleData(size, 'x');
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    EXPECT_EQ(channel->Send((uint8_t*)sampleData.data(), size), size)
            << "Buffer should be able to hold " << size << " bytes";
    EXPECT_EQ(channel->Send((uint8_t*)sampleData.data(), size), 0)
            << "Buffer should be filled to capacity";
}

TEST_F(BufferedAsyncDataChannelAdapterTest, no_write_buffer_full_recv) {
    constexpr size_t size = 1024;
    std::string sampleData(size, 'x');
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    EXPECT_EQ(channel->WriteToRecvBuffer((uint8_t*)sampleData.data(), size),
              size)
            << "Buffer should be able to hold " << size << " bytes";
    EXPECT_EQ(channel->WriteToRecvBuffer((uint8_t*)sampleData.data(), size), 0)
            << "Buffer should be fille to capacity";
}

TEST_F(BufferedAsyncDataChannelAdapterTest, write_to_recv_raises_read) {
    constexpr size_t size = 1024;
    TestEvent receivedCallback;
    std::string sampleData(size, 'x');
    bool eventRaised = false;
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    channel->WatchForNonBlockingRead([&](auto from) {
        eventRaised = true;
        receivedCallback.signal();
    });

    EXPECT_EQ(channel->WriteToRecvBuffer((uint8_t*)sampleData.data(), size),
              size)
            << "Buffer should be able to hold " << size << " bytes";

    pumpLooperUntilEvent(receivedCallback);
    EXPECT_TRUE(eventRaised);
}

TEST_F(BufferedAsyncDataChannelAdapterTest,
       register_recv_callback_fires_immediately_if_data_present) {
    constexpr size_t size = 1024;
    bool eventRaised = false;
    TestEvent receivedCallback;
    std::string sampleData(size, 'x');

    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    EXPECT_EQ(channel->WriteToRecvBuffer((uint8_t*)sampleData.data(), size),
              size)
            << "Buffer should be able to hold " << size << " bytes";

    channel->WatchForNonBlockingRead([&](auto from) {
        eventRaised = true;
        receivedCallback.signal();
    });

    pumpLooperUntilEvent(receivedCallback);
    EXPECT_TRUE(eventRaised);
}

TEST_F(BufferedAsyncDataChannelAdapterTest, write_to_send_raises_write) {
    constexpr size_t size = 1024;
    TestEvent receivedCallback;
    std::string sampleData(size, 'x');
    bool eventRaised = false;

    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());
    channel->WatchForNonBlockingSend([&](auto from) {
        eventRaised = true;
        receivedCallback.signal();
    });

    EXPECT_EQ(channel->Send((uint8_t*)sampleData.data(), size), size)
            << "Buffer should be able to hold " << size << " bytes";

    pumpLooperUntilEvent(receivedCallback);
    EXPECT_TRUE(eventRaised);
}

TEST_F(BufferedAsyncDataChannelAdapterTest,
       register_send_callback_fires_immediately_if_data_present) {
    constexpr size_t size = 1024;
    TestEvent receivedCallback;
    std::string sampleData(size, 'x');
    bool eventRaised = false;

    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    EXPECT_EQ(channel->Send((uint8_t*)sampleData.data(), size), size)
            << "Buffer should be able to hold " << size << " bytes";

    channel->WatchForNonBlockingSend([&](auto from) {
        eventRaised = true;
        receivedCallback.signal();
    });

    pumpLooperUntilEvent(receivedCallback);
    EXPECT_TRUE(eventRaised);
}

TEST_F(BufferedAsyncDataChannelAdapterTest, you_read_what_you_write) {
    constexpr size_t size = 1024;
    constexpr size_t chunk = 128;
    TestEvent receivedCallback;
    std::string sampleData(size, 'x');
    std::string recv;

    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    std::thread writer([&]() {
        for (int i = 0; i < size; i += chunk) {
            auto toWrite = chunk;
            while (toWrite > 0) {
                toWrite -= channel->WriteToRecvBuffer(
                        (uint8_t*)sampleData.data(), toWrite);
            }
        }
    });

    channel->WatchForNonBlockingRead([&](auto from) {
        ssize_t read = 1;
        while (read > 0) {
            uint8_t buf[512];
            read = from->Recv(buf, sizeof(buf));
            if (read > 0)
                recv.append(std::string((char*)buf, read));
        }
        if (recv == sampleData)
            receivedCallback.signal();
    });

    pumpLooperUntilEvent(receivedCallback);
    emptyPump();
    writer.join();
}

TEST_F(BufferedAsyncDataChannelAdapterTest, you_write_what_you_read) {
    constexpr size_t size = 1024;
    constexpr size_t chunk = 128;
    TestEvent receivedCallback;
    std::string sampleData(size, 'x');
    std::string recv;

    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    std::thread writer([&]() {
        for (int i = 0; i < size; i += chunk) {
            auto toWrite = chunk;
            while (toWrite > 0) {
                auto snd = channel->Send((uint8_t*)sampleData.data(), toWrite);
                toWrite -= snd;
            }
        }
    });

    channel->WatchForNonBlockingSend([&](auto from) {
        ssize_t read = 1;
        while (read > 0) {
            uint8_t buf[512];
            read = from->ReadFromSendBuffer(buf, sizeof(buf));
            if (read > 0)
                recv.append(std::string((char*)buf, read));
        }
        if (recv == sampleData)
            receivedCallback.signal();
    });

    pumpLooperUntilEvent(receivedCallback);
    emptyPump();
    writer.join();
}

TEST_F(BufferedAsyncDataChannelAdapterTest, tell_me_you_closed) {
    constexpr size_t size = 1024;
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    TestEvent receivedCallback;

    channel->WatchForClose([&](auto con) {
        EXPECT_FALSE(con->isOpen());
        receivedCallback.signal();
    });

    channel->Close();
    pumpLooperUntilEvent(receivedCallback);
}

TEST_F(BufferedAsyncDataChannelAdapterTest,
       no_removal_of_close_when_in_callback) {
    using ms = std::chrono::milliseconds;
    constexpr size_t size = 1024;
    std::mutex waitForClose;
    std::condition_variable closeCv;
    bool isOpen = true;
    bool can_continue = false;
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    TestEvent receivedCallback;

    channel->WatchForClose([&](auto con) {
        EXPECT_FALSE(con->isOpen());
        {
            std::unique_lock<std::mutex> guard(waitForClose);
            isOpen = false;
            closeCv.notify_one();
        }
        receivedCallback.signal();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    auto t1 = std::thread([this, &receivedCallback] {
        pumpLooperUntilEvent(receivedCallback);
    });

    channel->Close();
    {
        std::unique_lock<std::mutex> lk(waitForClose);
        closeCv.wait(lk, [&] { return !isOpen; });
    }

    auto start = std::chrono::system_clock::now();

    // We have a 100ms sleep, so this should take AT LEAST 50ms..
    channel->WatchForClose(nullptr);
    auto end = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<ms>(end - start);
    EXPECT_LT(50, diff.count());
    t1.join();
}

TEST_F(BufferedAsyncDataChannelAdapterTest,
       no_removal_of_read_when_in_callback) {
    using ms = std::chrono::milliseconds;
    constexpr size_t size = 1024;
    std::string data('x', 100);
    std::mutex waitForRead;
    std::condition_variable readCv;
    bool isNotified = true;
    bool can_continue = false;
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    TestEvent receivedCallback;

    channel->WatchForNonBlockingRead([&](auto con) {
        {
            std::unique_lock<std::mutex> guard(waitForRead);
            isNotified = false;
            readCv.notify_one();
        }
        receivedCallback.signal();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    auto t1 = std::thread([this, &receivedCallback] {
        pumpLooperUntilEvent(receivedCallback);
    });

    channel->WriteToRecvBuffer((uint8_t*)data.data(), data.size());
    {
        std::unique_lock<std::mutex> lk(waitForRead);
        readCv.wait(lk, [&] { return !isNotified; });
    }

    auto start = std::chrono::system_clock::now();

    // We have a 100ms sleep, so this should take AT LEAST 50ms..
    channel->WatchForNonBlockingRead(nullptr);
    auto end = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<ms>(end - start);
    EXPECT_LT(50, diff.count());
    t1.join();
}

TEST_F(BufferedAsyncDataChannelAdapterTest,
       no_removal_of_send_when_in_callback) {
    using ms = std::chrono::milliseconds;
    constexpr size_t size = 1024;
    std::string data('x', 100);
    std::mutex waitForRead;
    std::condition_variable sendCv;
    bool isNotified = true;
    bool can_continue = false;
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    TestEvent receivedCallback;

    channel->WatchForNonBlockingSend([&](auto con) {
        {
            std::unique_lock<std::mutex> guard(waitForRead);
            isNotified = false;
            sendCv.notify_one();
        }
        receivedCallback.signal();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    auto t1 = std::thread([this, &receivedCallback] {
        pumpLooperUntilEvent(receivedCallback);
    });

    channel->Send((uint8_t*)data.data(), data.size());
    {
        std::unique_lock<std::mutex> lk(waitForRead);
        sendCv.wait(lk, [&] { return !isNotified; });
    }

    auto start = std::chrono::system_clock::now();

    // We have a 100ms sleep, so this should take AT LEAST 50ms..
    channel->WatchForNonBlockingSend(nullptr);
    auto end = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<ms>(end - start);
    EXPECT_LT(50, diff.count());
    t1.join();
}

TEST_F(BufferedAsyncDataChannelAdapterTest, safely_send_out_of_scope) {
    TestEvent receivedCallback;
    constexpr size_t size = 1024;
    std::string data('x', 100);

    {
        auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

        channel->WatchForNonBlockingSend(
                [&](auto con) { receivedCallback.signal(); });
        channel->Send((uint8_t*)data.data(), data.size());
    }

    // We are scheduled on the looper, and are only pumping out when channel is
    // out of scope.
    pumpLooperUntilEvent(receivedCallback);
}

TEST_F(BufferedAsyncDataChannelAdapterTest, safely_recv_out_of_scope) {
    TestEvent receivedCallback;
    constexpr size_t size = 1024;
    std::string data('x', 100);

    {
        auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

        channel->WatchForNonBlockingRead(
                [&](auto con) { receivedCallback.signal(); });
        channel->WriteToRecvBuffer((uint8_t*)data.data(), data.size());
    }

    // We are scheduled on the looper, and are only pumping out when channel is
    // out of scope.
    pumpLooperUntilEvent(receivedCallback);
}

TEST_F(BufferedAsyncDataChannelAdapterTest, safely_close_out_of_scope) {
    TestEvent receivedCallback;
    constexpr size_t size = 1024;
    std::string data('x', 100);

    {
        auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

        channel->WatchForClose([&](auto con) { receivedCallback.signal(); });
        channel->Close();
    }

    // We are scheduled on the looper, and are only pumping out when channel is
    // out of scope.
    pumpLooperUntilEvent(receivedCallback);
}

TEST_F(BufferedAsyncDataChannelAdapterTest, safely_set_send_callback_from_within) {
    TestEvent receivedCallback;
    constexpr size_t size = 1024;
    std::string data('x', 100);
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    channel->WatchForNonBlockingSend([&](auto con) {
        channel->WatchForNonBlockingSend({});
        receivedCallback.signal();
    });
    channel->Send((uint8_t*)data.data(), data.size());

    // We are scheduled on the looper, and are only pumping out when channel is
    // out of scope.
    pumpLooperUntilEvent(receivedCallback);
}


TEST_F(BufferedAsyncDataChannelAdapterTest, safely_set_recv_callback_from_within) {
    TestEvent receivedCallback;
    constexpr size_t size = 1024;
    std::string data('x', 100);
    auto channel = BufferedAsyncDataChannel::create(size, mLooper.get());

    channel->WatchForNonBlockingRead([&](auto con) {
        channel->WatchForNonBlockingRead({});
        receivedCallback.signal();
    });
    channel->WriteToRecvBuffer((uint8_t*)data.data(), data.size());

    // We are scheduled on the looper, and are only pumping out when channel is
    // out of scope.
    pumpLooperUntilEvent(receivedCallback);
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android