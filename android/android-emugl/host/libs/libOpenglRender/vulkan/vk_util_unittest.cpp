// Copyright 2022 The Android Open Source Project
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

#include "vk_util.h"

#include <tuple>

namespace vk_util {
namespace vk_fn_info {

// Register a fake Vulkan function for testing.
using PFN_vkGfxstreamTestFunc = PFN_vkCreateDevice;
REGISTER_VK_FN_INFO(GfxstreamTestFunc, ("vkGfxstreamTestFunc", "vkGfxstreamTestFuncGOOGLE",
                                        "vkGfxstreamTestFuncGFXSTREAM"))
constexpr auto vkGfxstreamTestFuncNames = vk_fn_info::GetVkFnInfo<GfxstreamTestFunc>::names;

namespace {
using ::testing::_;
using ::testing::MockFunction;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrNe;

TEST(getVkInstanceProcAddrWithFallbackTest, ShouldReturnNullOnFailure) {
    VkInstance instance = reinterpret_cast<VkInstance>(0x1234'0000);
    MockFunction<std::remove_pointer_t<PFN_vkGetInstanceProcAddr>> vkGetInstanceProcAddrAlwaysNULL;

    EXPECT_CALL(vkGetInstanceProcAddrAlwaysNULL, Call(instance, _)).WillRepeatedly(Return(nullptr));

    EXPECT_EQ(getVkInstanceProcAddrWithFallback<GfxstreamTestFunc>({}, instance), nullptr);
    EXPECT_EQ(getVkInstanceProcAddrWithFallback<GfxstreamTestFunc>({nullptr, nullptr}, instance),
              nullptr);
    EXPECT_EQ(getVkInstanceProcAddrWithFallback<GfxstreamTestFunc>(
                  {vkGetInstanceProcAddrAlwaysNULL.AsStdFunction(),
                   vkGetInstanceProcAddrAlwaysNULL.AsStdFunction()},
                  instance),
              nullptr);
}

TEST(getVkInstanceProcAddrWithFallbackTest, ShouldSkipNullVkGetInstanceProcAddr) {
    VkInstance instance = reinterpret_cast<VkInstance>(0x1234'0000);
    PFN_vkGfxstreamTestFunc validFp = reinterpret_cast<PFN_vkGfxstreamTestFunc>(0x4321'0000);
    MockFunction<std::remove_pointer_t<PFN_vkGetInstanceProcAddr>> vkGetInstanceProcAddrMock;

    EXPECT_CALL(vkGetInstanceProcAddrMock, Call(instance, _))
        .WillRepeatedly(Return(reinterpret_cast<PFN_vkVoidFunction>(validFp)));

    EXPECT_EQ(getVkInstanceProcAddrWithFallback<GfxstreamTestFunc>(
                  {nullptr, vkGetInstanceProcAddrMock.AsStdFunction()}, instance),
              validFp);
    EXPECT_EQ(getVkInstanceProcAddrWithFallback<GfxstreamTestFunc>(
                  {vkGetInstanceProcAddrMock.AsStdFunction(), nullptr}, instance),
              validFp);
}

TEST(getVkInstanceProcAddrWithFallbackTest, ShouldSkipNullFpReturned) {
    VkInstance instance = reinterpret_cast<VkInstance>(0x1234'0000);
    PFN_vkGfxstreamTestFunc validFp = reinterpret_cast<PFN_vkGfxstreamTestFunc>(0x4321'0000);
    MockFunction<std::remove_pointer_t<PFN_vkGetInstanceProcAddr>> vkGetInstanceProcAddrMock;
    MockFunction<std::remove_pointer_t<PFN_vkGetInstanceProcAddr>> vkGetInstanceProcAddrAlwaysNULL;

    // We know that vkGfxstreamTest has different names.
    EXPECT_CALL(vkGetInstanceProcAddrMock,
                Call(instance, StrNe(std::get<1>(vkGfxstreamTestFuncNames))))
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(vkGetInstanceProcAddrMock,
                Call(instance, StrEq(std::get<1>(vkGfxstreamTestFuncNames))))
        .WillRepeatedly(Return(reinterpret_cast<PFN_vkVoidFunction>(validFp)));
    EXPECT_CALL(vkGetInstanceProcAddrAlwaysNULL, Call(instance, _)).WillRepeatedly(Return(nullptr));

    EXPECT_EQ(getVkInstanceProcAddrWithFallback<GfxstreamTestFunc>(
                  {vkGetInstanceProcAddrMock.AsStdFunction(),
                   vkGetInstanceProcAddrAlwaysNULL.AsStdFunction()},
                  instance),
              validFp);
    EXPECT_EQ(getVkInstanceProcAddrWithFallback<GfxstreamTestFunc>(
                  {vkGetInstanceProcAddrAlwaysNULL.AsStdFunction(),
                   vkGetInstanceProcAddrMock.AsStdFunction()},
                  instance),
              validFp);
}

TEST(getVkInstanceProcAddrWithFallbackTest, firstVkInstanceProcAddrShouldTakeThePriority) {
    VkInstance instance = reinterpret_cast<VkInstance>(0x1234'0000);
    PFN_vkGfxstreamTestFunc validFp1 = reinterpret_cast<PFN_vkGfxstreamTestFunc>(0x4321'0000);
    PFN_vkGfxstreamTestFunc validFp2 = reinterpret_cast<PFN_vkGfxstreamTestFunc>(0x3421'0070);
    MockFunction<std::remove_pointer_t<PFN_vkGetInstanceProcAddr>> vkGetInstanceProcAddrMock1;
    MockFunction<std::remove_pointer_t<PFN_vkGetInstanceProcAddr>> vkGetInstanceProcAddrMock2;

    EXPECT_CALL(vkGetInstanceProcAddrMock1, Call(instance, _))
        .WillRepeatedly(Return(reinterpret_cast<PFN_vkVoidFunction>(validFp1)));
    EXPECT_CALL(vkGetInstanceProcAddrMock2, Call(instance, _))
        .WillRepeatedly(Return(reinterpret_cast<PFN_vkVoidFunction>(validFp2)));

    EXPECT_EQ(getVkInstanceProcAddrWithFallback<GfxstreamTestFunc>(
                  {vkGetInstanceProcAddrMock1.AsStdFunction(),
                   vkGetInstanceProcAddrMock2.AsStdFunction()},
                  instance),
              validFp1);
}

TEST(getVkInstanceProcAddrWithFallbackTest, firstNameShouldTakeThePriority) {
    VkInstance instance = reinterpret_cast<VkInstance>(0x1234'0000);
    PFN_vkGfxstreamTestFunc validFps[] = {reinterpret_cast<PFN_vkGfxstreamTestFunc>(0x4321'0000),
                                          reinterpret_cast<PFN_vkGfxstreamTestFunc>(0x3421'0070),
                                          reinterpret_cast<PFN_vkGfxstreamTestFunc>(0x2222'4321)};
    MockFunction<std::remove_pointer_t<PFN_vkGetInstanceProcAddr>> vkGetInstanceProcAddrMock;

    EXPECT_CALL(vkGetInstanceProcAddrMock,
                Call(instance, StrEq(std::get<0>(vkGfxstreamTestFuncNames))))
        .WillRepeatedly(Return(reinterpret_cast<PFN_vkVoidFunction>(validFps[0])));
    ON_CALL(vkGetInstanceProcAddrMock, Call(instance, StrEq(std::get<1>(vkGfxstreamTestFuncNames))))
        .WillByDefault(Return(reinterpret_cast<PFN_vkVoidFunction>(validFps[1])));
    ON_CALL(vkGetInstanceProcAddrMock, Call(instance, StrEq(std::get<2>(vkGfxstreamTestFuncNames))))
        .WillByDefault(Return(reinterpret_cast<PFN_vkVoidFunction>(validFps[2])));

    EXPECT_EQ(getVkInstanceProcAddrWithFallback<GfxstreamTestFunc>(
                  {vkGetInstanceProcAddrMock.AsStdFunction()}, instance),
              validFps[0]);
}
}  // namespace
}  // namespace vk_fn_info
}  // namespace vk_util
