// Copyright 2017 The Android Open Source Project
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

#include <gtest/gtest.h>

#include "android/crashreport/StructuredInfo.h"

using namespace android::base;
using namespace android::crashreport;
using android::HostHwInfo;

static GpuInfo intelgpu("0x8086", "", "", "", "", "");
static GpuInfo nvidiagpu("0x10de", "", "", "", "", "");

TEST(StructuredInfo, basic) {
    StructuredInfo* siptr = StructuredInfo::get();
    EXPECT_TRUE(siptr);

    StructuredInfo& si = *siptr;
    si.clear();

    GpuInfoList testgpus;
    testgpus.infos.push_back(intelgpu);
    testgpus.infos.push_back(nvidiagpu);

    HostHwInfo::Info testinfo = {
        "KVMKVMKVM",
        0, 1, 64, 0xf,
        "Linux",
        &testgpus,
    };

    si.addHostHwInfo(testinfo);
}

TEST(StructuredInfo, testEncode) {
    StructuredInfo* siptr = StructuredInfo::get();
    EXPECT_TRUE(siptr);

    StructuredInfo& si = *siptr;
    si.clear();

    GpuInfoList testgpus;
    testgpus.infos.push_back(intelgpu);
    testgpus.infos.push_back(nvidiagpu);

    HostHwInfo::Info testinfo = {
        "KVMKVMKVM",
        0, 1, 64, 0xf,
        "Linux",
        &testgpus,
    };

    si.addHostHwInfo(testinfo);

    std::string encoded;
    si.toBinaryString(&encoded);

    EXPECT_TRUE(encoded.size() > 0);

    std::string encoded_copy(encoded);
    si.reloadFromBinaryString(encoded_copy);

    EXPECT_TRUE(encoded.size() == encoded_copy.size());
    EXPECT_TRUE(encoded == encoded_copy);
}
