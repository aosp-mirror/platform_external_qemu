// Copyright (C) 2018 The Android Open Source Project
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

#include <grpcpp/grpcpp.h>
#include <stdint.h>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aemu/base/TypeTraits.h"
#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/files/PathUtils.h"
#include "android/console.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/HangDetector.h"
#include "android/emulation/control/LineConsumer.h"
#include "android/emulation/control/adb/AdbShellStream.h"
#include "android/emulation/control/snapshot/CallbackStreambuf.h"
#include "android/snapshot/Icebox.h"
#include "android/snapshot/Snapshot.h"
#include "android/snapshot/SnapshotUtils.h"
#include "android/snapshot/Snapshotter.h"
#include "host-common/vm_operations.h"
#include "snapshot.pb.h"
#include "snapshot_service.grpc.pb.h"
#include "snapshot_service.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using namespace android::base;
using android::emulation::LineConsumer;

namespace android {
namespace emulation {
namespace control {

class SnapshotLineConsumer : public LineConsumer {
public:
    SnapshotLineConsumer(SnapshotPackage* status) : mStatus(status) {}

    SnapshotPackage* error() {
        mStatus->set_success(false);
        std::string errMsg;
        for (const auto& line : lines()) {
            errMsg += line + "\n";
        }
        mStatus->set_err("Operation failed due to: " + errMsg);
        return mStatus;
    }

private:
    SnapshotPackage* mStatus;
};

class SnapshotServiceImpl final : public SnapshotService::Service {
public:
    Status PullSnapshot(ServerContext* context,
                        const SnapshotPackage* request,
                        ServerWriter<SnapshotPackage>* writer) override {
        SnapshotPackage result;
        SnapshotLineConsumer errorConsumer(&result);
        FileFormat format;
        switch (request->format()) {
            case SnapshotPackage::TAR:
                format = TAR;
                break;
            case SnapshotPackage::TARGZ:
                format = TARGZ;
                break;
            case SnapshotPackage::DIRECTORY:
                format = DIRECTORY;
                break;
            default:
                result.set_success(false);
                result.set_err("Unknown snapshot output format");
                return Status::OK;
        }
        std::unique_ptr<CallbackStreambufWriter> csb;
        std::unique_ptr<std::ofstream> dstFile;
        std::streambuf* streamBufPtr = nullptr;
        if (request->format() == SnapshotPackage::DIRECTORY) {
            if (request->path() == "") {
                result.set_success(false);
                result.set_err("Output path not provided");
                return Status::OK;
            }
        } else {
            if (request->path() == "") {
                csb.reset(new CallbackStreambufWriter(
                        k256KB, [writer](char* bytes, std::size_t len) {
                            SnapshotPackage msg;
                            msg.set_payload(std::string(bytes, len));
                            msg.set_success(true);
                            return writer->Write(msg);
                        }));
                streamBufPtr = csb.get();
            } else {
                dstFile.reset(new std::ofstream(
                        android::base::PathUtils::asUnicodePath(
                                request->path().data())
                                .data(),
                        std::ios::binary | std::ios::out));
                if (!dstFile->is_open()) {
                    result.set_success(false);
                    result.set_err("Failed to write to " + request->path());
                    writer->Write(result);
                    return Status::OK;
                }
                streamBufPtr = dstFile->rdbuf();
            }
        }

        result.set_success(
                pullSnapshot(request->snapshot_id().c_str(), streamBufPtr,
                             request->path().c_str(), false, format, false,
                             errorConsumer.opaque(), LineConsumer::Callback));
        writer->Write(result);
        return Status::OK;
    }

    Status PushSnapshot(ServerContext* context,
                        ::grpc::ServerReader<SnapshotPackage>* reader,
                        SnapshotPackage* reply) override {
        SnapshotPackage msg;

        reply->set_success(true);
        auto cb = [reader, &msg](char** new_eback, char** new_gptr,
                                 char** new_egptr) {
            bool incoming = reader->Read(&msg);

            // Setup the buffer pointers.
            *new_eback = (char*)msg.payload().data();
            *new_gptr = *new_eback;
            *new_egptr = *new_gptr + msg.payload().size();

            return incoming;
        };

        std::string id;
        // First read desired format
        auto incoming = reader->Read(&msg);
        // First message likely only has snapshot id information and no
        // bytes, but anyone can set the snapshot id at any time.. so...
        if (msg.snapshot_id().size() > 0) {
            id = msg.snapshot_id();
        }
        FileFormat format;
        const char* directory = nullptr;
        switch (msg.format()) {
            case SnapshotPackage::TAR:
                format = TAR;
                break;
            case SnapshotPackage::TARGZ:
                format = TARGZ;
                break;
            case SnapshotPackage::DIRECTORY:
                format = DIRECTORY;
                directory = msg.path().c_str();
                break;
            default:
                reply->set_success(false);
                reply->set_err("Unknown snapshot format");
                return Status::OK;
        }

        std::unique_ptr<CallbackStreambufReader> csr;
        std::unique_ptr<std::istream> stream;
        if (msg.path() == "") {
            csr.reset(new CallbackStreambufReader(cb));
            stream = std::make_unique<std::istream>(csr.get());
        } else if (format != DIRECTORY) {
            stream = std::make_unique<std::ifstream>(
                    msg.path().c_str(),
                    std::ios_base::in | std::ios_base::binary);
        }

        SnapshotLineConsumer errorConsumer(reply);
        pushSnapshot(id.c_str(), stream.get(), directory, format,
                     errorConsumer.opaque(), LineConsumer::Callback);
        return Status::OK;
    }

