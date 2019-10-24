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
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "android/base/Log.h"
#include "android/base/StringView.h"
#include "android/base/Uuid.h"
#include "android/emulation/control/interceptor/LoggingInterceptor.h"
#include "android/emulation/control/snapshot/CallbackStreambuf.h"
#include "android/emulation/control/snapshot/GzipStreambuf.h"
#include "android/emulation/control/snapshot/TarStream.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshot.h"
#include "android/utils/path.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "snapshot-service.grpc.pb.h"
#include "snapshot-service.pb.h"
#include "snapshot.pb.h"

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
using namespace android::control::interceptor;

namespace android {
namespace emulation {
namespace control {

class SnapshotServiceImpl final : public SnapshotService::Service {
public:

    Status getSnapshot(ServerContext* context,
                       const Snapshot* request,
                       ServerWriter<Snapshot>* writer) override {
        Snapshot result;
        result.set_success(true);

        for (auto snapshot :
             android::snapshot::Snapshot::getExistingSnapshots()) {
            std::string datadir = snapshot.dataDir();

            if (snapshot.name() == request->snapshot_id()) {
                CallbackStreambufWriter csb(
                        k128KB, [writer](char* bytes, std::size_t len) {
                            Snapshot msg;
                            msg.set_payload(std::string(bytes, len));
                            return writer->Write(msg);
                        });
                GzipOutputStream gzout(&csb);
                TarWriter tw(snapshot.dataDir(), gzout);
                result.set_success(tw.addDirectory(".") && tw.close());
            }
        }
        writer->Write(result);
        return Status::OK;
    }

    Status putSnapshot(ServerContext* context,
                       ::grpc::ServerReader<Snapshot>* reader,
                       Snapshot* reply) override {
        Snapshot msg;
        std::string id = Uuid::generate().toString();
        std::string tmpSnap = android::snapshot::getSnapshotDir(id.c_str());

        reply->set_success(true);
        auto cb = [reader, &msg, &id](char** new_eback, char** new_gptr,
                                      char** new_egptr) {
            // Drop messages without bytes.
            bool incoming = reader->Read(&msg);

            // First message likely only has snapshot id information and no
            // bytes.
            if (msg.snapshot_id().size() > 0) {
                std::cout << msg.snapshot_id() << std::endl;
                id = msg.snapshot_id();
            }
            while (msg.payload().size() == 0 && incoming) {
                incoming = reader->Read(&msg);
            }
            if (incoming) {
                *new_eback = (char*)msg.payload().data();
                *new_gptr = *new_eback;
                *new_egptr = *new_gptr + msg.payload().size();
            }
            return incoming;
        };

        CallbackStreambufReader csr(cb);
        GzipInputStream gzin(&csr);
        TarReader tr(tmpSnap, gzin);
        auto entry = tr.first();
        while (entry.valid) {
            if (!tr.extract(entry)) {
                reply->set_success(false);
                reply->set_err("Failed to extract: " + entry.name);
            }
            entry = tr.next(entry);
        }
        reply->set_snapshot_id(id);
        std::string finalDest = android::snapshot::getSnapshotDir(id.c_str());
        LOG(INFO) << "Moving " << tmpSnap << " --> " << finalDest;
        path_delete_dir(finalDest.c_str());
        std::rename(tmpSnap.c_str(), finalDest.c_str());
        return Status::OK;
    }

    Status listSnapshots(ServerContext* context,
                         const ::google::protobuf::Empty* request,
                         SnapshotList* reply) override {
        for (auto snapshot :
             android::snapshot::Snapshot::getExistingSnapshots()) {
            auto protobuf = snapshot.getGeneralInfo();

            if (protobuf && snapshot.checkValid(false)) {
                auto details = reply->add_snapshots();
                details->set_snapshot_id(snapshot.name());
                *details->mutable_details() = *protobuf;
            }
        }

        return Status::OK;
    }


    static constexpr uint32_t k128KB = 128 * 1024;
};


SnapshotService::Service* getSnapshotService() {
    return new SnapshotServiceImpl();
};

}  // namespace control
}  // namespace emulation
}  // namespace android
