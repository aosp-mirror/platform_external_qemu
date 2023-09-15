// Copyright (C) 2022 The Android Open Source Project
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

#include "android/avd/info.h"
#include "android/console.h"
#include "android/emulation/control/incubating/AvdService.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "android/grpc/utils/EnumTranslate.h"
#include "android/utils/debug.h"
#include "avd_service.grpc.pb.h"
#include "avd_service.pb.h"
#include "host-common/FeatureControl.h"
#include "host-common/VmLock.h"
#include "host-common/feature_control.h"
#include "host-common/hw-config.h"

#ifdef DISABLE_ASYNC_GRPC
#include "android/grpc/utils/SyncToAsyncAdapter.h"
#else
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#endif

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("ModemService: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

using QemuAvdInfo = ::AvdInfo;

namespace android {
namespace emulation {
namespace control {
namespace incubating {

using google::protobuf::Empty;

class AvdImpl final : public AvdService::Service {
public:
    AvdImpl(QemuAvdInfo* avd) {
        char full_name[256];
        const char* tmp;
        static_assert(std::size(full_name) == 256);

        mAvd.set_name((tmp = avdInfo_getName(avd)) ? tmp : "");
        mAvd.set_id((tmp = avdInfo_getId(avd)) ? tmp : "");
        mAvd.set_api_level(avdInfo_getApiLevel(avd));
        mAvd.set_target((tmp = avdInfo_getTarget(avd)) ? tmp : "");
        mAvd.set_desert((tmp = avdInfo_getApiDessertName(mAvd.api_level())) ? tmp : "");

        avdInfo_getFullApiName(mAvd.api_level(), full_name,
                               std::size(full_name));
        mAvd.set_full_api_name(full_name);
        mAvd.set_google_apis(avdInfo_isGoogleApis(avd));
        mAvd.set_user_build(avdInfo_isUserBuild(avd));
        mAvd.set_atd(avdInfo_isAtd(avd));
        mAvd.set_flavor(static_cast<AvdInfo_AVD_FLAVOR>(
                (int)avdInfo_getAvdFlavor(avd) + 1));
        mAvd.set_max_display_entries(avdInfo_maxMultiDisplayEntries());
        mAvd.set_target_cpu_arch((tmp = avdInfo_getTargetCpuArch(avd)) ? tmp : "");
    }

    ::grpc::Status getAvdInfo(
            ::grpc::ServerContext* /*context*/,
            const ::google::protobuf::Empty* /*request*/,
            ::android::emulation::control::incubating::AvdInfo* response)
            override {
        response->CopyFrom(mAvd);
        return ::grpc::Status::OK;
    }

private:
    AvdInfo mAvd;
};

grpc::Service* getAvdService(const AndroidConsoleAgents* agents) {
    return new AvdImpl(getConsoleAgents()->settings->avdInfo());
}

}  // namespace incubating
}  // namespace control
}  // namespace emulation
}  // namespace android
