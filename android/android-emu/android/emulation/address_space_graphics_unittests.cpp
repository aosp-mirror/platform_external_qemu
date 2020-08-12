// Copyright 2019 The Android Open Source Project
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

#include <gtest/gtest.h>                                     // for Message
#include <stdint.h>                                          // for uint32_t
#include <stdio.h>                                           // for printf
#include <string.h>                                          // for size_t
#include <sys/types.h>                                       // for ssize_t
#include <algorithm>                                         // for uniform_...
#include <functional>                                        // for __base
#include <random>                                            // for default_...
#include <vector>                                            // for vector

#include "android/base/ring_buffer.h"                        // for ring_buf...
#include "android/base/threads/FunctorThread.h"              // for FunctorT...
#include "android/console.h"                                 // for getConso...
#include "android/emulation/AddressSpaceService.h"           // for AddressS...
#include "android/emulation/address_space_device.hpp"        // for goldfish...
#include "android/emulation/address_space_graphics.h"        // for AddressS...
#include "android/emulation/address_space_graphics_types.h"  // for asg_context
#include "android/emulation/hostdevices/HostAddressSpace.h"  // for HostAddr...
#include "android/globals.h"                                 // for android_hw

namespace android {
namespace base {
class Stream;
}  // namespace base
}  // namespace android

using android::base::FunctorThread;



namespace android {
namespace emulation {
namespace asg {

#define ASG_TEST_READ_PATTERN 0xAA
#define ASG_TEST_WRITE_PATTERN 0xBB

class AddressSpaceGraphicsTest : public ::testing::Test {
public:
    class Client {
    public:
        Client(HostAddressSpaceDevice* device) :
            mDevice(device),
            mHandle(mDevice->open()) {

            ping((uint64_t)AddressSpaceDeviceType::Graphics);

            auto getRingResult = ping((uint64_t)ASG_GET_RING);
            mRingOffset = getRingResult.metadata;
            mRingSize = getRingResult.size;

            EXPECT_EQ(0, mDevice->claimShared(mHandle, mRingOffset, mRingSize));

            mRingStorage =
                (char*)mDevice->getHostAddr(
                    mDevice->offsetToPhysAddr(mRingOffset));

            auto getBufferResult = ping((uint64_t)ASG_GET_BUFFER);
            mBufferOffset = getBufferResult.metadata;
            mBufferSize = getBufferResult.size;

            EXPECT_EQ(0, mDevice->claimShared(mHandle, mBufferOffset, mBufferSize));
            mBuffer =
                (char*)mDevice->getHostAddr(
                    mDevice->offsetToPhysAddr(mBufferOffset));

            mContext = asg_context_create(mRingStorage, mBuffer, mBufferSize);

            EXPECT_EQ(mBuffer, mContext.buffer);

            auto setVersionResult = ping((uint64_t)ASG_SET_VERSION, mVersion);
            uint32_t hostVersion = setVersionResult.size;
            EXPECT_LE(hostVersion, mVersion);
            EXPECT_EQ(android_hw->hw_gltransport_asg_writeStepSize,
                      mContext.ring_config->flush_interval);
            EXPECT_EQ(android_hw->hw_gltransport_asg_writeBufferSize,
                      mBufferSize);

            mContext.ring_config->transfer_mode = 1;
            mContext.ring_config->host_consumed_pos = 0;
            mContext.ring_config->guest_write_pos = 0;
            mBufferMask = mBufferSize - 1;

            mWriteStart = mBuffer;
        }

        ~Client() {
            mDevice->unclaimShared(mHandle, mBufferOffset);
            mDevice->unclaimShared(mHandle, mRingOffset);
            mDevice->close(mHandle);
        }

        bool isInError() const {
            return 1 == mContext.ring_config->in_error;
        }

        void abort() {
            mContext.ring_config->in_error = 1;
        }

        char* allocBuffer(size_t size) {
            if (size > mContext.ring_config->flush_interval) {
                return nullptr;
            }

            if (mWriteStart + mCurrentWriteBytes + size >
                mWriteStart + mWriteStep) {
                flush();
                mCurrentWriteBytes = 0;
            }

            char* res = mWriteStart + mCurrentWriteBytes;
            mCurrentWriteBytes += size;

            return res;
        }

