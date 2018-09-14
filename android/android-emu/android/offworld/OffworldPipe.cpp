// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidMessagePipe.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/metrics/MetricsLogging.h"
#include "android/offworld/proto/offworld.pb.h"
#include "android/snapshot/SnapshotAPI.h"
#include "android/snapshot/common.h"
#include "android/snapshot/interface.h"

#include <assert.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

using android::base::LazyInstance;
using android::base::Lock;
using android::base::Optional;

extern const QAndroidVmOperations* const gQAndroidVmOperations;

namespace {

struct OffworldCrossSession {
    Lock sLock = {};
};

LazyInstance<OffworldCrossSession> sOffworldCrossSession;

class OffworldPipe : public android::AndroidMessagePipe {
public:
    class Service : public android::AndroidPipe::Service {
    public:
        Service() : android::AndroidPipe::Service("OffworldPipe") {}
        bool canLoad() const override { return true; }

        virtual android::AndroidPipe* create(void* hwPipe, const char* args)
                override {
            // To avoid complicated synchronization issues, only 1 instance
            // of a OffworldPipe is allowed at a time
            if (sOffworldCrossSession->sLock.tryLock()) {
                return new OffworldPipe(hwPipe, this);
            } else {
                return nullptr;
            }
        }

        android::AndroidPipe* load(void* hwPipe,
                                   const char* args,
                                   android::base::Stream* stream) override {
            __attribute__((unused)) bool success =
                    sOffworldCrossSession->sLock.tryLock();
            assert(success);
            return new OffworldPipe(hwPipe, this, stream);
        }
    };
    OffworldPipe(void* hwPipe,
                 Service* service,
                 android::base::Stream* loadStream = nullptr)
        : android::AndroidMessagePipe(hwPipe, service, decodeAndExecute,
        loadStream) {
        ANDROID_IF_DEBUG(assert(sOffworldCrossSession->sLock.isLocked()));
        mIsLoad = static_cast<bool>(loadStream);
        if (mIsLoad) {
            resetRecvPayload(android::snapshot::getLoadMetadata());
        }
    }
    ~OffworldPipe() { sOffworldCrossSession->sLock.unlock(); }

private:
    static void encodeGuestRecvFrame(const google::protobuf::Message& message,
            std::vector<uint8_t>* output) {
        uint32_t recvSize = message.ByteSize();
        output->resize(recvSize + sizeof(recvSize));
        memcpy(output->data(), (void*)&recvSize, sizeof(recvSize));
        message.SerializeToArray(output->data() + sizeof(recvSize),
                recvSize);
    }
    static void decodeAndExecute(const std::vector<uint8_t>& input,
            std::vector<uint8_t>* output) {
        offworld::GuestSend guestSendPb;
        offworld::GuestRecv guestRecvPb;
        bool shouldReply = false;
        if (!guestSendPb.ParseFromArray(input.data(), input.size())) {
            E("Offworld lib message parsing failed.");
        } else {
            switch (guestSendPb.module_case()) {
                case offworld::GuestSend::ModuleCase::kSnapshot:
                    handleSnapshotPb(guestSendPb, &shouldReply, &guestRecvPb);
                    break;
                case offworld::GuestSend::ModuleCase::kArTesting:
                    // TODO
                    break;
                default:
                    E("Offworld lib received unrecognized message!\n");
            }
        }
        if (shouldReply) {
            output->resize(guestRecvPb.ByteSize());
            guestRecvPb.SerializeToArray(output, output->size());
        } else {
            output->clear();
        }
    }
    static void handleSnapshotPb(const offworld::GuestSend& guestSend,
                                 bool* shouldReply,
                                 offworld::GuestRecv* guestRecv) {
        const offworld::GuestSend::ModuleSnapshot& snapshotPb
                = guestSend.snapshot();
        using MS = offworld::GuestSend::ModuleSnapshot;
        switch (snapshotPb.function_case()) {
            case MS::FunctionCase::kCreateCheckpointParam: {
                android::snapshot::createCheckpoint(
                        snapshotPb.create_checkpoint_param().snapshot_name());
                *shouldReply = true;
                guestRecv->Clear();
                break;
            }
            case MS::FunctionCase::kGotoCheckpointParam: {
                const MS::GotoCheckpoint& gotoCheckpointParam
                        = snapshotPb.goto_checkpoint_param();

                Optional<android::base::FileShare> shareMode;

                if (gotoCheckpointParam.has_share_mode()) {
                    switch (gotoCheckpointParam.share_mode()) {
                        case MS::GotoCheckpoint::UNKNOWN:
                        case MS::GotoCheckpoint::UNCHANGED:
                            break;
                        case MS::GotoCheckpoint::READ_ONLY:
                            shareMode = android::base::FileShare::Read;
                            break;
                        case MS::GotoCheckpoint::WRITABLE:
                            shareMode = android::base::FileShare::Write;
                            break;
                        default:
                            fprintf(stderr,
                                    "WARNING: unsupported share mode, "
                                    "default to unchanged.\n");
                            break;
                    }
                }

                android::snapshot::gotoCheckpoint(
                        gotoCheckpointParam.snapshot_name(),
                        gotoCheckpointParam.metadata(), shareMode);
                *shouldReply = false;
                break;
            }
            case MS::FunctionCase::kForkReadOnlyInstancesParam: {
                android::snapshot::forkReadOnlyInstances(
                        snapshotPb.fork_read_only_instances_param()
                                .num_instances());
                *shouldReply = false;
                break;
            }
            case MS::FunctionCase::kDoneInstanceParam: {
                android::snapshot::doneInstance();
                *shouldReply = false;
                break;
            }
            default:
                fprintf(stderr, "Error: offworld lib received unrecognized "
                        "message!");
                *shouldReply = false;
        }
    }
    enum class OP : int32_t { Save = 0, Load = 1 };
    bool mIsLoad;
};


}  // namespace

namespace android {
namespace offworld {

void registerOffworldPipeService() {
    if (android::featurecontrol::isEnabled(android::featurecontrol::Offworld)) {
        android::AndroidPipe::Service::add(new OffworldPipe::Service());
    }
}

}  // namespace offworld
}  // namespace android
