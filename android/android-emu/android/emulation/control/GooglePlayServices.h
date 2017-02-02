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

#include <functional>

namespace android {
namespace emulation {

class GooglePlayServices {
public:
    enum class Result { Success, OperationInProgress, UnknownError };

    using ResultCallback = std::function<void(Result result)>;

    explicit GooglePlayServices(AdbInterface* adb) : mAdb(adb) {}
    ~GooglePlayServices();

    // Opens the Google play store settings in the play store app.
    void showPlayStoreSettings(ResultCallback resultCallback);
    // Opens the Play Services app details page in the play store.
    void showPlayServicesPage(ResultCallback resultCallback);

private:
    static const android::base::System::Duration kAdbCommandTimeoutMs;

    AdbInterface* mAdb;
    AdbCommandPtr mStoreSettingsCommand;
    AdbCommandPtr mServicesPageCommand;
    const ResultCallback mResultCallback;

    DISALLOW_COPY_AND_ASSIGN(GooglePlayServices);
};

}  // namespace emulation
}  // namespace android