        int writeFully(const char* buf, size_t size) {
            flush();
            ensureType1Finished();
            mContext.ring_config->transfer_size = size;
            mContext.ring_config->transfer_mode = 3;

            size_t sent = 0;
            size_t quarterRingSize = mBufferSize / 4;
            size_t chunkSize = size < quarterRingSize ? size : quarterRingSize;

            while (sent < size) {
                size_t remaining = size - sent;
                size_t sendThisTime = remaining < chunkSize ? remaining : chunkSize;

                long sentChunks =
                    ring_buffer_view_write(
                        mContext.to_host_large_xfer.ring,
                        &mContext.to_host_large_xfer.view,
                        buf + sent, sendThisTime, 1);

                if (*(mContext.host_state) != ASG_HOST_STATE_CAN_CONSUME) {
                    ping(ASG_NOTIFY_AVAILABLE);
                }

                if (sentChunks == 0) {
                    ring_buffer_yield();
                }

                sent += sentChunks * sendThisTime;

                if (isInError()) {
                    return -1;
                }
            }

            ensureType3Finished();
            mContext.ring_config->transfer_mode = 1;
            return 0;
        }

        ssize_t speculativeRead(char* readBuffer, size_t minSizeToRead) {
            flush();
            ensureConsumerFinishing();

            size_t actuallyRead = 0;
            while (!actuallyRead) {
                uint32_t readAvail =
                    ring_buffer_available_read(
                        mContext.from_host_large_xfer.ring,
                        &mContext.from_host_large_xfer.view);

                if (!readAvail) {
                    ring_buffer_yield();
                    continue;
                }

                uint32_t toRead = readAvail > minSizeToRead ?
                    minSizeToRead : readAvail;

                long stepsRead = ring_buffer_view_read(
                    mContext.from_host_large_xfer.ring,
                    &mContext.from_host_large_xfer.view,
                    readBuffer, toRead, 1);

                actuallyRead += stepsRead * toRead;

                if (isInError()) {
                    return -1;
                }
            }

            return actuallyRead;
        }

        void flush() {
            if (!mCurrentWriteBytes) return;
            type1WriteWithNotify(mWriteStart - mBuffer, mCurrentWriteBytes);
            advanceWrite();
        }

        uint32_t get_relative_buffer_pos(uint32_t pos) {
            return pos & mBufferMask;
        }

        uint32_t get_available_for_write() {
            uint32_t host_consumed_view;
            __atomic_load(&mContext.ring_config->host_consumed_pos,
                          &host_consumed_view,
                          __ATOMIC_SEQ_CST);
            uint32_t availableForWrite =
                get_relative_buffer_pos(
                    host_consumed_view -
                    mContext.ring_config->guest_write_pos - 1);
            return availableForWrite;
        }

        void advanceWrite() {
            uint32_t avail = get_available_for_write();

            while (avail < mContext.ring_config->flush_interval) {
                ensureConsumerFinishing();
                avail = get_available_for_write();
            }

            __atomic_add_fetch(
                &mContext.ring_config->guest_write_pos,
                mContext.ring_config->flush_interval,
                __ATOMIC_SEQ_CST);

            char* newBuffer =
                mBuffer +
                get_relative_buffer_pos(
                    mContext.ring_config->guest_write_pos);

            mWriteStart = newBuffer;
            mCurrentWriteBytes = 0;
        }

        int type1WriteWithNotify(uint32_t bufferOffset, size_t size) {
            size_t sent = 0;
            size_t sizeForRing = 8;

            struct asg_type1_xfer xfer {
                bufferOffset,
                (uint32_t)size,
            };

            uint8_t* writeBufferBytes = (uint8_t*)(&xfer);

            while (sent < sizeForRing) {

                long sentChunks = ring_buffer_write(
                        mContext.to_host, writeBufferBytes + sent, sizeForRing - sent, 1);

                if (*(mContext.host_state) != ASG_HOST_STATE_CAN_CONSUME) {
                    ping(ASG_NOTIFY_AVAILABLE);
                }

                if (sentChunks == 0) {
                    ring_buffer_yield();
                }

                sent += sentChunks * (sizeForRing - sent);

                if (isInError()) {
                    return -1;
                }
            }

            return 0;
        }

        void ensureConsumerFinishing() {
            uint32_t currAvailRead =
                ring_buffer_available_read(mContext.to_host, 0);

            while (currAvailRead) {
                ring_buffer_yield();
                uint32_t nextAvailRead = ring_buffer_available_read(mContext.to_host, 0);

                if (nextAvailRead != currAvailRead) {
                    break;
                }

                if (*(mContext.host_state) != ASG_HOST_STATE_CAN_CONSUME) {
                    ping(ASG_NOTIFY_AVAILABLE);
                    break;
                }
            }
        }

