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
#include "android/emulation/hostdevices/HostAddressSpace.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/address_space_device.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace android {
namespace emulation {
namespace asg {

#define TEST_READ_PATTERN 0xAA
#define TEST_WRITE_PATTERN 0xBB

class AddressSpaceGraphicsTest : public ::testing::Test {
protected:
    class Client {
    public:
    };

    class Consumer {
    public:
        Consumer(struct asg_context _context,
                 ConsumerCallbacks _callbacks) :
            context(_context),
            callbacks(_callbacks) { }

        void setRoundTrip(uint32_t _toHostBytes,
                          uint32_t _fromHostBytes) {
            toHostBytes = _toHostBytes;
            fromHostBytes = _fromHostBytes;
        }

        int step() {
            uint32_t incoming =
                ring_buffer_available_read(
                    context.to_host, 0);

            ensureReadBuffer(incoming);

            if (incoming) {
                uint32_t transferMode = context.ring_config->transfer_mode;

                switch (transferMode) {
                    case 1:
                        type1Read(incoming);
                        break;
                    case 2:
                        type2Read(incoming);
                        break;
                    case 3:
                        type3Read(incoming);
                        break;
                    default:
                        EXPECT_TRUE(false) << "Failed, invalid transfer mode";
                }

                return 0;
            } else {
                return callbacks.onUnavailableRead();
            }
        }

        void ensureReadBuffer(uint32_t new_xfer) {
            size_t readBufferAvail = readBuffer.size() - readPos;
            if (readBufferAvail < new_xfer) {
                readBuffer.resize(readBuffer.size() + 2 * new_xfer);
            }
        }

        void type1GetPtrSize(uint64_t item, char** ptr, size_t* sizeOut) {
            uint32_t offset = (uint32_t)item;
            uint32_t size = (uint32_t)(item >> 32);

            *ptr = context.buffer + offset;
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
                    context.to_host, 0, 8, currentXferPtr));
                type1GetPtrSize(currentXfer, &ptr, &size);

                ensureReadBuffer(size);

                memcpy(readBuffer.data() + readPos,
                       ptr, size);
                readPos += size;

                EXPECT_EQ(1, ring_buffer_read(
                    context.to_host, currentXferPtr, 8, 1));
            }
        }

        void type2Read(uint32_t avail) {
            uint32_t xferTotal = avail / 16;
            for (uint32_t i = 0; i < xferTotal; ++i) {
                uint64_t physAddrAndSize[2];
                uint8_t* addrSizePtr = (uint8_t*)(physAddrAndSize);

                EXPECT_EQ(0, ring_buffer_copy_contents(
                    context.to_host, 0, 16, addrSizePtr));

                char* ptr = callbacks.getPtr(physAddrAndSize[0]);

                ensureReadBuffer(physAddrAndSize[1]);

                memcpy(readBuffer.data() + readPos, ptr,
                       physAddrAndSize[1]);
                readPos += physAddrAndSize[1];

                EXPECT_EQ(1, ring_buffer_read(
                    context.to_host, addrSizePtr, 16, 1));
            }
        }

        void type3Read(uint32_t avail) {
            (void)avail;
            uint32_t xferTotal = context.ring_config->transfer_size;
            ensureReadBuffer(xferTotal);
            ring_buffer_read_fully(
                context.to_host_large_xfer.ring,
                &context.to_host_large_xfer.view,
                readBuffer.data() + readPos,
                xferTotal);
            readPos += xferTotal;
        }

        struct asg_context context;
        ConsumerCallbacks callbacks;
        std::vector<char> readBuffer;
        std::vector<char> writeBuffer;
        size_t readPos = 0;
        uint32_t toHostBytes = 0;
        uint32_t fromHostBytes = 0;
    };


    static void SetUpTestCase() {
        goldfish_address_space_set_vm_operations(gQAndroidVmOperations);
    }

    static void TearDownTestCase() { }

    void SetUp() override {
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
    }

    HostAddressSpaceDevice* mDevice = nullptr;
};

TEST_F(AddressSpaceGraphicsTest, Basic) { }

} // namespace asg
} // namespace emulation
} // namespace android
