// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/metrics/AutoFlushedIniFile.h"

#include "android/base/Log.h"

namespace android {
namespace metrics {

using android::base::IniFile;
using android::base::Looper;
using std::unique_ptr;

// static
// Every minute.
Looper::Duration AutoFlushedIniFile::kWriteCallbackTimeoutMs = 60 * 1000;

bool AutoFlushedIniFile::start(unique_ptr<IniFile> iniFile) {
    if (mIniFile.get()) {
        LOG(WARNING) << "Switching the monitored |iniFile| is "
                     << "currently not supported.";
        return false;
    }
    mIniFile.reset(iniFile.release());

    // Be cynical, flush regardless of changes.
    if (!mIniFile->write()) {
        return false;
    }

    mTimer.reset(
            mLooper->createTimer(&AutoFlushedIniFile::writeCallback, this));
    mTimer->startRelative(kWriteCallbackTimeoutMs);
    return true;
}

// static
void AutoFlushedIniFile::writeCallback(void* opaqueThis, Looper::Timer* timer) {
    auto thisPtr = static_cast<AutoFlushedIniFile*>(opaqueThis);
    if (!thisPtr->mIniFile->writeIfChanged()) {
        LOG(VERBOSE) << "Attempt to auto flush IniFile "
                     << thisPtr->mIniFile->getBackingFile() << " failed.";
    }
    thisPtr->mTimer->startRelative(kWriteCallbackTimeoutMs);
}

AutoFlushedIniFile::~AutoFlushedIniFile() {
    if (mTimer.get()) {
        mTimer->stop();
        mTimer.reset();
    }

    if (mIniFile.get()) {
        // Be cynical, flush regardless of changes.
        if (!mIniFile->write()) {
            LOG(WARNING) << "Final attempt to flush IniFile "
                         << mIniFile->getBackingFile()
                         << " failed. Your data may be incomplete!";
        }
        // Explicitly kill the object right here to ensure no further updates.
        mIniFile.reset();
    }
}

}  // namespace metrics
}  // namespace android