        void ensureType1Finished() {
            ensureConsumerFinishing();

            uint32_t currAvailRead =
                ring_buffer_available_read(mContext.to_host, 0);

            while (currAvailRead) {
                ring_buffer_yield();
                currAvailRead = ring_buffer_available_read(mContext.to_host, 0);
                if (isInError()) {
                    return;
                }
            }
        }

        void ensureType3Finished() {
            uint32_t availReadLarge =
                ring_buffer_available_read(
                    mContext.to_host_large_xfer.ring,
                    &mContext.to_host_large_xfer.view);
            while (availReadLarge) {
                ring_buffer_yield();
                availReadLarge =
                    ring_buffer_available_read(
                        mContext.to_host_large_xfer.ring,
                        &mContext.to_host_large_xfer.view);
                if (*(mContext.host_state) != ASG_HOST_STATE_CAN_CONSUME) {
                    ping(ASG_NOTIFY_AVAILABLE);
                }
                if (isInError()) {
                    return;
                }
            }
        }

        char* getBufferPtr() { return mBuffer; }

    private:

        AddressSpaceDevicePingInfo ping(uint64_t metadata, uint64_t size = 0) {
            AddressSpaceDevicePingInfo info;
            info.metadata = metadata;
            mDevice->ping(mHandle, &info);
            return info;
        }

        HostAddressSpaceDevice* mDevice;
        uint32_t mHandle;
        uint64_t mRingOffset;
        uint64_t mRingSize;
        uint64_t mBufferOffset;
        uint64_t mBufferSize;
        char* mRingStorage;
        char* mBuffer;
        struct asg_context mContext;
        uint32_t mVersion = 1;

        char* mWriteStart = 0;
        uint32_t mWriteStep = 0;
        uint32_t mCurrentWriteBytes = 0;
        uint32_t mBufferMask = 0;
    };

    class Consumer {
    public:
        Consumer(struct asg_context context,
                 ConsumerCallbacks callbacks) :
            mContext(context),
            mCallbacks(callbacks),
            mThread([this] { threadFunc(); }) {
            mThread.start();
        }

        ~Consumer() {
            mThread.wait();
        }

        void setRoundTrip(bool enabled,
                          uint32_t toHostBytes = 0,
                          uint32_t fromHostBytes = 0) {
            mRoundTripEnabled = enabled;
            if (mRoundTripEnabled) {
                mToHostBytes = toHostBytes;
                mFromHostBytes = fromHostBytes;
            }
        }

        void handleRoundTrip() {
            if (!mRoundTripEnabled) return;

            if (mReadPos == mToHostBytes) {
                std::vector<char> reply(mFromHostBytes, ASG_TEST_READ_PATTERN);
                uint32_t origBytes = mFromHostBytes;
                auto res = ring_buffer_write_fully_with_abort(
                    mContext.from_host_large_xfer.ring,
                    &mContext.from_host_large_xfer.view,
                    reply.data(),
                    mFromHostBytes,
                    1, &mContext.ring_config->in_error);
                if (res < mFromHostBytes) {
                    printf("%s: aborted write (%u vs %u %u). in error? %u\n", __func__,
                            res, mFromHostBytes, origBytes,
                           mContext.ring_config->in_error);
                    EXPECT_EQ(1, mContext.ring_config->in_error);
                }
                mReadPos = 0;
            }
        }

        void ensureWritebackDone() {
            while (mReadPos) {
                ring_buffer_yield();
            }
        }

        int step() {

            uint32_t nonLargeAvail =
                ring_buffer_available_read(
                    mContext.to_host, 0);

            uint32_t largeAvail =
                ring_buffer_available_read(
                    mContext.to_host_large_xfer.ring,
                    &mContext.to_host_large_xfer.view);

            ensureReadBuffer(nonLargeAvail);

            int res = 0;
            if (nonLargeAvail) {
                uint32_t transferMode = mContext.ring_config->transfer_mode;

                switch (transferMode) {
                    case 1:
                        type1Read(nonLargeAvail);
                        break;
                    case 2:
                        type2Read(nonLargeAvail);
                        break;
                    case 3:
                        break;
                    default:
                        EXPECT_TRUE(false) << "Failed, invalid transfer mode";
                }


                res = 0;
            } else if (largeAvail) {
                res = type3Read(largeAvail);
            } else {
                res = mCallbacks.onUnavailableRead();
            }

            handleRoundTrip();

            return res;
        }

