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

#include "android/emulation/control/FilePusher.cpp"

namespace android {
namespace emulation {

using android::base::Looper;
using android::base::StringView;
using std::string;
using std::vector;

FilePusher::FilePusher(Looper* looper,
                       const vector<string>& adbCommandArgs,
                       ProgressCallback progressCallback,
                       PushResultCallback pushResultCallback,
                       Looper::Duration progressCheckIntervalMs) :
    mLooper(looper),
    mProgressCallback(progressCallback),
    mPushResultCallback(pushResultCallback),
    mMonitorProgressTask(looper,
                         [this] () { this->monitorPush(); },
                         progressCheckIntervalMs) {
        setAdbCommandArgs(adbCommandArgs);
    }

void FilePusher::setAdbCommandArgs(const vector<string>& adbCommandArgs) {
    mAdbCommandArgs = adbCommandArgs;
}

bool FilePusher::enqueue(StringView localFilePath,
                         StringView remoteFilePath) {
    if (mFatalFailureOccured) {
        return false;
    }

    mPushQueue.emplace(localFilePath, remoteFilePath, statSize(localFilePath));
    if (mPushQueue.size() == 1) {
        pushNextItem();
    }
    return true;
}

void FilePusher::pushNextItem() {
    if (mPushQueue.empty()) {
        mMonitorProgressTask.stop();
        return;
    }

    auto pushItem = mPushQueue.pop();
    // Attempt the actual adb push.

    if(!mIsProgressCheckActive) {
        mIsProgressCheckActive = true;
        mMonitorProgressTask.start();
    }
}

bool FilePusher::monitorPush() {
    // Check if last push finished, notify result if applicable.
    // Parse output file.
    // send progress update if applicable.
}

size_t FilePusher::size(StringView localFilePath) {
    return 0;
}

}  // namespace emulation
}  // namespace android
