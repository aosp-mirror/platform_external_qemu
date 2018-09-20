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
#include "android/emulation/hostpipe/HostGoldfishPipe.h"

#include "android/emulation/AndroidMessagePipe.h"

#include <gtest/gtest.h>

namespace android {

class HostGoldfishPipeTest : public ::testing::Test {
protected:
    void SetUp() override {
        AndroidPipe::Service::resetAll();
        mDevice = HostGoldfishPipeDevice::get();
    }

    void TearDown() override {
        AndroidPipe::Service::resetAll();
        // mDevice is singleton, no need to tear down
    }
    HostGoldfishPipeDevice* mDevice = nullptr;
};

TEST_F(HostGoldfishPipeTest, Basic) { }

TEST_F(HostGoldfishPipeTest, MessagePipe) {
   const char kPayload[] = "Hello World";
   const char kResponse[] = "response";

   bool ran = false;

   auto myDecodeAndExecute =
       [kPayload, kResponse, &ran](
          const std::vector<uint8_t>& input,
          std::vector<uint8_t>* output) {
       EXPECT_STREQ(kPayload, (char*)input.data());
       size_t bytes = strlen(kResponse) + 1;
       output->resize(bytes);
       memset(output->data(), 0x0, bytes);
       memcpy(output->data(), kResponse, strlen(kResponse));
       ran = true;
   };

   registerAndroidMessagePipeService(
       "testMessagePipe", myDecodeAndExecute);

   auto pipe = mDevice->connect("testMessagePipe");

   int32_t payloadSz = strlen(kPayload) + 1;

   mDevice->write(pipe, &payloadSz, sizeof(int32_t));
   mDevice->write(pipe, kPayload, strlen(kPayload) + 1);

   int32_t responseSize = 0;
   mDevice->read(pipe, (void*)&responseSize, sizeof(int32_t));

   EXPECT_TRUE(ran);

   std::vector<uint8_t> response(responseSize + 1, 0);
   mDevice->read(pipe, (void*)response.data(), responseSize);

   EXPECT_STREQ(kResponse, (char*)response.data());

   mDevice->close(pipe);
}

} // namespace android
