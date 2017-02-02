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

#include "android/emulation/control/GooglePlayServices.h"

namespace android {
namespace emulation {

using android::base::System;

// static
const System::Duration GooglePlayServices::kAdbCommandTimeoutMs = 5000;

GooglePlayServices::~GooglePlayServices() {
    if (mStoreSettingsCommand) {
        mStoreSettingsCommand->cancel();
    }
    if (mServicesPageCommand) {
        mServicesPageCommand->cancel();
    }
}

void GooglePlayServices::showPlayStoreSettings(ResultCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    if (mStoreSettingsCommand) {
        resultCallback(Result::OperationInProgress);
        return;
    }
    mStoreSettingsCommand = mAdb->runAdbCommand(
            {"shell", "am start", "-n",
             "com.android.vending/"
             "com.google.android.finsky.activities.SettingsActivity"},
            [this, resultCallback](const OptionalAdbCommandResult& result) {
                resultCallback((!result || result->exit_code)
                                       ? Result::UnknownError
                                       : Result::Success);
                mStoreSettingsCommand.reset();
            },
            kAdbCommandTimeoutMs, false);
}

void GooglePlayServices::showPlayServicesPage(ResultCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    if (mServicesPageCommand) {
        resultCallback(Result::OperationInProgress);
        return;
    }
    mServicesPageCommand = mAdb->runAdbCommand(
            {"shell", "am start", "-a", "android.intent.action.VIEW", "-d",
             "'market://details?id=com.google.android.gms'"},
            [this, resultCallback](const OptionalAdbCommandResult& result) {
                resultCallback((!result || result->exit_code)
                                       ? Result::UnknownError
                                       : Result::Success);
                mServicesPageCommand.reset();
            },
            kAdbCommandTimeoutMs, false);
}

}  // namespace emulation
}  // namespace android
