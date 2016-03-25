// Copyright (C) 2016 The Android Open Source Project
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

#pragma once

#include "android/base/async/Looper.h"
#include "android/base/Compiler.h"
#include "android/base/String.h"
#include "android/base/system/System.h"
#include "android/base/threads/ParallelTask.h"

#include <functional>
#include <memory>
#include <vector>

namespace android {
namespace emulation {

class ApkInstaller : public std::enable_shared_from_this<ApkInstaller> {
public:
    enum class Result {
        kSuccess,
        kOperationInProgress,
        kApkPermissionsError,
        kAdbConnectionFailed,
        kDeviceStorageFull,
        kInstallFailed,
        kUnknownError
    };

    using ResultCallback =
            std::function<void(Result result,
                               android::base::StringView errorString)>;

    // Entry point to ApkInstaller.
    // Objects of this type are managed via std::shared_ptr
    // Args:
    //     adbCommandArgs: The command line prefix for use with adb. Usually,
    //             'adb -s "emulator_name"'
    static std::shared_ptr<ApkInstaller> create(
            android::base::Looper* looper,
            const android::base::StringVector& adbCommandArgs,
            android::base::StringView apkFilePath,
            ResultCallback resultCallback);

    // In case of synchronous failures, this will call |resultCallback| before
    // returning.
    void start();

    // Cancel an ongoing install. This DOES NOT guarantee that the operation
    // will be cancelled. But it ensures that the resultCallback is not called
    // in the future.
    void cancel();

    // Is an install being done currently?
    // The task may be complete but awaiting the return of mResultCallback
    // in taskDoneFunction. So don't check if we're in flight, just check if
    // the task exists at all. If it does, we're still in flight.
    bool inFlight() const { return bool(mParallelTask); }

    // Update various paths.
    // Fails if an install is in progress, indicated by the return value.
    bool setAdbCommandArgs(const android::base::StringVector& adbCommandArgs);
    bool setApkFilePath(android::base::StringView apkFilePath);

    // Parses the file at |outputFilePath| as if it was the output from running
    // "adb install" to see if the install succeeded. Returns true if the
    // install succeeded, false otherwise. If the install failed,
    // |outErrorString| is populated with the error from "adb install".
    static bool parseOutputForFailure(const android::base::String& outputFilePath,
                                      android::base::String* outErrorString);

    static const char kDefaultErrorString[];

private:
    static const android::base::System::Duration kInstallTimeoutMs;
    static const char kInstallOutputFileBase[];

    ApkInstaller(android::base::Looper* looper,
                 const android::base::StringVector& adbCommandArgs,
                 android::base::StringView apkFilePath,
                 ResultCallback resultCallback);

    intptr_t taskFunction(Result* outResult);
    void taskDoneFunction(const Result& result);


private:
    android::base::Looper* const mLooper;
    const ResultCallback mResultCallback;
    android::base::String mApkFilePath;
    android::base::StringVector mInstallCommand;

    bool mCancelled = false;
    std::unique_ptr<android::base::ParallelTask<ApkInstaller::Result>>
            mParallelTask;
    const android::base::String mOutputFilePath;
    android::base::String mErrorCode;

    DISALLOW_COPY_AND_ASSIGN(ApkInstaller);
};

}  // namespace emulation
}  // namespace android
