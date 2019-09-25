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

#include "android/emulation/address_space_graphics.h"

#include "android/base/files/Stream.h"
#include "android/base/files/MemStream.h"
#include "android/base/ring_buffer.h"
#include "android/emulation/AddressSpaceService.h"
#include "android/emulation/address_space_device.hpp"
#include "android/emulation/address_space_graphics_types.h"
#include "android/emulation/hostdevices/HostAddressSpace.h"
#include "android/emulation/control/vm_operations.h"
#include "android/globals.h"

#include <gtest/gtest.h>

#include <vector>

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
                    mDevice->offsetToPhysAddr(mRingOffset));

            mContext = asg_context_create(mRingStorage, mBuffer, mBufferSize);

            auto setVersionResult = ping((uint64_t)ASG_SET_VERSION, mVersion);
            uint32_t hostVersion = setVersionResult.size;
            EXPECT_LE(hostVersion, mVersion);

            mContext.ring_config->host_consumed_pos = 0;
            mContext.ring_config->guest_write_pos = 0;
            mBufferMask = (mBufferSize / mContext.ring_config->flush_interval) - 1;
        }

        int step(uint32_t writeSize, uint32_t readSize) {
            return 0;
        }

        char* allocBuffer(size_t size) {
            if (size > mBufferSize) {
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
            ensureConsumerFinished();
            return 0;
        }

        void flush() {
            if (!mCurrentWriteBytes) return;

            type1WriteWithNotif(mWriteStart - mBuffer, mCurrentWriteBytes);
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

        int type1WriteWithNotif(uint32_t bufferOffset, size_t size) {
            size_t sent = 0;
            size_t sizeForRing = 8;

            uint64_t sizePart = ((uint64_t)(sizeForRing) << 32);
            uint64_t offPart = ((uint64_t)(bufferOffset));
            uint64_t data = sizePart | offPart;

            uint8_t* writeBufferBytes = (uint8_t*)(&data);

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
            }

            return 0;
        }

        void ensureConsumerFinishing() {
            uint32_t currAvailRead =
                ring_buffer_available_read(mContext.to_host, 0);

            while (currAvailRead) {
                sched_yield();
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

        void ensureConsumerFinished() {
            ensureConsumerFinishing();

            uint32_t currAvailRead =
                ring_buffer_available_read(mContext.to_host, 0);

            while (currAvailRead) {
                sched_yield();
                currAvailRead = ring_buffer_available_read(mContext.to_host, 0);
            }
        }


        ~Client() {
            mDevice->unclaimShared(mHandle, mBufferOffset);
            mDevice->unclaimShared(mHandle, mRingOffset);
            mDevice->close(mHandle);
        }

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
            mCallbacks(callbacks) { }

        void setRoundTrip(uint32_t toHostBytes,
                          uint32_t fromHostBytes) {
            mToHostBytes = toHostBytes;
            mFromHostBytes = fromHostBytes;
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

                return 0;
            } else {
                return mCallbacks.onUnavailableRead();
            }
        }

        void ensureReadBuffer(uint32_t new_xfer) {
            size_t readBufferAvail = mReadBuffer.size() - mReadPos;
            if (readBufferAvail < new_xfer) {
                mReadBuffer.resize(mReadBuffer.size() + 2 * new_xfer);
            }
        }

        void type1GetPtrSize(uint64_t item, char** ptr, size_t* sizeOut) {
            uint32_t offset = (uint32_t)item;
            uint32_t size = (uint32_t)(item >> 32);

            *ptr = mContext.buffer + offset;
            *sizeOut = (size_t)size;
        }

        void type1Read(uint32_t avail) {
            uint32_t xferTotal = avail / 8;
            for (uint32_t i = 0; i < xferTotal; ++i) {
                uint64_t currentXfer;
                uint8_t* currentXferPtr = (uint8_t*)currentXfer;

                char* ptr;
                size_t size;
                EXPECT_EQ(0, ring_buffer_copy_contents(
                    mContext.to_host, 0, 8, currentXferPtr));
                type1GetPtrSize(currentXfer, &ptr, &size);

                ensureReadBuffer(size);

                memcpy(mReadBuffer.data() + mReadPos,
                       ptr, size);
                mReadPos += size;
                mContext.ring_config->host_consumed_pos =
                    ptr - mContext.buffer;

                EXPECT_EQ(1, ring_buffer_read(
                    mContext.to_host, currentXferPtr, 8, 1));
            }
        }

        void type2Read(uint32_t avail) {
            uint32_t xferTotal = avail / 16;
            for (uint32_t i = 0; i < xferTotal; ++i) {
                uint64_t physAddrAndSize[2];
                uint8_t* addrSizePtr = (uint8_t*)(physAddrAndSize);

                EXPECT_EQ(0, ring_buffer_copy_contents(
                    mContext.to_host, 0, 16, addrSizePtr));

                char* ptr = mCallbacks.getPtr(physAddrAndSize[0]);

                ensureReadBuffer(physAddrAndSize[1]);

                memcpy(mReadBuffer.data() + mReadPos, ptr,
                       physAddrAndSize[1]);
                mReadPos += physAddrAndSize[1];

                EXPECT_EQ(1, ring_buffer_read(
                    mContext.to_host, addrSizePtr, 16, 1));
            }
        }

        void type3Read(uint32_t avail) {
            (void)avail;
            uint32_t xferTotal = mContext.ring_config->transfer_size;
            ensureReadBuffer(xferTotal);
            ring_buffer_read_fully(
                mContext.to_host_large_xfer.ring,
                &mContext.to_host_large_xfer.view,
                mReadBuffer.data() + mReadPos,
                xferTotal);
            mReadPos += xferTotal;
        }

    private:
        struct asg_context mContext;
        ConsumerCallbacks mCallbacks;
        std::vector<char> mReadBuffer;
        std::vector<char> mWriteBuffer;
        size_t mReadPos = 0;
        uint32_t mToHostBytes = 0;
        uint32_t mFromHostBytes = 0;
    };

protected:
    static void SetUpTestCase() {
        goldfish_address_space_set_vm_operations(gQAndroidVmOperations);
    }

    static void TearDownTestCase() { }

    void SetUp() override {
        android_hw->hw_gltransport_asg_writeBufferSize = 16384;
        android_hw->hw_gltransport_asg_writeStepSize = 1024;

        mDevice = HostAddressSpaceDevice::get();
        ConsumerInterface interface = {
            // create
            [](struct asg_context context,
               ConsumerCallbacks callbacks) {
               Consumer* c = new Consumer(context, callbacks);
               return (void*)c;
            },
            // destroy
            [](void* context) {
               Consumer* c = reinterpret_cast<Consumer*>(context);
               delete c;
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
        android_hw->hw_gltransport_asg_writeBufferSize = 16384;
        android_hw->hw_gltransport_asg_writeStepSize = 1024;
    }

    HostAddressSpaceDevice* mDevice = nullptr;
};

TEST_F(AddressSpaceGraphicsTest, Basic) {
    Client client(mDevice);
}

} // namespace asg
} // namespace emulation
} // namespace android