    Status ListSnapshots(ServerContext* context,
                         const SnapshotFilter* request,
                         SnapshotList* reply) override {
        for (auto snapshot : snapshot::Snapshot::getExistingSnapshots()) {
            auto protobuf = snapshot.getGeneralInfo();

            bool keep = (request->statusfilter() ==
                                 SnapshotFilter::CompatibleOnly &&
                         snapshot.checkValid(false)) ||
                        request->statusfilter() == SnapshotFilter::All;
            if (protobuf && keep) {
                auto details = reply->add_snapshots();
                details->set_snapshot_id(snapshot.name().data());
                details->set_size(snapshot.folderSize());
                if (snapshot.isLoaded()) {
                    // Invariant: SnapshotDetails::Loaded ->
                    // SnapshotDetails::Compatible
                    details->set_status(SnapshotDetails::Loaded);
                } else {
                    // We only need to check for snapshot validity once.
                    // Invariant: SnapshotFilter::CompatbileOnly ->
                    // snapshot.checkValid(false)
                    if (request->statusfilter() ==
                                SnapshotFilter::CompatibleOnly ||
                        snapshot.checkValid(false)) {
                        details->set_status(SnapshotDetails::Compatible);
                    } else {
                        details->set_status(SnapshotDetails::Incompatible);
                    }
                }
                *details->mutable_details() = *protobuf;
            }
        }

        return Status::OK;
    }

    Status LoadSnapshot(ServerContext* context,
                        const SnapshotPackage* request,
                        SnapshotPackage* reply) override {
        reply->set_snapshot_id(request->snapshot_id());

        auto snapshot =
                snapshot::Snapshot::getSnapshotById(request->snapshot_id());

        if (!snapshot) {
            // Nope, the snapshot doesn't exist.
            reply->set_success(false);
            reply->set_err("Could not find " + request->snapshot_id());
            return Status::OK;
        }

        SnapshotLineConsumer slc(reply);
        bool snapshot_success = false;

        // Put an extra pause in hang detector.
        // Snapshotter already calls a hang detector pause. But it is not
        // enough for imported snapshots, because it performs extra steps
        // (rebase snasphot) before the snapshotter pause. So it would
        // require an extra pause here.
        crashreport::CrashReporter::get()->hangDetector().pause(true);
        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [&snapshot_success, &slc, &snapshot]() {
                    snapshot_success = getConsoleAgents()->vm->snapshotLoad(
                            snapshot->name().data(), slc.opaque(),
                            LineConsumer::Callback);
                });
        crashreport::CrashReporter::get()->hangDetector().pause(false);
        if (!snapshot_success) {
            slc.error();
            return Status::OK;
        }

        reply->set_success(true);
        return Status::OK;
    }

    Status SaveSnapshot(ServerContext* context,
                        const SnapshotPackage* request,
                        SnapshotPackage* reply) override {
        reply->set_snapshot_id(request->snapshot_id());

        auto snapshot =
                snapshot::Snapshot::getSnapshotById(request->snapshot_id());
        if (snapshot) {
            // Nope, the snapshot already exists.
            reply->set_success(false);
            reply->set_err("SnapshotPackage with " + request->snapshot_id() +
                           " already exists!");
            return Status::OK;
        }

        SnapshotLineConsumer slc(reply);
        bool snapshot_success = false;
        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [&snapshot_success, &slc, &request]() {
                    snapshot_success = getConsoleAgents()->vm->snapshotSave(
                            request->snapshot_id().c_str(), slc.opaque(),
                            LineConsumer::Callback);
                });
        if (!snapshot_success) {
            slc.error();
            return Status::OK;
        }

        reply->set_success(true);
        return Status::OK;
    }

    Status DeleteSnapshot(ServerContext* context,
                          const SnapshotPackage* request,
                          SnapshotPackage* reply) override {
        reply->set_snapshot_id(request->snapshot_id());
        reply->set_success(true);

        // This is really best effor here. We will not discover errors etc.
        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [&request]() {
                    snapshot::Snapshotter::get().deleteSnapshot(
                            request->snapshot_id().c_str());
                });
        return Status::OK;
    }

