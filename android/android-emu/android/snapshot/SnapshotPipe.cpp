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
#include "android/crashreport/crash-handler.h"
#include "android/emulation/AndroidMessagePipe.h"
#include "android/metrics/MetricsLogging.h"
#include "android/multi-instance.h"
#include "android/snapshot/common.h"
#include "android/snapshot/interface.h"
#include "android/snapshot/proto/offworld.pb.h"

#include <assert.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

extern const QAndroidVmOperations* const gQAndroidVmOperations;

namespace {

struct SnapshotCrossSession {
    android::AndroidMessagePipe::DataBuffer sMetaData = {};
    android::base::Lock sLock = {};
    int sForkTotal = 0;
    int sForkId = 0;
};

android::base::LazyInstance<SnapshotCrossSession> sSnapshotCrossSession;

size_t getReadable(std::iostream& stream) {
    size_t beg = stream.tellg();
    stream.seekg(0, stream.end);
    size_t ret = (size_t)stream.tellg() - beg;
    stream.seekg(beg, stream.beg);
    return ret;
}

class SnapshotPipe : public android::AndroidMessagePipe {
public:
    class Service : public android::AndroidPipe::Service {
    public:
        Service() : android::AndroidPipe::Service("SnapshotPipe") {}
        bool canLoad() const override { return true; }

        virtual android::AndroidPipe* create(void* hwPipe, const char* args)
                override {
            // To avoid complicated synchronization issues, only 1 instance
            // of a SnapshotPipe is allowed at a time
            if (sSnapshotCrossSession->sLock.tryLock()) {
                return new SnapshotPipe(hwPipe, this);
            } else {
                return nullptr;
            }
        }

