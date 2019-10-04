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
#include "RingStream.h"

#include "android/base/files/MemStream.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/address_space_device.hpp"
#include "android/emulation/address_space_graphics.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/hostdevices/HostAddressSpace.h"
#include "android/emulation/hostdevices/HostGoldfishPipe.h"

#include <gtest/gtest.h>

#include <vector>

using android::base::FunctorThread;
using android::HostAddressSpaceDevice;
using android::HostGoldfishPipeDevice;
using emugl::RingStream;

static android::base::MemStream* sSnapshotStream = nullptr;

static void sTestSaveSnapshot(android::base::Stream* stream) {
    HostAddressSpaceDevice::get()->saveSnapshot(stream);
    HostGoldfishPipeDevice::get()->saveSnapshot(stream);
}

static void sTestLoadSnapshot(android::base::Stream* stream) {
    HostAddressSpaceDevice::get()->loadSnapshot(stream);
    HostGoldfishPipeDevice::get()->loadSnapshot(stream);
}

static const SnapshotCallbacks* sSnapshotCallbacks = nullptr;

static const QAndroidVmOperations sQAndroidVmOperations = {
    .vmStop = []() -> bool { fprintf(stderr, "libOpenglRender vm ops: vm stop\n"); return true; },
    .vmStart = []() -> bool { fprintf(stderr, "libOpenglRender vm ops: vm start\n"); return true; },
    .vmReset = []() { fprintf(stderr, "libOpenglRender vm ops: vm reset\n"); },
    .vmShutdown = []() { fprintf(stderr, "libOpenglRender vm ops: vm reset\n"); },
    .vmPause = []() -> bool { fprintf(stderr, "libOpenglRender vm ops: vm pause\n"); return true; },
    .vmResume = []() -> bool { fprintf(stderr, "libOpenglRender vm ops: vm resume\n"); return true; },
    .vmIsRunning = []() -> bool { fprintf(stderr, "libOpenglRender vm ops: vm is running\n"); return true; },
    .snapshotList = [](void*, LineConsumerCallback, LineConsumerCallback) -> bool { fprintf(stderr, "libOpenglRender vm ops: snapshot list\n"); return true; },
    .snapshotSave = [](const char* name, void* opaque, LineConsumerCallback) -> bool {
        fprintf(stderr, "libOpenglRender vm ops: snapshot save\n");
        sSnapshotCallbacks->ops[SNAPSHOT_SAVE].onStart(opaque, name);
        if (sSnapshotStream) {
            delete sSnapshotStream;
            sSnapshotStream = nullptr;
        }
        sSnapshotStream = new android::base::MemStream;
        sTestSaveSnapshot(sSnapshotStream);
        sSnapshotCallbacks->ops[SNAPSHOT_SAVE].onEnd(opaque, name, 0);
        return true;
    },
    .snapshotLoad = [](const char* name, void* opaque, LineConsumerCallback) -> bool {
        fprintf(stderr, "libOpenglRender vm ops: snapshot load\n");
        sSnapshotCallbacks->ops[SNAPSHOT_LOAD].onStart(opaque, name);
        sTestLoadSnapshot(sSnapshotStream);
        sSnapshotCallbacks->ops[SNAPSHOT_LOAD].onEnd(opaque, name, 0);
        sSnapshotStream->rewind();
        return true;
    },
    .snapshotDelete = [](const char* name, void* opaque, LineConsumerCallback errConsumer) -> bool {
        fprintf(stderr, "libOpenglRender vm ops: snapshot delete\n");
        return true;
    },
    .snapshotRemap = [](bool shared, void* opaque, LineConsumerCallback errConsumer) -> bool {
        fprintf(stderr, "libOpenglRender vm ops: snapshot remap\n");
        return true;
    },
    .setSnapshotCallbacks = [](void* opaque, const SnapshotCallbacks* callbacks) {
        fprintf(stderr, "libOpenglRender vm ops: set snapshot callbacks\n");
        sSnapshotCallbacks = callbacks;
    },
    .mapUserBackedRam = [](uint64_t gpa, void* hva, uint64_t size) {
        (void)size;
        HostAddressSpaceDevice::get()->setHostAddrByPhysAddr(gpa, hva);
    },
    .unmapUserBackedRam = [](uint64_t gpa, uint64_t size) {
        (void)size;
        HostAddressSpaceDevice::get()->unsetHostAddrByPhysAddr(gpa);
    },
    .getVmConfiguration = [](VmConfiguration* out) {
        fprintf(stderr, "libOpenglRender vm ops: get vm configuration\n");
     },
    .setFailureReason = [](const char* name, int failureReason) {
        fprintf(stderr, "libOpenglRender vm ops: set failure reason\n");
     },
    .setExiting = []() {
        fprintf(stderr, "libOpenglRender vm ops: set exiting\n");
     },
    .allowRealAudio = [](bool allow) {
        fprintf(stderr, "libOpenglRender vm ops: allow real audio\n");
     },
    .physicalMemoryGetAddr = [](uint64_t gpa) {
        fprintf(stderr, "libOpenglRender vm ops: physical memory get addr\n");
        void* res = HostAddressSpaceDevice::get()->getHostAddr(gpa);
        if (!res) return (void*)(uintptr_t)gpa;
        return res;
     },
    .isRealAudioAllowed = [](void) {
        fprintf(stderr, "libOpenglRender vm ops: is real audiop allowed\n");
        return true;
    },
    .setSkipSnapshotSave = [](bool used) {
        fprintf(stderr, "libOpenglRender vm ops: set skip snapshot save\n");
    },
    .isSnapshotSaveSkipped = []() {
        fprintf(stderr, "libOpenglRender vm ops: is snapshot save skipped\n");
        return false;
    },
};