        void ensureReadBuffer(uint32_t new_xfer) {
            size_t readBufferAvail = mReadBuffer.size() - mReadPos;
            if (readBufferAvail < new_xfer) {
                mReadBuffer.resize(mReadBuffer.size() + 2 * new_xfer);
            }
        }

        void type1Read(uint32_t avail) {
            uint32_t xferTotal = avail / 8;
            for (uint32_t i = 0; i < xferTotal; ++i) {
                struct asg_type1_xfer currentXfer;
                uint8_t* currentXferPtr = (uint8_t*)(&currentXfer);

                EXPECT_EQ(0, ring_buffer_copy_contents(
                    mContext.to_host, 0,
                    sizeof(currentXfer), currentXferPtr));

                char* ptr = mContext.buffer + currentXfer.offset;
                size_t size = currentXfer.size;

                ensureReadBuffer(size);

                memcpy(mReadBuffer.data() + mReadPos,
                       ptr, size);

                for (uint32_t j = 0; j < size; ++j) {
                    EXPECT_EQ((char)ASG_TEST_WRITE_PATTERN,
                              (mReadBuffer.data() + mReadPos)[j]);
                }

                mReadPos += size;
                mContext.ring_config->host_consumed_pos =
                    ptr - mContext.buffer;

                EXPECT_EQ(1, ring_buffer_advance_read(
                    mContext.to_host, sizeof(asg_type1_xfer), 1));
            }
        }

        void type2Read(uint32_t avail) {
            uint32_t xferTotal = avail / 16;
            for (uint32_t i = 0; i < xferTotal; ++i) {
                struct asg_type2_xfer currentXfer;
                uint8_t* xferPtr = (uint8_t*)(&currentXfer);

                EXPECT_EQ(0, ring_buffer_copy_contents(
                    mContext.to_host, 0, sizeof(currentXfer),
                    xferPtr));

                char* ptr = mCallbacks.getPtr(currentXfer.physAddr);
                ensureReadBuffer(currentXfer.size);

                memcpy(mReadBuffer.data() + mReadPos, ptr,
                       currentXfer.size);
                mReadPos += currentXfer.size;

                EXPECT_EQ(1, ring_buffer_advance_read(
                    mContext.to_host, sizeof(currentXfer), 1));
            }
        }

        int type3Read(uint32_t avail) {
            (void)avail;
            ensureReadBuffer(avail);
            ring_buffer_read_fully_with_abort(
                mContext.to_host_large_xfer.ring,
                &mContext.to_host_large_xfer.view,
                mReadBuffer.data() + mReadPos,
                avail,
                1, &mContext.ring_config->in_error);
            mReadPos += avail;
            return 0;
        }

    private:

        void threadFunc() {
            while(-1 != step());
        }

        struct asg_context mContext;
        ConsumerCallbacks mCallbacks;
        FunctorThread mThread;
        std::vector<char> mReadBuffer;
        std::vector<char> mWriteBuffer;
        size_t mReadPos = 0;
        uint32_t mToHostBytes = 0;
        uint32_t mFromHostBytes = 0;
        bool mRoundTripEnabled = false;
    };

protected:
    static void SetUpTestCase() {
        goldfish_address_space_set_vm_operations(getConsoleAgents()->vm);
    }

    static void TearDownTestCase() { }

    void SetUp() override {
        android_hw->hw_gltransport_asg_writeBufferSize = 524288;
        android_hw->hw_gltransport_asg_writeStepSize = 1024;

        mDevice = HostAddressSpaceDevice::get();
        ConsumerInterface interface = {
            // create
            [this](struct asg_context context,
               ConsumerCallbacks callbacks) {
               Consumer* c = new Consumer(context, callbacks);
               mCurrentConsumer = c;
               return (void*)c;
            },
            // destroy
            [this](void* context) {
               Consumer* c = reinterpret_cast<Consumer*>(context);
               delete c;
               mCurrentConsumer = nullptr;
            },
            // save
            [](void* consumer, base::Stream* stream) { },
            // load
            [](void* consumer, base::Stream* stream) { },
        };
        AddressSpaceGraphicsContext::setConsumer(interface);
    }

