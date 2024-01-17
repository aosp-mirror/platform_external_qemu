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
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "android/avd/avd-info.h"
#include "android/avd/info-grpc.h"
#include "android/avd/info.h"
#include "android/base/files/IniFile.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/emulation/control/utils/AvdClient.h"
#include "avd_service_mock.grpc.pb.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
namespace android {
namespace emulation {
namespace control {

using android::emulation::control::incubating::MockAvdServiceStub;
using namespace testing;

class MyAvdServiceStub : public AvdService::StubInterface {
public:
    ::grpc::Status getAvdInfo(
            ::grpc::ClientContext* context,
            const ::google::protobuf::Empty& request,
            ::android::emulation::control::incubating::AvdInfo* response)
            override {
        response->CopyFrom(mAvdInfo);
        return ::grpc::Status::OK;
    }

    ::grpc::ClientAsyncResponseReaderInterface<
            ::android::emulation::control::incubating::AvdInfo>*
    AsyncgetAvdInfoRaw(::grpc::ClientContext* context,
                       const ::google::protobuf::Empty& request,
                       ::grpc::CompletionQueue* cq) override {
        return nullptr;
    }
    ::grpc::ClientAsyncResponseReaderInterface<
            ::android::emulation::control::incubating::AvdInfo>*
    PrepareAsyncgetAvdInfoRaw(::grpc::ClientContext* context,
                              const ::google::protobuf::Empty& request,
                              ::grpc::CompletionQueue* cq) override {
        return nullptr;
    }

    ::android::emulation::control::incubating::AvdInfo mAvdInfo;
};

class AvdGrpcTest : public Test {
    void SetUp() override {
        mMock = new MyAvdServiceStub();
        mClient = std::make_unique<AvdClient>(mTest, mMock);
    }

public:
    void SetupStubCall(AvdInfo response) { mMock->mAvdInfo.CopyFrom(response); }

    MyAvdServiceStub* mMock;
    std::unique_ptr<AvdClient> mClient;
    std::shared_ptr<EmulatorTestClient> mTest =
            std::make_shared<EmulatorTestClient>();
};

TEST_F(AvdGrpcTest, target_will_be_set) {
    AvdInfo response;
    response.set_device_name("Foo");

    SetupStubCall(response);
    auto info = avdInfo_from_grpc(mClient.get());
    EXPECT_TRUE(info != nullptr);
    EXPECT_STREQ(avdInfo_getName(info), response.device_name().c_str());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_target_arch) {
    AvdInfo response;
    response.set_target_arch("Foo");

    SetupStubCall(response);
    auto info = avdInfo_from_grpc(mClient.get());
    EXPECT_TRUE(info != nullptr);
    EXPECT_STREQ(info->targetArch, response.target_arch().c_str());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_acpi_ini_path) {
    AvdInfo response;
    response.set_acpi_ini_path("PathToACPI.ini");

    // SetupStubCall function should be modified to take the AvdInfo response.
    SetupStubCall(response);

    auto info = avdInfo_from_grpc(mClient.get());
    EXPECT_TRUE(info != nullptr);
    EXPECT_STREQ(info->acpiIniPath, response.acpi_ini_path().c_str());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_device_name) {
    AvdInfo response;
    response.set_device_name("MyDevice");

    SetupStubCall(response);
    auto info = avdInfo_from_grpc(mClient.get());
    EXPECT_TRUE(info != nullptr);
    EXPECT_STREQ(info->deviceName, response.device_name().c_str());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_api_level) {
    AvdInfo response;
    response.set_api_level(30);
    SetupStubCall(response);

    auto info = avdInfo_from_grpc(mClient.get());
    EXPECT_TRUE(info != nullptr);
    EXPECT_EQ(info->apiLevel, response.api_level());
    avdInfo_free(info);
}

TEST_F(AvdGrpcTest, sets_boot_properties) {
    AvdInfo response;
    std::string boot("a=b\nc=d");
    response.set_boot_properties(boot);
    SetupStubCall(response);

    auto info = avdInfo_from_grpc(mClient.get());
    EXPECT_TRUE(info != nullptr);
    EXPECT_EQ(info->bootProperties->size, boot.size());

    auto props = std::string((char*)info->bootProperties->data,
                             info->bootProperties->size);
    EXPECT_EQ(props, boot);
    avdInfo_free(info);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