const QAndroidVmOperations* const gQAndroidVmOperations =
        &sQAndroidVmOperations;

#define ECHO_WRITE_PATTERN 0xAA
#define ECHO_READ_PATTERN 0xBB

struct RoundTrip {
    size_t writeSize;
    size_t readSize;
};

namespace android {
namespace emulation {

class AddressSpaceGraphicsRingStreamTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        goldfish_address_space_set_vm_operations(gQAndroidVmOperations);
    }

    static void TearDownTestCase() { }

    AddressSpaceDevicePingInfo createGraphicsContextRequest() {
        AddressSpaceDevicePingInfo req = {};
        req.metadata = static_cast<uint64_t>(
            AddressSpaceDeviceType::Graphics);
        return req;
    }

    AddressSpaceDevicePingInfo graphicsPing(AddressSpaceGraphicsContext::Command cmd) {
        AddressSpaceDevicePingInfo req = {};
        req.metadata = static_cast<uint64_t>(cmd);
        mDevice->ping(mHandle, &req);
        return req;
    }

    void ringWriteFullyWithHangup(
        const void* writeBuffer, size_t writeSize) {
        const size_t halfRingSize = (size_t)(AddressSpaceGraphicsContext::kRingSize >> 1);

        size_t transferChunkSize =
            writeSize > halfRingSize ? halfRingSize : writeSize;
        size_t sent = 0;
        char* writeBufferBytes = (char*)writeBuffer;

        while (sent < writeSize) {
            if (writeSize - sent < transferChunkSize) {
                transferChunkSize = writeSize - sent;
            }

            long sentChunks = ring_buffer_view_write(
                mToHost.ring,
                &mToHost.view,
                writeBufferBytes + sent, transferChunkSize, 1);

            // Ping if unsuccessful send and host has not woken up.
            if ((sent == 0 || sentChunks == 0) &&
                (AddressSpaceGraphicsContext::ConsumerState)(mToHost.ring->state) !=
                    (AddressSpaceGraphicsContext::ConsumerState::CanConsume)) {
                graphicsPing(AddressSpaceGraphicsContext::Command::NotifyAvailable);
                sched_yield();
            }

            sent += sentChunks * transferChunkSize;
        }

        // Expect the host to finish off all this traffic here.
        while (ring_buffer_available_read(mToHost.ring, &mToHost.view)) {
            sched_yield();
            if ((AddressSpaceGraphicsContext::ConsumerState)(mToHost.ring->state) != (AddressSpaceGraphicsContext::ConsumerState::CanConsume)) {
                graphicsPing(AddressSpaceGraphicsContext::Command::NotifyAvailable);
            }
        }
    }

    void ringReadFully(void* readBuffer, size_t readSize) {
        ring_buffer_read_fully(
            mFromHost.ring, &mFromHost.view, readBuffer, (uint32_t)readSize);
    }

