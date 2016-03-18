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

FilePusher::create(
        Looper* looper,
        const vector<string>& adbCommandArgs,
        ProgressCallback progressCallback,
        PushFailedCallback pushFailedCallback) {
    auto inst = new FilePusher(looper, adbCommandArgs, progressCallback,
                               pushFailedCallback);
    return std::shared_ptr<FilePusher>(inst);
}

FilePusher::FilePusher(
        Looper* looper,
        const vector<string>& adbCommandArgs,
        ProgressCallback progressCallback,
        PushFailedCallback pushFailedCallback) :
    mLooper(looper),
    mAdbCommandArgs(adbCommandArgs),
    mProgressCallback(progressCallback),
    mPushFailedCallback(pushFailedCallback) {}

}  // namespace emulation
}  // namespace android
