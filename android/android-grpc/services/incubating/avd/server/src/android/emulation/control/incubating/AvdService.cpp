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

#include "android/base/files/IniFile.h"
#include "android/console.h"
#include "android/emulation/control/incubating/AvdService.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "android/grpc/utils/EnumTranslate.h"
#include "android/avd/info.h"
#include "android/avd/avd-info.h"
#include "android/utils/debug.h"
#include "avd_service.grpc.pb.h"
#include "avd_service.pb.h"
#include "host-common/FeatureControl.h"
#include "host-common/VmLock.h"
#include "host-common/feature_control.h"
#include "host-common/hw-config.h"

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("AvdService: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

using QemuAvdInfo = ::AvdInfo;

namespace android {
namespace emulation {
namespace control {
namespace incubating {

using google::protobuf::Empty;

static std::string em(const char* str) {
    return str ? std::string(str) : "";
}

static std::string fileDataAsString(FileData* data) {
    return std::string(reinterpret_cast<char*>(data->data), data->size);
}

class AvdImpl final : public AvdService::Service {
public:
    AvdImpl(QemuAvdInfo* info) : mQemuAvd(info){};

    AvdInfo loadInfo() {
        AvdInfo avdInfo;
        avdInfo.set_in_android_build(avdInfo_inAndroidBuild(mQemuAvd));
        avdInfo.set_android_out(em(mQemuAvd->androidOut));
        avdInfo.set_android_build_root(em(mQemuAvd->androidBuildRoot));
        avdInfo.set_target_arch(em(mQemuAvd->targetArch));
        avdInfo.set_target_abi(em(mQemuAvd->targetAbi));
        avdInfo.set_acpi_ini_path(em(mQemuAvd->acpiIniPath));
        avdInfo.set_target(em(mQemuAvd->target));

        avdInfo.set_device_name(em(mQemuAvd->deviceName));
        avdInfo.set_device_id(em(mQemuAvd->deviceId));
        avdInfo.set_sdk_root_path(em(mQemuAvd->sdkRootPath));

        for (int i = 0; i < mQemuAvd->numSearchPaths; i++) {
            if (mQemuAvd->searchPaths[i]) {
                avdInfo.add_search_paths(mQemuAvd->searchPaths[i]);
            }
        }

        avdInfo.set_content_path(em(mQemuAvd->contentPath));
        avdInfo.set_root_ini_path(em(mQemuAvd->rootIniPath));

        android::base::IniFile* cfgini =
                reinterpret_cast<android::base::IniFile*>(mQemuAvd->rootIni);
        if (cfgini) {
            auto& inimap = *avdInfo.mutable_root_ini();
            for (auto it = cfgini->begin(); it != cfgini->end(); ++it) {
                inimap[*it] = cfgini->getString(*it, "unused");
            }
        }
        cfgini = reinterpret_cast<android::base::IniFile*>(mQemuAvd->configIni);

        if (cfgini) {
            auto& inimap = *avdInfo.mutable_config_ini();
            for (auto it = cfgini->begin(); it != cfgini->end(); ++it) {
                inimap[*it] = cfgini->getString(*it, "unused");
            }
        }
        cfgini = reinterpret_cast<android::base::IniFile*>(
                mQemuAvd->skinHardwareIni);
        if (cfgini) {
            auto& inimap = *avdInfo.mutable_skin_hardware_ini();
            for (auto it = cfgini->begin(); it != cfgini->end(); ++it) {
                inimap[*it] = cfgini->getString(*it, "unused");
            }
        }

        avdInfo.set_api_level(mQemuAvd->apiLevel);
        avdInfo.set_incremental_version(mQemuAvd->incrementalVersion);
        avdInfo.set_is_marshmallow_or_higher(mQemuAvd->isMarshmallowOrHigher);
        avdInfo.set_is_google_apis(mQemuAvd->isGoogleApis);
        avdInfo.set_is_user_build(mQemuAvd->isUserBuild);
        avdInfo.set_is_atd(mQemuAvd->isAtd);
        avdInfo.set_flavor(static_cast<AvdFlavor>((int)mQemuAvd->flavor + 1));
        avdInfo.set_skin_name(em(mQemuAvd->skinName));
        avdInfo.set_skin_dir_path(em(mQemuAvd->skinDirPath));
        avdInfo.set_core_hardware_ini_path(em(mQemuAvd->coreHardwareIniPath));
        avdInfo.set_snapshot_lock_path(em(mQemuAvd->snapshotLockPath));
        avdInfo.set_multi_instance_lock_path(
                em(mQemuAvd->multiInstanceLockPath));

        avdInfo.set_build_properties(
                fileDataAsString(mQemuAvd->buildProperties));
        avdInfo.set_boot_properties(fileDataAsString(mQemuAvd->bootProperties));

        for (int i = 0; i < AVD_IMAGE_MAX; i++) {
            if (mQemuAvd->imagePath[i]) {
                avdInfo.add_image_paths(mQemuAvd->imagePath[i]);
                avdInfo.add_image_states(static_cast<ImageState>(
                        (int)mQemuAvd->imageState[i] + 1));
            }
        }

        avdInfo.set_no_checks(mQemuAvd->noChecks);
        avdInfo.set_sysdir(em(mQemuAvd->sysdir));
        return avdInfo;
    }

    ::grpc::Status getAvdInfo(
            ::grpc::ServerContext* /*context*/,
            const ::google::protobuf::Empty* /*request*/,
            ::android::emulation::control::incubating::AvdInfo* response)
            override {
        response->CopyFrom(loadInfo());
        return ::grpc::Status::OK;
    }

private:
    QemuAvdInfo* mQemuAvd;
};

grpc::Service* getAvdService(const AndroidConsoleAgents* agents) {
    return new AvdImpl(getConsoleAgents()->settings->avdInfo());
}

}  // namespace incubating
}  // namespace control
}  // namespace emulation
}  // namespace android