    void singleEchoTest(size_t echoSize, size_t echoReadbackSize) {
        std::unique_ptr<RingStream> ringStream;
        std::unique_ptr<FunctorThread> fakeRenderThread;

        std::vector<char> expectedEchoWriteBuf(echoSize, ECHO_WRITE_PATTERN);
        std::vector<char> expectedEchoReadBuf(echoReadbackSize, ECHO_READ_PATTERN);

        AddressSpaceGraphicsContext::ConsumerCreateCallback createFunc =
            [&ringStream, &fakeRenderThread,
             echoSize, echoReadbackSize,
             &expectedEchoWriteBuf](struct ring_buffer_with_view tohost,
               struct ring_buffer_with_view fromhost,
               AddressSpaceGraphicsContext::OnUnavailableReadCallback onUnavailableReadFunc) {

               ringStream.reset(new RingStream(tohost, fromhost, onUnavailableReadFunc, 4096));

               fakeRenderThread.reset(new FunctorThread(
                   [&ringStream, echoSize, echoReadbackSize, &expectedEchoWriteBuf] {
                   const size_t wantedRead = echoSize;
                   const size_t wantedWrite = echoReadbackSize;
                   std::vector<char> toRead(wantedRead, 0);
                   std::vector<char> toWrite(wantedWrite, ECHO_READ_PATTERN);

                   fprintf(stderr, "%s: then read\n", __func__);
                   size_t readSoFar = 0;
                   while (readSoFar < wantedRead) {
                       size_t actual = ringStream->read(
                           toRead.data() + readSoFar, wantedRead - readSoFar); 
                       if (actual > 0) readSoFar += actual;
                       if (!actual) break;; // EOF
                   }

                   EXPECT_EQ(expectedEchoWriteBuf, toRead);

                   fprintf(stderr, "%s: start writing back\n", __func__);
                   auto writebackBuf = ringStream->alloc(wantedWrite);
                   memcpy(writebackBuf, toWrite.data(), wantedWrite);
                   ringStream->flush();
                   fprintf(stderr, "%s: start writing back (done)\n", __func__);

                   // should get EOF right after
                   EXPECT_EQ(0, ringStream->read(toRead.data(), wantedRead));
               }));

               fakeRenderThread->start();
               return ringStream.get();
        };

        AddressSpaceGraphicsContext::ConsumerDestroyCallback destroyFunc =
            [&fakeRenderThread](void* opaque) { fakeRenderThread->wait(); };

        mContext->setConsumer(createFunc, destroyFunc);

        graphicsPing(AddressSpaceGraphicsContext::Command::CreateConsumer);

        std::vector<char> echoBuf(echoSize, ECHO_WRITE_PATTERN);
        std::vector<char> echoReadbackBuf(echoReadbackSize, 0);

        ringWriteFullyWithHangup(echoBuf.data(), echoSize);
        ringReadFully(echoReadbackBuf.data(), echoReadbackSize);

        EXPECT_EQ(expectedEchoReadBuf, echoReadbackBuf);

        graphicsPing(AddressSpaceGraphicsContext::Command::DestroyConsumer);
    }

    void multiRoundTripTest(const std::vector<RoundTrip>& roundTrips) {
        std::unique_ptr<RingStream> ringStream;
        std::unique_ptr<FunctorThread> fakeRenderThread;

        AddressSpaceGraphicsContext::ConsumerCreateCallback createFunc =
            [&ringStream, &fakeRenderThread, &roundTrips](
               struct ring_buffer_with_view tohost,
               struct ring_buffer_with_view fromhost,
               AddressSpaceGraphicsContext::OnUnavailableReadCallback onUnavailableReadFunc) {

               ringStream.reset(new RingStream(tohost, fromhost, onUnavailableReadFunc, 4096));

               fakeRenderThread.reset(new FunctorThread(
                   [&ringStream,&roundTrips] {

                   EXPECT_FALSE(roundTrips.empty());

                   std::vector<char> toRead;
                   std::vector<char> toReadCheck;
                   std::vector<char> toWrite;

                   for (const auto& trip : roundTrips) {
                       const size_t wantedRead = trip.writeSize;
                       const size_t wantedWrite = trip.readSize;
                       toRead.resize(wantedRead, 0);
                       toReadCheck.resize(wantedRead, ECHO_WRITE_PATTERN);
                       toWrite.resize(wantedWrite, ECHO_READ_PATTERN);

                       size_t readSoFar = 0;
                       while (readSoFar < wantedRead) {
                           size_t actual = ringStream->read(
                               toRead.data() + readSoFar, wantedRead - readSoFar); 
                           if (actual > 0) readSoFar += actual;
                           if (!actual) break;; // EOF
                       }

                       EXPECT_EQ(toReadCheck, toRead);

                       auto writebackBuf = ringStream->alloc(wantedWrite);
                       memcpy(writebackBuf, toWrite.data(), wantedWrite);
                       ringStream->flush();
                   }

                   // should get EOF right after
                   EXPECT_EQ(0, ringStream->read(toRead.data(), 512));
               }));

               fakeRenderThread->start();
               return ringStream.get();
        };

        AddressSpaceGraphicsContext::ConsumerDestroyCallback destroyFunc =
            [&fakeRenderThread](void* opaque) { fakeRenderThread->wait(); };

        mContext->setConsumer(createFunc, destroyFunc);

        graphicsPing(AddressSpaceGraphicsContext::Command::CreateConsumer);

        EXPECT_FALSE(roundTrips.empty());

        std::vector<char> echoBuf;
        std::vector<char> echoReadbackBuf;
        std::vector<char> echoReadbackCheckBuf;

        for (const auto& trip : roundTrips) {
            echoBuf.resize(trip.writeSize, ECHO_WRITE_PATTERN);
            echoReadbackBuf.resize(trip.readSize, 0);
            echoReadbackCheckBuf.resize(trip.readSize, ECHO_READ_PATTERN);
            ringWriteFullyWithHangup(echoBuf.data(), trip.writeSize);
            ringReadFully(echoReadbackBuf.data(), trip.readSize);
            EXPECT_EQ(echoReadbackCheckBuf, echoReadbackBuf);
        }

        graphicsPing(AddressSpaceGraphicsContext::Command::DestroyConsumer);
    }