    Status TrackProcess(ServerContext* context,
                        const IceboxTarget* request,
                        IceboxTarget* reply) override {
        int pid = request->pid();
        if (!request->package_name().empty()) {
            AdbShellStream getPid("pidof " + request->package_name());
            std::vector<char> sout;
            std::vector<char> serr;
            if (getPid.readAll(sout, serr) == 0 && sout.size() > 0) {
                sscanf(sout.data(), "%d", &pid);
            }
        }

        if (pid == 0) {
            reply->set_err("Pid cannot be found..");
            reply->set_failed(true);
            return Status::OK;
        }

        std::string snapshotName = request->snapshot_id() == ""
                                           ? "icebox-" + std::to_string(pid)
                                           : request->snapshot_id();
        icebox::track_async(pid, snapshotName, request->max_snapshot_number());

        reply->set_pid(pid);
        reply->set_snapshot_id(snapshotName);
        return Status::OK;
    }

    Status UpdateSnapshot(ServerContext* context,
                          const SnapshotUpdateDescription* request,
                          SnapshotDetails* reply) override {
        auto snapshot =
                snapshot::Snapshot::getSnapshotById(request->snapshot_id());
        if (!snapshot.hasValue()) {
            return Status(grpc::StatusCode::NOT_FOUND,
                          "Snapshot with id " + request->snapshot_id() +
                                  " does not exist.");
        }

        emulator_snapshot::Snapshot* info =
                const_cast<emulator_snapshot::Snapshot*>(
                        snapshot->getGeneralInfo());
        if (!info) {
            return Status(grpc::StatusCode::INTERNAL,
                          "Unable to retrieve general info for " +
                                  request->snapshot_id());
        }

        if (!request->has_description() && !request->has_logical_name()) {
            return Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "No description, nor logical_name present for " +
                                  request->snapshot_id());
        }

        if (request->has_description()) {
            info->set_description(request->description().value());
        }

        if (request->has_logical_name()) {
            info->set_logical_name(request->logical_name().value());
        }

        if (!snapshot->writeSnapshotToDisk()) {
            return Status(grpc::StatusCode::INTERNAL,
                          "Unable to retrieve general info for " +
                                  request->snapshot_id());
        }

        return GetSnapshot(context, request->snapshot_id(), reply);
    }

    Status GetScreenshot(ServerContext* context,
                         const SnapshotId* request,
                         SnapshotScreenshotFile* response) override {
        auto snapshot =
                snapshot::Snapshot::getSnapshotById(request->snapshot_id());

        if (!snapshot.hasValue()) {
            return Status(grpc::StatusCode::NOT_FOUND,
                          "Snapshot with id " + request->snapshot_id() +
                                  " does not exist.");
        }
        std::ifstream file(snapshot->screenshot(), std::ios::binary);
        if (!file) {
            return Status(grpc::StatusCode::INTERNAL,
                          "Unable to open screenshot for snapshot: " +
                                  request->snapshot_id());
        }
        auto screenshot = std::string((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
        response->set_format(SnapshotScreenshotFile::FORMAT_PNG);
        response->set_data(std::move(screenshot));
        return Status::OK;
    }

private:
    // Gets an individual snapshot
    Status GetSnapshot(ServerContext* context,
                       const std::string snapshot_id,
                       SnapshotDetails* reply) {
        auto snapshot = snapshot::Snapshot::getSnapshotById(snapshot_id);
        if (!snapshot.hasValue()) {
            return Status(
                    grpc::StatusCode::NOT_FOUND,
                    "Snapshot with id " + snapshot_id + " does not exist.");
        }

        emulator_snapshot::Snapshot* info =
                const_cast<emulator_snapshot::Snapshot*>(
                        snapshot->getGeneralInfo());
        if (!info) {
            return Status(grpc::StatusCode::INTERNAL,
                          "Unable to retrieve general info for " + snapshot_id);
        }
        auto details = reply->mutable_details();
        details->CopyFrom(*info);

        reply->set_snapshot_id(snapshot->name().data());
        reply->set_size(snapshot->folderSize());
        if (snapshot->isLoaded()) {
            // Invariant: SnapshotDetails::Loaded ->
            // SnapshotDetails::Compatible
            reply->set_status(SnapshotDetails::Loaded);
        } else {
            // We only need to check for snapshot validity once.
            // Invariant: SnapshotFilter::CompatbileOnly ->
            // snapshot.checkValid(false)
            if (snapshot->checkValid(false)) {
                reply->set_status(SnapshotDetails::Compatible);
            } else {
                reply->set_status(SnapshotDetails::Incompatible);
            }
        }

        return Status::OK;
    }

    static constexpr uint32_t k256KB = 256 * 1024;
    static constexpr uint32_t k64KB = 64 * 1024;
};  // namespace control

SnapshotService::Service* getSnapshotService() {
    return new SnapshotServiceImpl();
};

}  // namespace control
}  // namespace emulation
}  // namespace android
