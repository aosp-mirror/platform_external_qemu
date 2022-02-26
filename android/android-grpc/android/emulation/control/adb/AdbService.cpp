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
#include "android/emulation/control/adb/AdbService.h"

#include <grpcpp/grpcpp.h>  // for Status, ServerCo...
#include <fstream>
#include <sstream>

#include "adb_service.grpc.pb.h"  // for Rtc, Rtc::Stub
#include "adb_service.pb.h"       // for JsepMsg, RtcId
#include "android/base/files/PathUtils.h"
#include "android/emulation/control/adb/adbkey.h"

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google

using ::google::protobuf::Empty;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;
using android::base::PathUtils;

namespace android {
namespace emulation {
namespace control {

class AdbServiceImpl : public Adb::Service {
public:
    Status pullAdbKey(ServerContext* context,
                      const Empty* request,
                      AdbKey* response) override {
        auto privKey = getPrivateAdbKeyPath();
        if (privKey.empty()) {
            return Status(StatusCode::ABORTED, "Private key was not found", "");
        }

        std::ifstream privateKey(PathUtils::asUnicodePath(privKey).c_str());
        std::stringstream keyText;
        keyText << privateKey.rdbuf();

        if (privateKey.bad()) {
            return Status(StatusCode::ABORTED,
                          "Unable to read private key file.", "");
        }

        response->set_private_key(keyText.str());
        return Status::OK;
    }
};

grpc::Service* getAdbService() {
    return new AdbServiceImpl();
}
}  // namespace control
}  // namespace emulation
}  // namespace android
