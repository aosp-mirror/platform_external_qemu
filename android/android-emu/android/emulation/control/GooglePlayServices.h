// Copyright (C) 2017 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/system/System.h"
#include "android/emulation/control/AdbInterface.h"
#include "android/emulation/control/AndroidPropertyInterface.h"

#include <functional>
#include <string>

namespace android {
namespace emulation {

class GooglePlayServices {
public:
    enum class Result {
        Success,
        OperationInProgress,
        AppNotInstalled,
        UnknownError
    };

    using ResultCallback = std::function<void(Result result)>;
    using ResultOutputCallback =
            std::function<void(Result result,
                               android::base::StringView outString)>;

    explicit GooglePlayServices(AdbInterface* adb,
                                AndroidPropertyInterface* qemuProp)
        : mAdb(adb), mAndroidProp(qemuProp) {}
    ~GooglePlayServices();

    // Opens the Google play store settings in the play store app.
    void showPlayStoreSettings(ResultCallback resultCallback);
    // Opens the Play Services app details page in the play store.
    void showPlayServicesPage(ResultCallback resultCallback);
    // Gets a string of the latest version number of the play store.
    void getPlayStoreVersion(ResultOutputCallback resultCallback);
    // Get a string of the latest version number of play services.
    void getPlayServicesVersion(ResultOutputCallback resultCallback);
    // Get a system property.
    void waitForBootCompletion(ResultOutputCallback resultCallback);
    // Wait for update on play store.
    void waitForPlayStoreUpdate(ResultCallback resultCallback);
    // Wait for update on play services.
    void waitForPlayServicesUpdate(ResultCallback resultCallback);

    // Parses the contents of |output| stream as if it was the output from
    // running "adb shell dumpsys <pkgname>" to get the
    // latest version name of the package. Returns true if the output has
    // a line in the form of "versionName=<pkgVersion>", false otherwise.
    // There may be leading spaces in the line.
    static bool parseOutputForVersion(std::istream& output,
                                      std::string* outErrorString);

private:
    static const android::base::System::Duration kAdbCommandTimeoutMs;
    static const android::base::System::Duration kWaitPropTimeoutMs;
    static constexpr android::base::StringView kPlayStorePkgName =
            "com.android.vending";
    static constexpr android::base::StringView kPlayServicesPkgName =
            "com.google.android.gms";

    AdbInterface* mAdb;
    AndroidPropertyInterface* mAndroidProp;
    WaitObjectPtr mBootWaitObject;
    WaitObjectPtr mStoreUpdateWaitObject;
    WaitObjectPtr mServicesUpdateWaitObject;
    AdbCommandPtr mStoreSettingsCommand;
    AdbCommandPtr mServicesPageCommand;
    AdbCommandPtr mStoreVersionCommand;
    AdbCommandPtr mServicesVersionCommand;
    const ResultCallback mResultCallback;

    DISALLOW_COPY_AND_ASSIGN(GooglePlayServices);
};

}  // namespace emulation
}  // namespace android
