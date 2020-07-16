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
#include <sys/stat.h>                                              // for stat
#include <cstdio>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/base/Stopwatch.h"
#include "android/base/StringView.h"
#include "android/base/Uuid.h"                                     // for Uuid
#include "android/base/files/GzipStreambuf.h"
#include "android/base/files/PathUtils.h"                          // for pj
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/console.h"
#include "android/emulation/control/LineConsumer.h"
#include "android/emulation/control/adb/AdbShellStream.h"
#include "android/emulation/control/snapshot/CallbackStreambuf.h"
#include "android/emulation/control/snapshot/TarStream.h"
#include "android/emulation/control/vm_operations.h"
#include "android/snapshot/Icebox.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshot.h"
#include "android/snapshot/Snapshotter.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"
#include "snapshot.pb.h"
#include "snapshot_service.grpc.pb.h"
#include "snapshot_service.pb.h"

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google

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
        auto snapshot =
                snapshot::Snapshot::getSnapshotById(request->snapshot_id());

        if (!snapshot) {
            // Nope, the snapshot doesn't exist.
            result.set_success(false);
            result.set_err("Could not find " + request->snapshot_id());
            writer->Write(result);
            return Status::OK;
        }

        Stopwatch sw;
        auto tmpdir = pj(System::get()->getTempDir(), snapshot->name());
        const auto tmpdir_deleter =
                base::makeCustomScopedPtr(&tmpdir, [](std::string* tmpdir) {
                    // Best effort to cleanup the mess.
                    path_delete_dir(tmpdir->c_str());
                });
        android_mkdir(tmpdir.data(), 0700);

        // An imported snapshot already has the qcow2 images inside its snapshot
        // directory, so they are already in a good state.
        if (!snapshot->isImported()) {
            // Exports all qcow2 images..
            SnapshotLineConsumer slc(&result);
            auto exp = getConsoleAgents()->vm->snapshotExport(
                    snapshot->name().data(), tmpdir.data(), slc.opaque(),
                    LineConsumer::Callback);

            if (!exp) {
                writer->Write(*slc.error());
                return Status::OK;
            }

            LOG(VERBOSE) << "Exported snapshot in " << sw.restartUs() << " us";
        }

        // Stream the tmpdir out as a tar.gz..
        CallbackStreambufWriter csb(
                k256KB, [writer](char* bytes, std::size_t len) {
                    SnapshotPackage msg;
                    msg.set_payload(std::string(bytes, len));
                    msg.set_success(true);
                    return writer->Write(msg);
                });

        std::unique_ptr<std::ostream> stream;
        if (request->format() == SnapshotPackage::TARGZ) {
            stream = std::make_unique<GzipOutputStream>(&csb);
        } else {
            stream = std::make_unique<std::ostream>(&csb);
        }

        // Use of  a 64 KB  buffer gives good performance (see performance tests.)
        TarWriter tw(tmpdir, *stream, k64KB);
        result.set_success(tw.addDirectory("."));
        if (tw.fail()) {
            result.set_err(tw.error_msg());
        }
        LOG(VERBOSE) << "Completed writing in " << sw.restartUs() << " us";

        // Now add in the metadata.
        auto entries = System::get()->scanDirEntries(snapshot->dataDir(), true);
        for (const auto& fname : entries) {
            if (!System::get()->pathIsFile(fname)) {
                continue;
            }
            struct stat sb;
            android::base::StringView name;
            char buf[k64KB];
            PathUtils::split(fname, nullptr, &name);

            // Use of  a 64 KB  buffer gives good performance (see performance tests.)
            std::ifstream ifs(fname, std::ios_base::in | std::ios_base::binary);
            ifs.rdbuf()->pubsetbuf(buf, sizeof(buf));

            if (android_stat(fname.c_str(), &sb) != 0 ||
                !tw.addFileEntryFromStream(ifs, name, sb)) {
                result.set_err("Unable to tar " + fname);
                break;
            }
        }
        LOG(VERBOSE) << "Wrote metadata in " << sw.restartUs() << " us";

        tw.close();
        if (tw.fail()) {
            result.set_err(tw.error_msg());
        }

        writer->Write(result);
        return Status::OK;
    }

    Status PushSnapshot(ServerContext* context,
                        ::grpc::ServerReader<SnapshotPackage>* reader,
                        SnapshotPackage* reply) override {
        SnapshotPackage msg;

        // Create a temporary directory for the snapshot..
        std::string id = Uuid::generate().toString();
        std::string tmpSnap = snapshot::getSnapshotDir(id.c_str());

        const auto tmpdir_deleter = base::makeCustomScopedPtr(
                &tmpSnap,
                [](std::string* tmpSnap) {  // Best effort to cleanup the mess.
                    path_delete_dir(tmpSnap->c_str());
                });

        reply->set_success(true);
        auto cb = [reader, &msg, &id](char** new_eback, char** new_gptr,
                                      char** new_egptr) {
            bool incoming = reader->Read(&msg);

            // Setup the buffer pointers.
            *new_eback = (char*)msg.payload().data();
            *new_gptr = *new_eback;
            *new_egptr = *new_gptr + msg.payload().size();

            return incoming;
        };

        // First read desired format
        auto incoming = reader->Read(&msg);
        // First message likely only has snapshot id information and no
        // bytes, but anyone can set the snapshot id at any time.. so...
        if (msg.snapshot_id().size() > 0) {
            id = msg.snapshot_id();
        }

        std::unique_ptr<std::istream> stream;
        CallbackStreambufReader csr(cb);
        if (msg.format() == SnapshotPackage::TARGZ) {
            stream = std::make_unique<GzipInputStream>(&csr);
        } else {
            stream = std::make_unique<std::istream>(&csr);
        }

        TarReader tr(tmpSnap, *stream);
        for (auto entry = tr.first(); tr.good(); entry = tr.next(entry)) {
            tr.extract(entry);
        }

        if (tr.fail()) {
            reply->set_success(false);
            reply->set_err(tr.error_msg());
            return Status::OK;
        }

        reply->set_snapshot_id(id);
        std::string finalDest = snapshot::getSnapshotDir(id.c_str());
        if (System::get()->pathExists(finalDest) &&
            path_delete_dir(finalDest.c_str()) != 0) {
            reply->set_success(false);
            reply->set_err("Failed to delete: " + finalDest);
            LOG(INFO) << "Failed to delete: " + finalDest;
            return Status::OK;
        }

        if (std::rename(tmpSnap.c_str(), finalDest.c_str()) != 0) {
            reply->set_success(false);
            reply->set_err("Failed to rename: " + tmpSnap + " --> " +
                           finalDest);
            LOG(INFO) << "Failed to rename: " + tmpSnap + " --> " + finalDest;
        }

        // Okay, now we have to fix up (i.e. import) the snapshot
        auto snapshot = android::snapshot::Snapshot::getSnapshotById(id);
        if (!snapshot) {
            // Que what?, we just created you..
            reply->set_success(false);
            reply->set_err("Cannot find created snapshot");
            // Best effort to cleanup mess.
            path_delete_dir(finalDest.c_str());
            return Status::OK;
        }

        if (!snapshot->fixImport()) {
            reply->set_success(false);
            reply->set_err("Failed to import snapshot.");
            // Best effort to cleanup mess.
            path_delete_dir(finalDest.c_str());
        }

        return Status::OK;
    }

    Status ListSnapshots(ServerContext* context,
                         const ::google::protobuf::Empty* request,
                         SnapshotList* reply) override {
        for (auto snapshot : snapshot::Snapshot::getExistingSnapshots()) {
            auto protobuf = snapshot.getGeneralInfo();

            if (protobuf && snapshot.checkValid(false)) {
                auto details = reply->add_snapshots();
                details->set_snapshot_id(snapshot.name());
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

        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [&snapshot_success, &slc, &snapshot]() {
                    snapshot_success = getConsoleAgents()->vm->snapshotLoad(
                            snapshot->name().data(), slc.opaque(),
                            LineConsumer::Callback);
                });
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
        snapshot::Snapshotter::get().deleteSnapshot(
                request->snapshot_id().c_str());
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
        icebox::track_async(pid, snapshotName);

        reply->set_pid(pid);
        reply->set_snapshot_id(snapshotName);
        return Status::OK;
    }

private:
    static constexpr uint32_t k256KB = 256 * 1024;
    static constexpr uint32_t k64KB = 64 * 1024;
};  // namespace control

SnapshotService::Service* getSnapshotService() {
    return new SnapshotServiceImpl();
};

}  // namespace control
}  // namespace emulation
}  // namespace android
