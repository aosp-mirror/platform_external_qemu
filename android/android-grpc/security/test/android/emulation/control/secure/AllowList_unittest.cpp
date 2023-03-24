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
#include "android/emulation/control/secure/AllowList.h"

#include <gtest/gtest.h>  // for SuiteApiResolver, Message
#include <chrono>
#include <fstream>
#include <vector>
#include "aemu/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"

namespace android {
namespace emulation {
namespace control {

using android::base::PathUtils;
using android::base::pj;
using android::base::System;

TEST(AllowListTest, bad_is_not_null) {
    EXPECT_NE(AllowList::fromJson("xxx"), nullptr);
}

TEST(AllowListTest, detects_unprotected) {
    auto list = AllowList::fromJson(R"#(
    {
    // Set of methods that do not require any validations, the do not require a token.
    // If your method DOES NOT match this list, you will require a token of sorts.
    "unprotected": [
        ".*/getGps"
    ]})#");
    EXPECT_FALSE(list->requiresAuthentication("foo/getGps"));
    EXPECT_TRUE(list->requiresAuthentication("bar/huusku"));
}

TEST(AllowListTest, always_detects_unprotected) {
    auto list = AllowList::fromJson(R"#(
    {
    // Set of methods that do not require any validations, the do not require a token.
    // If your method DOES NOT match this list, you will require a token of sorts.
    "unprotected": [
        ".*/getGps"
    ]})#");

    // 2nd call might be cached.
    EXPECT_FALSE(list->requiresAuthentication("foo/getGps"));
    EXPECT_FALSE(list->requiresAuthentication("foo/getGps"));
    EXPECT_TRUE(list->requiresAuthentication("bar/huusku"));
    EXPECT_TRUE(list->requiresAuthentication("bar/huusku"));
}

TEST(AllowListTest, subject_is_protected) {
    auto list = AllowList::fromJson(R"#(
    {
    "allowlist": [
         {
            "iss": "android-studio",
            "allowed": [
                 ".*/getGps"
            ]
        }
    ]})#");

    EXPECT_TRUE(list->isAllowed("android-studio", "foo/getGps"));
    EXPECT_FALSE(list->isAllowed("gradle", "foo/getGps"));
}

TEST(AllowListTest, do_not_consume_memory) {
    using namespace std::chrono_literals;
    auto list = AllowList::fromJson(R"#(
    {
    "allowlist": [
         {
            "iss": "android-studio",
            "allowed": [
                 ".*/getGps"
            ]
        }
    ]})#");
    auto end = std::chrono::system_clock::now() + 1s;
    int i = 0;
    while (std::chrono::system_clock::now() < end) {
        i = (i + 1) % 512;
        EXPECT_TRUE(list->isAllowed("android-studio",
                                  "foo" + std::to_string(i) + "/getGps"));
    }
}

TEST(AllowListTest, subject_is_always_protected) {
    auto list = AllowList::fromJson(R"#(
    {
    "allowlist": [
         {
            "iss": "android-studio",
            "allowed": [
                 ".*/getGps"
            ]
        }
    ]})#");

    EXPECT_TRUE(list->isAllowed("android-studio", "foo/getGps"));
    EXPECT_TRUE(list->isAllowed("android-studio", "foo/getGps"));
    EXPECT_FALSE(list->isAllowed("gradle", "foo/getGps"));
    EXPECT_FALSE(list->isAllowed("gradle", "foo/getGps"));
}

TEST(AllowListTest, ignore_reserved_iss) {
    auto list = AllowList::fromJson(R"#(
    {
    "allowlist": [
         {
            "iss": "__default__",
            "allowed": [
                 ".*/getGps"
            ]
        }
    ]})#");
    EXPECT_TRUE(list->requiresAuthentication("foo/getGps"));
    EXPECT_TRUE(list->isRed("__everyone__", "foo/getGps"));
}

TEST(AllowListTest, can_parse_default_list) {
    // Make sure the default list we ship actually parses
    // and has some expected values..
    auto access = pj(pj(System::get()->getProgramDirectory(), "lib"),
                     "emulator_access.json");

    auto file = std::ifstream(access);
    ASSERT_TRUE(file.good());

    auto list = AllowList::fromStream(file);
    std::vector<std::string> allowed{
            "/android.emulation.control.EmulatorController/"
            "closeExtendedControls",
            "/android.emulation.control.EmulatorController/getClipboard",
            "/android.emulation.control.EmulatorController/"
            "getDisplayConfigurations",
            "/android.emulation.control.EmulatorController/getPhysicalModel",
            "/android.emulation.control.EmulatorController/getScreenshot",
            "/android.emulation.control.EmulatorController/getStatus",
            "/android.emulation.control.EmulatorController/getVmState",
            "/android.emulation.control.EmulatorController/injectAudio",
            "/android.emulation.control.EmulatorController/"
            "rotateVirtualSceneCamera",
            "/android.emulation.control.EmulatorController/sendKey",
            "/android.emulation.control.EmulatorController/sendMouse",
            "/android.emulation.control.EmulatorController/sendTouch",
            "/android.emulation.control.EmulatorController/setClipboard",
            "/android.emulation.control.EmulatorController/"
            "setDisplayConfigurations",
            "/android.emulation.control.EmulatorController/setDisplayMode",
            "/android.emulation.control.EmulatorController/setPhysicalModel",
            "/android.emulation.control.EmulatorController/"
            "setVirtualSceneCameraVelocity",
            "/android.emulation.control.EmulatorController/setVmState",
            "/android.emulation.control.EmulatorController/"
            "showExtendedControls",
            "/android.emulation.control.EmulatorController/streamClipboard",
            "/android.emulation.control.EmulatorController/streamNotification",
            "/android.emulation.control.EmulatorController/streamScreenshot",
            "/android.emulation.control.SnapshotService/DeleteSnapshot",
            "/android.emulation.control.SnapshotService/ListSnapshots",
            "/android.emulation.control.SnapshotService/LoadSnapshot",
            "/android.emulation.control.SnapshotService/PushSnapshot",
            "/android.emulation.control.SnapshotService/SaveSnapshot"};
    for (const auto& isGreen : allowed) {
        EXPECT_TRUE(list->isAllowed("android-studio", isGreen));
    }
}
}  // namespace control
}  // namespace emulation
}  // namespace android
