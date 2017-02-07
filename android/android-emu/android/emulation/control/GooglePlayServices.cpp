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

using android::base::StringView;
using android::base::System;
using std::string;

// static
const System::Duration GooglePlayServices::kTimeoutMs = 5000;
const string GooglePlayServices::kPlayStorePkgName = "com.android.vending";
const string GooglePlayServices::kPlayServicesPkgName =
        "com.google.android.gms";

GooglePlayServices::~GooglePlayServices() {
    if (mStoreSettingsCommand) {
        mStoreSettingsCommand->cancel();
    }
    if (mServicesPageCommand) {
        mServicesPageCommand->cancel();
    }
    if (mStoreVersionCommand) {
        mStoreVersionCommand->cancel();
    }
    if (mServicesVersionCommand) {
        mServicesVersionCommand->cancel();
    }
    if (mGetpropCommand) {
        mGetpropCommand->cancel();
    }
}

void GooglePlayServices::setAdbInterface(AdbInterface* adb) {
    mAdb = adb;
}

// static
bool GooglePlayServices::parseOutputForVersion(std::ifstream& stream,
                                               string* outString) {
    // Only accept output in the form of
    //
    //     <W>versionName=<version>
    //
    // Where <W> can be any number/kind of whitespace characters.

    const string keystr = "versionName=";
    string line;

    // "dumpsys package <pkgname>" may return more than one version
    // name, depending on if there were any updates installed. In
    // these cases, we only need the latest version name, which is
    // on the first line returned. If there is no output, or no
    // value for versionName, we can assume that this package is
    // not installed.

    if (!stream || !outString) {
        return false;
    }

    while (getline(stream, line)) {
        auto keyPos = line.find(keystr);
        if (keyPos != string::npos &&
            keyPos + keystr.length() < line.length()) {
            // Make sure everything before is only whitespace.
            bool isValid = true;
            for (size_t i = 0; i < keyPos; ++i) {
                if (!std::isspace(static_cast<unsigned char>(line[i]))) {
                    isValid = false;
                    break;
                }
            }
            if (isValid) {
              *outString =
                      line.substr(keyPos + keystr.length(),
                                  line.length() - (keyPos + keystr.length()));
              return true;
            }
        }
    }
    return false;
}

void GooglePlayServices::getSystemProperty(
        android::base::StringView sysProp,
        ResultOutputCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    std::vector<std::string> getpropCommand{"shell", "getprop", sysProp};
    mGetpropCommand = mAdb->runAdbCommand(
            getpropCommand,
            [resultCallback, this](const OptionalAdbCommandResult& result) {
                string line;
                if (result && result->output &&
                    getline(*(result->output), line)) {
                    resultCallback(Result::kSuccess, line);
                } else {
                    resultCallback(Result::kUnknownError, "");
                }
            },
            kTimeoutMs, true);
}

void GooglePlayServices::showPlayStoreSettings(ResultCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    if (mStoreSettingsCommand) {
        resultCallback(Result::kOperationInProgress);
    }
    mStoreSettingsCommand = mAdb->runAdbCommand(
            {"shell", "am start", "-n",
             kPlayStorePkgName +
                     "/com.google.android.finsky.activities.SettingsActivity"},
            [this, resultCallback](const OptionalAdbCommandResult& result) {
                resultCallback((!result || result->exit_code)
                                       ? Result::kUnknownError
                                       : Result::kSuccess);
                mStoreSettingsCommand.reset();
            },
            kTimeoutMs, false);
}

void GooglePlayServices::showPlayServicesPage(ResultCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    if (mServicesPageCommand) {
        resultCallback(Result::kOperationInProgress);
    }
    mServicesPageCommand = mAdb->runAdbCommand(
            {"shell", "am start", "-a", "android.intent.action.VIEW", "-d",
             "'market://details?id=" + kPlayServicesPkgName + "'"},
            [this, resultCallback](const OptionalAdbCommandResult& result) {
                resultCallback((!result || result->exit_code)
                                       ? Result::kUnknownError
                                       : Result::kSuccess);
                mServicesPageCommand.reset();
            },
            kTimeoutMs, false);
}

void GooglePlayServices::getPlayStoreVersion(
        ResultOutputCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    if (mStoreVersionCommand) {
        resultCallback(Result::kOperationInProgress, "");
    }
    mStoreVersionCommand = mAdb->runAdbCommand(
            {"shell", "dumpsys", "package", kPlayStorePkgName},
            [this, resultCallback](const OptionalAdbCommandResult& result) {
                if (!result || result->exit_code) {
                    resultCallback(Result::kUnknownError, "");
                } else {
                    string outString;
                    const bool parseResult = parseOutputForVersion(
                            *(result->output), &outString);
                    resultCallback(parseResult ? Result::kSuccess
                                               : Result::kAppNotInstalled,
                                   outString);
                }
                mStoreVersionCommand.reset();
            },
            kTimeoutMs, true);
}

void GooglePlayServices::getPlayServicesVersion(
        ResultOutputCallback resultCallback) {
    if (!mAdb) {
        return;
    }

    if (mServicesVersionCommand) {
        resultCallback(Result::kOperationInProgress, "");
    }
    mServicesVersionCommand = mAdb->runAdbCommand(
            {"shell", "dumpsys", "package", kPlayServicesPkgName},
            [this, resultCallback](const OptionalAdbCommandResult& result) {
                if (!result || result->exit_code) {
                    resultCallback(Result::kUnknownError, "");
                } else {
                    string outString;
                    const bool parseResult = parseOutputForVersion(
                            *(result->output), &outString);
                    resultCallback(parseResult ? Result::kSuccess
                                               : Result::kAppNotInstalled,
                                   outString);
                }
                mServicesVersionCommand.reset();
            },
            kTimeoutMs, true);
}

}  // namespace emulation
}  // namespace android
