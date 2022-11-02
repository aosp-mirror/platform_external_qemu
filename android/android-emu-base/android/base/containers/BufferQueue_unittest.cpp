// Copyright (C) 2016 The Android Open Source Project
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

#include "aemu/base/containers/BufferQueue.h"

#ifdef _WIN32
#undef ERROR  // Make LOG(ERROR) compile properly.
#endif

#include "aemu/base/Log.h"
#include "aemu/base/synchronization/Lock.h"
#include "aemu/base/synchronization/MessageChannel.h"
#include "aemu/base/threads/Thread.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

using android::base::AutoLock;
using android::base::BufferQueue;
using android::base::Lock;
using Result = android::base::BufferQueueResult;
using Buffer = std::string;

TEST(BufferQueue, Constructor) {
    Lock lock;
    BufferQueue<Buffer> queue(16, lock);
}

TEST(BufferQueue, popLockedBefore) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    AutoLock al(lock);
    Buffer msg;
    int64_t blockUntil = System::get()->getUnixTimeUs() + 2 * 1000;
    EXPECT_EQ(Result::Timeout, queue.popLockedBefore(&msg, blockUntil));
}

TEST(BufferQueue, popLockedBeforeWithDataPresent) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    AutoLock al(lock);
    Buffer msg;
    EXPECT_EQ(Result::Ok, queue.tryPushLocked(Buffer("Hello")));

    int64_t blockUntil = System::get()->getUnixTimeUs() + 2 * 1000;
    EXPECT_EQ(Result::Ok, queue.popLockedBefore(&msg, blockUntil));
    EXPECT_STREQ("Hello", msg.c_str());
}

TEST(BufferQueue, tryPushLocked) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    AutoLock al(lock);

    EXPECT_EQ(Result::Ok, queue.tryPushLocked(Buffer("Hello")));
    EXPECT_EQ(Result::Ok, queue.tryPushLocked(Buffer("World")));

    Buffer buff0("You Shall Not Move");
    EXPECT_EQ(Result::TryAgain, queue.tryPushLocked(std::move(buff0)));
    EXPECT_FALSE(buff0.empty()) << "Buffer should not be moved on failure!";
}

TEST(BufferQueue, tryPushLockedOnClosedQueue) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    AutoLock al(lock);

    EXPECT_EQ(Result::Ok, queue.tryPushLocked(Buffer("Hello")));

    // Closing the queue prevents pushing new items to the queue.
    queue.closeLocked();

    EXPECT_EQ(Result::Error, queue.tryPushLocked(Buffer("World")));
}

TEST(BufferQueue, tryPopLocked) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    AutoLock al(lock);

    Buffer buffer;
    EXPECT_EQ(Result::TryAgain, queue.tryPopLocked(&buffer));

    EXPECT_EQ(Result::Ok, queue.tryPushLocked(Buffer("Hello")));
    EXPECT_EQ(Result::Ok, queue.tryPushLocked(Buffer("World")));

    EXPECT_EQ(Result::Ok, queue.tryPopLocked(&buffer));
    EXPECT_STREQ("Hello", buffer.data());

    EXPECT_EQ(Result::Ok, queue.tryPopLocked(&buffer));
    EXPECT_STREQ("World", buffer.data());

    EXPECT_EQ(Result::TryAgain, queue.tryPopLocked(&buffer));
    EXPECT_STREQ("World", buffer.data());
}

TEST(BufferQueue, tryPopLockedOnClosedQueue) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    AutoLock al(lock);

    Buffer buffer;
    EXPECT_EQ(Result::TryAgain, queue.tryPopLocked(&buffer));

    EXPECT_EQ(Result::Ok, queue.tryPushLocked(Buffer("Hello")));
    EXPECT_EQ(Result::Ok, queue.tryPushLocked(Buffer("World")));

    EXPECT_EQ(Result::Ok, queue.tryPopLocked(&buffer));
    EXPECT_STREQ("Hello", buffer.data());

    // Closing the queue doesn't prevent popping existing items, but
    // will generate Result::Error once it is empty.
    queue.closeLocked();

    EXPECT_EQ(Result::Ok, queue.tryPopLocked(&buffer));
    EXPECT_STREQ("World", buffer.data());

    EXPECT_EQ(Result::Error, queue.tryPopLocked(&buffer));
    EXPECT_STREQ("World", buffer.data());
}

namespace {

// A TestThread instance that holds a reference to a queue and can either
// push or pull to it, on command from another thread. This uses a
// MessageChannel to implement the communication channel between the
// command thread and this one.
class TestThread final : public android::base::Thread {
public:
    TestThread(Lock& lock, BufferQueue<Buffer>& queue)
        // NOTE: The default stack size of android::base::Thread is too
        //       small and will result in runtime errors.
        : mLock(lock), mQueue(queue) {}

    // Tell the test thread to push |buffer| to the queue.
    // Call endPush() later to get the command's result.
    bool startPush(Buffer&& buffer) {
        return mInput.send(
                Request{.cmd = Cmd::Push, .buffer = std::move(buffer)});
    }

    // Get the result of a previous startPush() command.
    Result endPush() {
        Reply reply = {};
        if (!mOutput.receive(&reply)) {
            return Result::Error;
        }
        return reply.result;
    }