        android::AndroidPipe* load(void* hwPipe,
                                   const char* args,
                                   android::base::Stream* stream) override {
            __attribute__((unused)) bool success =
                    sSnapshotCrossSession->sLock.tryLock();
            assert(success);
            return new SnapshotPipe(hwPipe, this, stream);
        }
    };
    SnapshotPipe(void* hwPipe,
                 Service* service,
                 android::base::Stream* loadStream = nullptr)
        : android::AndroidMessagePipe(hwPipe, service, decodeAndExecute,
        loadStream) {
        ANDROID_IF_DEBUG(assert(sSnapshotCrossSession->sLock.isLocked()));
        mIsLoad = static_cast<bool>(loadStream);
        if (mIsLoad) {
            resetRecvPayload(std::move(sSnapshotCrossSession->sMetaData));
            // sMetaData should be cleared after the move
        }
    }
    ~SnapshotPipe() { sSnapshotCrossSession->sLock.unlock(); }

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
                const std::string& snapshotName
                        = snapshotPb.create_checkpoint_param().snapshot_name();
                gQAndroidVmOperations->vmStop();
                android::base::ThreadLooper::runOnMainLooper([snapshotName]() {
                    androidSnapshot_save(snapshotName.data());
                    gQAndroidVmOperations->vmStart();
                });
                *shouldReply = true;
                guestRecv->Clear();
                break;
            }
            case MS::FunctionCase::kGotoCheckpointParam: {
                const MS::GotoCheckpoint& gotoCheckpointParam
                        = snapshotPb.goto_checkpoint_param();
                const std::string& snapshotName
                        = gotoCheckpointParam.snapshot_name();
                // Note: metadata are raw bytes, not necessary a string. It is
                // just protobuf encodes them into strings. The data should be
                // handled as raw bytes (no extra '\0' at the end). Please try
                // not to use metadata.c_str().
                const std::string& metadata
                        = gotoCheckpointParam.metadata();
                offworld::GuestRecv::ModuleSnapshot::CreateCheckpoint recv;
                recv.set_metadata(metadata);
                encodeGuestRecvFrame(recv, &sSnapshotCrossSession->sMetaData);
                gQAndroidVmOperations->vmStop();
                android::base::FileShare shareMode =
                        android::multiinstance::getInstanceShareMode();

                if (gotoCheckpointParam.has_share_mode() &&
                    gotoCheckpointParam.share_mode() !=
                            MS::GotoCheckpoint::UNKNOWN &&
                    gotoCheckpointParam.share_mode() !=
                            MS::GotoCheckpoint::UNCHANGED) {
                    switch (gotoCheckpointParam.share_mode()) {
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
                printf("snapshot load share mode %d\n", shareMode);
                android::base::ThreadLooper::runOnMainLooper([snapshotName,
                                                              shareMode]() {
                    bool res = android::multiinstance::updateInstanceShareMode(
                            snapshotName.c_str(), shareMode);
                    if (!res) {
                        fprintf(stderr, "WARNING: share mode update failure\n");
                    }
                    androidSnapshot_load(snapshotName.data());
                    gQAndroidVmOperations->vmStart();
                });
                *shouldReply = false;
                break;
            }
            case MS::FunctionCase::kForkReadOnlyInstancesParam: {
                if (android::multiinstance::getInstanceShareMode() ==
                    android::base::FileShare::Write) {
                    sSnapshotCrossSession->sForkTotal =
                            snapshotPb.fork_read_only_instances_param()
                                    .num_instances();
                    sSnapshotCrossSession->sForkId = 0;
                    offworld::GuestRecv::ModuleSnapshot::ForkReadOnlyInstances
                            recv;
                    recv.set_instance_id(sSnapshotCrossSession->sForkId);
                    encodeGuestRecvFrame(recv,
                                         &sSnapshotCrossSession->sMetaData);
                    gQAndroidVmOperations->vmStop();
                    android::base::ThreadLooper::runOnMainLooper([]() {
                        androidSnapshot_save(android::snapshot::kDefaultBootSnapshot);
                        bool res =
                                android::multiinstance::updateInstanceShareMode(
                                        android::snapshot::kDefaultBootSnapshot,
                                        android::base::FileShare::Read);
                        if (!res) {
                            fprintf(stderr,
                                    "WARNING: share mode update failure\n");
                        }
                        androidSnapshot_load(android::snapshot::kDefaultBootSnapshot);
                        gQAndroidVmOperations->vmStart();
                    });
                }
                *shouldReply = false;
                break;
            }
            case MS::FunctionCase::kDoneInstanceParam: {
                if (sSnapshotCrossSession->sForkId <
                    sSnapshotCrossSession->sForkTotal - 1) {
                    sSnapshotCrossSession->sForkId++;
                    offworld::GuestRecv::ModuleSnapshot::ForkReadOnlyInstances
                            recv;
                    recv.set_instance_id(sSnapshotCrossSession->sForkId);
                    encodeGuestRecvFrame(recv,
                                         &sSnapshotCrossSession->sMetaData);
                    gQAndroidVmOperations->vmStop();
                    // Load back to write mode for the last instance
                    android::base::FileShare mode =
                            sSnapshotCrossSession->sForkId <
                                            sSnapshotCrossSession->sForkTotal -
                                                    2
                                    ? android::base::FileShare::Read
                                    : android::base::FileShare::Write;
                    android::base::ThreadLooper::runOnMainLooper([mode]() {
                        bool res =
                                android::multiinstance::updateInstanceShareMode(
                                        android::snapshot::kDefaultBootSnapshot,
                                        mode);
                        if (!res) {
                            fprintf(stderr,
                                    "WARNING: share mode update failure\n");
                        }
                        androidSnapshot_load(android::snapshot::kDefaultBootSnapshot);
                        gQAndroidVmOperations->vmStart();
                    });
                } else if (sSnapshotCrossSession->sForkId ==
                           sSnapshotCrossSession->sForkTotal - 1) {
                    sSnapshotCrossSession->sForkId = 0;
                    sSnapshotCrossSession->sForkTotal = 0;
                }
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
namespace snapshot {
void registerSnapshotPipeService() {
    android::AndroidPipe::Service::add(new SnapshotPipe::Service());
}
}  // namespace snapshot
}  // namespace android
