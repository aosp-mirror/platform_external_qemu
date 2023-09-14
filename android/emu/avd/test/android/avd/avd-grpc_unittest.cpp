// Copyright (C) 2023 The Android Open Source Project
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
#include "aemu/base/files/IniFile.h"
#include "android/avd/avd-info.h"
#include "android/avd/info-grpc.h"
#include "android/avd/info.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/emulation/control/utils/SimpleAvdClient.h"
#include "avd_service_mock.grpc.pb.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
namespace android {
namespace emulation {
namespace control {

using android::emulation::control::incubating::MockAvdServiceStub;
using namespace testing;

using QemuAvdInfo = ::AvdInfo;

class AvdGrpcTest : public Test {
    void SetUp() override {
        mMock = new MockAvdServiceStub();
        mClient = std::make_unique<SimpleAvdClient>(mTest, mMock);
    }

public:
    void SetupStubCall(AvdInfo response) {
        EXPECT_CALL(*mMock, getAvdInfo(_, _, _))
                .Times(AtLeast(1))
                .WillOnce(DoAll(SetArgPointee<2>(response),
                                Return(grpc::Status::OK)));
    }

    MockAvdServiceStub* mMock;
    std::unique_ptr<SimpleAvdClient> mClient;
    std::shared_ptr<EmulatorTestClient> mTest =
            std::make_shared<EmulatorTestClient>();
};

TEST_F(AvdGrpcTest, target_will_be_set) {
    AvdInfo response;
    response.set_device_name("Foo");

    SetupStubCall(response);
    auto info = avdInfo_from_grpc(mClient.get());
    EXPECT_STREQ(avdInfo_getName(info), response.device_name().c_str());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_target_arch) {
    AvdInfo response;
    response.set_target_arch("Foo");

    SetupStubCall(response);
    QemuAvdInfo* info = avdInfo_from_grpc(mClient.get());
    EXPECT_STREQ(info->targetArch, response.target_arch().c_str());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_acpi_ini_path) {
    AvdInfo response;
    response.set_acpi_ini_path("PathToACPI.ini");

    // SetupStubCall function should be modified to take the AvdInfo response.
    SetupStubCall(response);

    QemuAvdInfo* info = avdInfo_from_grpc(mClient.get());
    EXPECT_STREQ(info->acpiIniPath, response.acpi_ini_path().c_str());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_device_name) {
    AvdInfo response;
    response.set_device_name("MyDevice");

    SetupStubCall(response);
    QemuAvdInfo* info = avdInfo_from_grpc(mClient.get());
    EXPECT_STREQ(info->deviceName, response.device_name().c_str());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_api_level) {
    AvdInfo response;
    response.set_api_level(30);
    SetupStubCall(response);

    QemuAvdInfo* info = avdInfo_from_grpc(mClient.get());
    EXPECT_EQ(info->apiLevel, response.api_level());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_boot_properties) {
    AvdInfo response;
    std::string boot("a=b\nc=d");
    response.set_boot_properties(boot);
    SetupStubCall(response);

    QemuAvdInfo* info = avdInfo_from_grpc(mClient.get());
    EXPECT_EQ(info->bootProperties->size, boot.size());

    auto props = std::string((char*)info->bootProperties->data,
                             info->bootProperties->size);
    EXPECT_EQ(props, boot);
    avdInfo_free(info);
}

}  // namespace control
}  // namespace emulation
}  // namespace android