    // Tell the test thread to pop a buffer from the queue.
    // Call endPop() to get the command's result, as well as the popped
    // buffer if it is Result::Ok.
    bool startPop() { return mInput.send(Request{.cmd = Cmd::Pop}); }

    // Return the result of a previous startPop() command. If result is
    // Result::Ok, sets |*buffer| to the result buffer.
    Result endPop(Buffer* buffer) {
        Reply reply = {};
        if (!mOutput.receive(&reply)) {
            return Result::Error;
        }
        if (reply.result == Result::Ok) {
            *buffer = std::move(reply.buffer);
        }
        return reply.result;
    }

    // Tell the test thread to close the queue from its side.
    void doClose() { mInput.send(Request{.cmd = Cmd::Close}); }

    // Tell the test thread to stop after completing its current command.
    void stop() {
        mInput.send(Request{.cmd = Cmd::Stop});
        ASSERT_TRUE(this->wait());
    }

private:
    enum class Cmd {
        Push,
        Pop,
        Close,
        Stop,
    };

    struct Request {
        Cmd cmd;
        Buffer buffer;
    };

    struct Reply {
        Result result;
        Buffer buffer;
    };

    // Main thread function.
    virtual intptr_t main() override final {
        for (;;) {
            Request r;
            if (!mInput.receive(&r)) {
                LOG(ERROR) << "Could not receive command";
                break;
            }
            if (r.cmd == Cmd::Stop) {
                break;
            }
            mLock.lock();
            Reply reply = {};
            bool sendReply = false;
            switch (r.cmd) {
                case Cmd::Push:
                    reply.result = mQueue.pushLocked(std::move(r.buffer));
                    sendReply = true;
                    break;

                case Cmd::Pop:
                    reply.result = mQueue.popLocked(&reply.buffer);
                    sendReply = true;
                    break;

                case Cmd::Close:
                    mQueue.closeLocked();
                    break;

                default:;
            }
            mLock.unlock();
            if (sendReply) {
                if (!mOutput.send(std::move(reply))) {
                    LOG(ERROR) << "Could not send reply";
                    break;
                }
            }
        }
        return 0U;
    }

    Lock& mLock;
    BufferQueue<Buffer>& mQueue;
    android::base::MessageChannel<Request, 4> mInput;
    android::base::MessageChannel<Reply, 4> mOutput;
};

}  // namespace

TEST(BufferQueue, pushLocked) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    TestThread thread(lock, queue);

    ASSERT_TRUE(thread.start());
    ASSERT_TRUE(thread.startPop());

    lock.lock();
    EXPECT_EQ(Result::Ok, queue.pushLocked(Buffer("Hello")));
    EXPECT_EQ(Result::Ok, queue.pushLocked(Buffer("World")));
    EXPECT_EQ(Result::Ok, queue.pushLocked(Buffer("Foo")));
    lock.unlock();

    thread.stop();
}

TEST(BufferQueue, pushLockedWithClosedQueue) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    TestThread thread(lock, queue);

    ASSERT_TRUE(thread.start());

    lock.lock();
    EXPECT_EQ(Result::Ok, queue.pushLocked(Buffer("Hello")));
    // Closing the queue prevents pushing new items, but not
    // pulling from the queue.
    queue.closeLocked();
    EXPECT_EQ(Result::Error, queue.pushLocked(Buffer("World")));
    lock.unlock();

    Buffer buffer;
    ASSERT_TRUE(thread.startPop());
    EXPECT_EQ(Result::Ok, thread.endPop(&buffer));
    EXPECT_STREQ("Hello", buffer.data());

    thread.stop();
}

TEST(BufferQueue, popLocked) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    TestThread thread(lock, queue);

    ASSERT_TRUE(thread.start());
    ASSERT_TRUE(thread.startPush(Buffer("Hello World")));
    EXPECT_EQ(Result::Ok, thread.endPush());

    lock.lock();
    Buffer buffer;
    EXPECT_EQ(Result::Ok, queue.popLocked(&buffer));
    EXPECT_STREQ("Hello World", buffer.data());
    lock.unlock();

    thread.stop();
}

TEST(BufferQueue, popLockedWithClosedQueue) {
    Lock lock;
    BufferQueue<Buffer> queue(2, lock);
    TestThread thread(lock, queue);

    ASSERT_TRUE(thread.start());
    ASSERT_TRUE(thread.startPush(Buffer("Hello World")));
    EXPECT_EQ(Result::Ok, thread.endPush());

    // Closing the queue shall not prevent pulling items from it.
    // After that, Result::Error shall be returned.
    thread.doClose();

    ASSERT_TRUE(thread.startPush(Buffer("Foo Bar")));
    EXPECT_EQ(Result::Error, thread.endPush());

    lock.lock();
    Buffer buffer;
    EXPECT_EQ(Result::Ok, queue.popLocked(&buffer));
    EXPECT_STREQ("Hello World", buffer.data());

    EXPECT_EQ(Result::Error, queue.popLocked(&buffer));
    EXPECT_STREQ("Hello World", buffer.data());
    lock.unlock();

    thread.stop();
}

}  // namespace base
}  // namespace android
