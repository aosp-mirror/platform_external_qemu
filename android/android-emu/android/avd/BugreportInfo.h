/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "android/base/Compiler.h"

#include <string>

namespace android {
namespace avd {

class BugreportInfo {
public:
    BugreportInfo();
    std::string dump();
    std::string androidVer;
    std::string androidStduioVer;
    std::string avdDetails;
    std::string buildFingerprint;
    std::string cpuModel;
    std::string deviceName;
    std::string emulatorVer;
    std::string gpuListInfo;
    std::string hypervisorVer;
    std::string hostOsName;
    std::string sdkToolsVer;
    std::string totalMem;

    DISALLOW_COPY_AND_ASSIGN(BugreportInfo);
};

}  // namespace avd
}  // namespace android
