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

                   if (actual > 0) readSoFar += actual;

                   if (!actual) break;; // EOF
               }
               fprintf(stderr, "%s: done\n", __func__);
           }));

           fakeRenderThread->start();
           return ringStream.get();
    };

    AddressSpaceGraphicsContext::ConsumerDestroyCallback destroyFunc =
        [&fakeRenderThread](void* opaque) { fakeRenderThread->wait(); };

    mContext->setConsumer(createFunc, destroyFunc);

    graphicsPing(AddressSpaceGraphicsContext::Command::CreateConsumer);
    graphicsPing(AddressSpaceGraphicsContext::Command::DestroyConsumer);

    fakeRenderThread->wait();
}

} // namespace android
} // namespace emulation