    void TearDown() override {
        AddressSpaceGraphicsContext::clear();
        mDevice->clear();
        android_hw->hw_gltransport_asg_writeBufferSize = 524288;
        android_hw->hw_gltransport_asg_writeStepSize = 1024;
        EXPECT_EQ(nullptr, mCurrentConsumer);
    }

    void setRoundTrip(bool enabled, size_t writeBytes, size_t readBytes) {
        EXPECT_NE(nullptr, mCurrentConsumer);
        mCurrentConsumer->setRoundTrip(enabled, writeBytes, readBytes);
    }

    struct RoundTrip {
        size_t writeBytes;
        size_t readBytes;
    };

    void runRoundTrips(Client& client, const std::vector<RoundTrip>& trips) {
        EXPECT_NE(nullptr, mCurrentConsumer);

        for (const auto& trip : trips) {
            mCurrentConsumer->setRoundTrip(true, trip.writeBytes, trip.readBytes);

            std::vector<char> send(trip.writeBytes, ASG_TEST_WRITE_PATTERN);
            std::vector<char> expectedRead(trip.readBytes, ASG_TEST_READ_PATTERN);
            std::vector<char> toRead(trip.readBytes, 0);

            size_t stepSize = android_hw->hw_gltransport_asg_writeStepSize;
            size_t stepSizeRead = android_hw->hw_gltransport_asg_writeBufferSize;

            size_t sent = 0;
            while (sent < trip.writeBytes) {
                size_t remaining = trip.writeBytes - sent;
                size_t next = remaining < stepSize ? remaining : stepSize;
                auto buf = client.allocBuffer(next);
                memcpy(buf, send.data() + sent, next);
                sent += next;
            }

            client.flush();

            size_t recv = 0;

            while (recv < trip.readBytes) {
                ssize_t readThisTime = client.speculativeRead(
                    toRead.data() + recv, stepSizeRead);
                EXPECT_GE(readThisTime, 0);
                recv += readThisTime;
            }

            EXPECT_EQ(expectedRead, toRead);

            // make sure the consumer is hung up here or this will
            // race with setRoundTrip
            mCurrentConsumer->ensureWritebackDone();
        }

        mCurrentConsumer->setRoundTrip(false);
    }