    void SetUp() override {
        mControlOps = get_address_space_device_control_ops();
        mDevice = HostAddressSpaceDevice::get();
        AddressSpaceGraphicsContext::init(mControlOps);
        mHandle = mDevice->open();
        auto req = createGraphicsContextRequest();
        mDevice->ping(mHandle, &req);

        mContext = (AddressSpaceGraphicsContext*)mControlOps->handle_to_context(mHandle);

        EXPECT_NE(nullptr, mContext);

        req = graphicsPing(AddressSpaceGraphicsContext::Command::AllocOrGetOffset);
        mOffset = req.metadata;
        req = graphicsPing(AddressSpaceGraphicsContext::Command::GetSize);
        mSize = req.metadata;

        EXPECT_EQ(0, HostAddressSpaceDevice::get()->claimShared(mHandle, mOffset, mSize));

        void *hostPtr =
            HostAddressSpaceDevice::get()->getHostAddr(
                HostAddressSpaceDevice::get()->offsetToPhysAddr(
                    mOffset));

        EXPECT_NE(nullptr, hostPtr);

        mBuffer = (char*)hostPtr;

        mToHost.ring = reinterpret_cast<ring_buffer*>(mBuffer + AddressSpaceGraphicsContext::kToHostRingInfoOffset);
        mFromHost.ring = reinterpret_cast<ring_buffer*>(mBuffer + AddressSpaceGraphicsContext::kFromHostRingInfoOffset);

        ring_buffer_view_init(
            mToHost.ring,
            &mToHost.view,
            reinterpret_cast<uint8_t*>(mBuffer + AddressSpaceGraphicsContext::kToHostRingBufferOffset),
            AddressSpaceGraphicsContext::kRingSize);

        ring_buffer_view_init(
            mFromHost.ring,
            &mFromHost.view,
            reinterpret_cast<uint8_t*>(mBuffer + AddressSpaceGraphicsContext::kFromHostRingBufferOffset),
            AddressSpaceGraphicsContext::kRingSize);

        mRingVersion =
            (mToHost.ring->host_version > mToHost.ring->guest_version) ?
                mToHost.ring->guest_version : mToHost.ring->host_version;

        req = graphicsPing(AddressSpaceGraphicsContext::Command::GuestInitializedRings);
    }

    void TearDown() override {
        mDevice->close(mHandle);
        AddressSpaceGraphicsContext::clear();
        mDevice->clear();
    }

    struct address_space_device_control_ops* mControlOps = nullptr;
    HostAddressSpaceDevice* mDevice = nullptr;
    uint32_t mHandle = 0;
    AddressSpaceGraphicsContext* mContext = 0;

    uint64_t mOffset;
    uint64_t mSize;
    char* mBuffer;
    struct ring_buffer_with_view mToHost;
    struct ring_buffer_with_view mFromHost;
    uint32_t mRingVersion;
};

