// Copyright 2018 The Android Open Source Project
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

#include "android/emulation/testing/MockAndroidVmOperations.h"
#include "android/base/Log.h"

static bool sSkipSnapshotSave = false;

static const QAndroidVmOperations sQAndroidVmOperations = {
        .vmStop = []() -> bool {
            CHECK(android::MockAndroidVmOperations::mock);
            return android::MockAndroidVmOperations::mock->vmStop();
        },
        .vmStart = []() -> bool {
            CHECK(android::MockAndroidVmOperations::mock);
            return android::MockAndroidVmOperations::mock->vmStart();
        },
        .vmReset = nullptr,       // Not currently mocked.
        .vmShutdown = nullptr,    // Not currently mocked.
        .vmPause = nullptr,       // Not currently mocked.
        .vmResume = nullptr,      // Not currently mocked.
        .vmIsRunning = nullptr,   // Not currently mocked.
        .snapshotList = nullptr,  // Not currently mocked.
        .snapshotSave = [](const char* name,
                           void* opaque,
                           LineConsumerCallback errConsumer) -> bool {
            CHECK(android::MockAndroidVmOperations::mock);
            return android::MockAndroidVmOperations::mock->snapshotSave(
                    name, opaque, errConsumer);
        },
        .snapshotLoad = [](const char* name,
                           void* opaque,
                           LineConsumerCallback errConsumer) -> bool {
            CHECK(android::MockAndroidVmOperations::mock);
            return android::MockAndroidVmOperations::mock->snapshotLoad(
                    name, opaque, errConsumer);
        },
        .snapshotDelete = nullptr,  // Not currently mocked.
        .snapshotRemap = [](bool shared,
                            void* opaque,
                            LineConsumerCallback errConsumer) -> bool {
            CHECK(android::MockAndroidVmOperations::mock);
            return android::MockAndroidVmOperations::mock->snapshotRemap(
                    shared, opaque, errConsumer);
        },
        .setSnapshotCallbacks =
                [](void* opaque, const SnapshotCallbacks* callbacks) {
                    CHECK(android::MockAndroidVmOperations::mock);
                    return android::MockAndroidVmOperations::mock
                            ->setSnapshotCallbacks(opaque, callbacks);
                },
        .mapUserBackedRam = [](uint64_t gpa, void* hva, uint64_t size) {},
        .unmapUserBackedRam = [](uint64_t gpa, uint64_t size) {},
        .getVmConfiguration = nullptr,       // Not currently mocked.
        .setFailureReason = nullptr,         // Not currently mocked.
        .setExiting = nullptr,               // Not currently mocked.
        .allowRealAudio = nullptr,           // Not currently mocked.
        .physicalMemoryGetAddr = nullptr,    // Not currently mocked.
        .isRealAudioAllowed = nullptr,       // Not currently mocked.
        .setSkipSnapshotSave =
            [](bool skip) { sSkipSnapshotSave = skip; },
        .isSnapshotSaveSkipped =
            []() -> bool { return sSkipSnapshotSave; },
};

extern "C" const QAndroidVmOperations* const gQAndroidVmOperations =
        &sQAndroidVmOperations;

namespace android {

MockAndroidVmOperations* MockAndroidVmOperations::mock = nullptr;

}  // namespace android