    HostAddressSpaceDevice* mDevice = nullptr;
    Consumer* mCurrentConsumer = nullptr;
};

// Tests that we can create a client for ASG,
// which then in turn creates a consumer thread on the "host."
// Then test the thread teardown.
TEST_F(AddressSpaceGraphicsTest, Basic) {
    Client client(mDevice);
}

// Tests writing via an IOStream-like interface
// (allocBuffer, then flush)
TEST_F(AddressSpaceGraphicsTest, BasicWrite) {
    EXPECT_EQ(1024, android_hw->hw_gltransport_asg_writeStepSize);
    Client client(mDevice);

    // Tests that going over the step size results in nullptr
    // when using allocBuffer
    auto buf = client.allocBuffer(1025);
    EXPECT_EQ(nullptr, buf);

    buf = client.allocBuffer(4);
    EXPECT_NE(nullptr, buf);
    memset(buf, ASG_TEST_WRITE_PATTERN, 4);
    client.flush();
}

// Tests that further allocs result in flushing
TEST_F(AddressSpaceGraphicsTest, FlushFromAlloc) {
    EXPECT_EQ(1024, android_hw->hw_gltransport_asg_writeStepSize);
    Client client(mDevice);

    auto buf = client.allocBuffer(1024);
    memset(buf, ASG_TEST_WRITE_PATTERN, 1024);

    for (uint32_t i = 0; i < 10; ++i) {
        buf = client.allocBuffer(1024);
        memset(buf, ASG_TEST_WRITE_PATTERN, 1024);
    }
}

// Tests type 3 (large) transfer by itself
TEST_F(AddressSpaceGraphicsTest, LargeXfer) {
    Client client(mDevice);

    std::vector<char> largeBuf(1048576, ASG_TEST_WRITE_PATTERN);
    client.writeFully(largeBuf.data(), largeBuf.size());
}

// Round trip test
TEST_F(AddressSpaceGraphicsTest, RoundTrip) {
    Client client(mDevice);
    setRoundTrip(true, 1, 1);
    char element = (char)(ASG_TEST_WRITE_PATTERN);
    char reply;
    char expectedReply = (char)(ASG_TEST_READ_PATTERN);

    auto buf = client.allocBuffer(1);
    *buf = element;
    client.flush();
    client.speculativeRead(&reply, 1);
}

// Round trip test (more than one)
TEST_F(AddressSpaceGraphicsTest, RoundTrips) {
    Client client(mDevice);

    std::vector<RoundTrip> trips = {
        { 1, 1, },
        { 2, 2, },
        { 4, 4, },
        { 1026, 34, },
        { 4, 1048576, },
    };

    runRoundTrips(client, trips);
}

// Round trip test (random)
TEST_F(AddressSpaceGraphicsTest, RoundTripsRandom) {
    Client client(mDevice);

    std::default_random_engine generator;
    generator.seed(0);
    std::uniform_int_distribution<int>
        sizeDist(1, 4097);
    std::vector<RoundTrip> trips;
    for (uint32_t i = 0; i < 1000; ++i) {
        trips.push_back({
            (size_t)sizeDist(generator),
            (size_t)sizeDist(generator),
        });
    };

    runRoundTrips(client, trips);
}

// Abort test. Say that we are reading back 4096
// bytes, but only actually read back 1 then abort.
TEST_F(AddressSpaceGraphicsTest, Abort) {
    Client client(mDevice);
    setRoundTrip(true, 1, 1048576);

    char send = ASG_TEST_WRITE_PATTERN;
    char recv = ASG_TEST_READ_PATTERN;
    auto buf = client.allocBuffer(1);
    *buf = send;
    client.flush();
    client.abort();
}

// Test having to create more than one block, and
// ensure traffic works each time.
TEST_F(AddressSpaceGraphicsTest, BlockCreateDestroy) {

    std::vector<Client*> clients;

    std::default_random_engine generator;
    generator.seed(0);
    std::uniform_int_distribution<int>
        sizeDist(1, 47);
    std::vector<RoundTrip> trips;
    for (uint32_t i = 0; i < 100; ++i) {
        trips.push_back({
            (size_t)sizeDist(generator),
            (size_t)sizeDist(generator),
        });
    };

    int numBlocksMax = 3;
    int numBlocksDetected = 0;
    char* bufLow = (char*)(uintptr_t)(-1);
    char* bufHigh = 0;

    while (true) {
        Client* c = new Client(mDevice);
        runRoundTrips(*c, trips);

        clients.push_back(c);

        char* bufPtr = c->getBufferPtr();
        bufLow = bufPtr < bufLow ? bufPtr : bufLow;
        bufHigh = bufPtr > bufHigh ? bufPtr : bufHigh;

        size_t gap = bufHigh - bufLow;

        numBlocksDetected =
            gap / ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE;

        if (numBlocksDetected > numBlocksMax) break;
    }

    for (auto c: clients) {
        delete c;
    }
}

// Test having to create more than one block, and
// ensure traffic works each time, but also randomly
// delete previous allocs to cause fragmentation.
TEST_F(AddressSpaceGraphicsTest, BlockCreateDestroyRandom) {
    std::vector<Client*> clients;

    std::default_random_engine generator;
    generator.seed(0);

    std::uniform_int_distribution<int>
        sizeDist(1, 89);
    std::bernoulli_distribution
        deleteDist(0.2);

    std::vector<RoundTrip> trips;
    for (uint32_t i = 0; i < 100; ++i) {
        trips.push_back({
            (size_t)sizeDist(generator),
            (size_t)sizeDist(generator),
        });
    };

    int numBlocksMax = 3;
    int numBlocksDetected = 0;
    char* bufLow = (char*)(uintptr_t)(-1);
    char* bufHigh = 0;

    while (true) {
        Client* c = new Client(mDevice);
        runRoundTrips(*c, trips);

        clients.push_back(c);

        char* bufPtr = c->getBufferPtr();
        bufLow = bufPtr < bufLow ? bufPtr : bufLow;
        bufHigh = bufPtr > bufHigh ? bufPtr : bufHigh;

        size_t gap = bufHigh - bufLow;

        numBlocksDetected =
            gap / ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE;

        if (numBlocksDetected > numBlocksMax) break;

        if (deleteDist(generator)) {
            delete c;
            clients[clients.size() - 1] = 0;
        }
    }

    for (auto c: clients) {
        delete c;
    }
}

} // namespace asg
} // namespace emulation
} // namespace android