// A basic test: Starts up a fake render thread,
// doesn't send any traffic at all,
// and EOF's the renderthread.
TEST_F(AddressSpaceGraphicsRingStreamTest, Basic) {
    std::unique_ptr<RingStream> ringStream;
    std::unique_ptr<FunctorThread> fakeRenderThread;

    AddressSpaceGraphicsContext::ConsumerCreateCallback createFunc =
        [&ringStream, &fakeRenderThread](struct ring_buffer_with_view tohost,
           struct ring_buffer_with_view fromhost,
           AddressSpaceGraphicsContext::OnUnavailableReadCallback onUnavailableReadFunc) {

           ringStream.reset(new RingStream(tohost, fromhost, onUnavailableReadFunc, 4096));

           fakeRenderThread.reset(new FunctorThread(
               [&ringStream] {
               constexpr size_t wantedRead = 512;
               char toRead[wantedRead];
               size_t readSoFar = 0;
               while (readSoFar < wantedRead) {
                   size_t actual = ringStream->read(toRead + readSoFar, wantedRead - readSoFar); 

                   // We don't intend to receive traffic here.
                   EXPECT_EQ(0, actual);

                   if (actual > 0) readSoFar += actual;

                   if (!actual) break;; // EOF
               }
           }));

           fakeRenderThread->start();
           return ringStream.get();
    };

    AddressSpaceGraphicsContext::ConsumerDestroyCallback destroyFunc =
        [&fakeRenderThread](void* opaque) { fakeRenderThread->wait(); };

    mContext->setConsumer(createFunc, destroyFunc);

    graphicsPing(AddressSpaceGraphicsContext::Command::CreateConsumer);
    graphicsPing(AddressSpaceGraphicsContext::Command::DestroyConsumer);
}

// Using RingStream to echo back traffic in a 1:1 ratio, 64 bytes to and from.
TEST_F(AddressSpaceGraphicsRingStreamTest, Echo64) {
    singleEchoTest(64, 64);
}

// Using RingStream to echo back traffic in a 16:1 ratio, 64 bytes to, 4 bytes from.
TEST_F(AddressSpaceGraphicsRingStreamTest, Echo64_4) {
    singleEchoTest(64, 4);
}

// Tests reading back more than writing.
TEST_F(AddressSpaceGraphicsRingStreamTest, EchoReadbackMore) {
    singleEchoTest(4, 46);
}

// Tests exceeding the ring buffer size on write.
TEST_F(AddressSpaceGraphicsRingStreamTest, EchoBigWrite) {
    singleEchoTest(AddressSpaceGraphicsContext::kRingSize * 4, 64);
}

// Tests exceeding the ring buffer size on read.
TEST_F(AddressSpaceGraphicsRingStreamTest, EchoBigRead) {
    singleEchoTest(64, AddressSpaceGraphicsContext::kRingSize * 4);
}

// Tests odd sizes.
TEST_F(AddressSpaceGraphicsRingStreamTest, EchoOddSizes) {
    singleEchoTest(1, 3);
    singleEchoTest(5, 7);
    singleEchoTest(4095, 7);
    singleEchoTest(4097, 7);
    singleEchoTest(1, 4095);
    singleEchoTest(1, 4097);
    singleEchoTest(AddressSpaceGraphicsContext::kRingSize + 33,
                   AddressSpaceGraphicsContext::kRingSize - 31);
}

// Tests two round trips in a row.
TEST_F(AddressSpaceGraphicsRingStreamTest, MultiRoundTrips_2) {
    std::vector<RoundTrip> trips = {
        { 64, 64 },
        { 64, 64 },
    };
    multiRoundTripTest(trips);
}

// Tests 1k round trips.
TEST_F(AddressSpaceGraphicsRingStreamTest, MultiRoundTrips_1k) {
    std::vector<RoundTrip> trips;
    for (uint32_t i = 0; i < 1000; ++i) {
        trips.push_back({ 64, 64 });
    };
    multiRoundTripTest(trips);
}

// Tests 1k round trips with larger writes than reads.
TEST_F(AddressSpaceGraphicsRingStreamTest, MultiRoundTrips_1k_LargerWrites) {
    std::vector<RoundTrip> trips;
    for (uint32_t i = 0; i < 1000; ++i) {
        trips.push_back({ 325, 4 });
    };
    multiRoundTripTest(trips);
}

// Tests 1k round trips with larger reads than writes.
TEST_F(AddressSpaceGraphicsRingStreamTest, MultiRoundTrips_1k_LargerReads) {
    std::vector<RoundTrip> trips;
    for (uint32_t i = 0; i < 1000; ++i) {
        trips.push_back({ 4, 325 });
    };
    multiRoundTripTest(trips);
}

} // namespace android
} // namespace emulation
