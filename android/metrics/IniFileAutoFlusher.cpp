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

#include "android/metrics/IniFileAutoFlusher.h"

#include "android/base/Log.h"

namespace android {
namespace metrics {

using android::base::IniFile;
using android::base::Looper;
using std::shared_ptr;

using WeakPtr = std::weak_ptr<IniFileAutoFlusher>;

// static
// Every minute.
Looper::Duration IniFileAutoFlusher::kWriteCallbackTimeoutMs = 60 * 1000;

bool IniFileAutoFlusher::start(shared_ptr<IniFile> iniFile) {
    if (mIniFile.get()) {
        LOG(WARNING) << "Switching the monitored |iniFile| is "
                        "currently not supported.";
        return false;
    }

    mIniFile = std::move(iniFile);

    // Be cynical, flush regardless of changes.
    if (!mIniFile->write()) {
        return false;
    }

    // QEMU timer code may run the callback even if the timer has just been
    // stopped. This means it may even run it if the class is already deleted.
    // That's why we pass a weak pointer here - it serves as a simple state
    // monitor - if one was able to lock it, the object lives for the lifetime
    // of the locked pointer; if locking fails, the object is dead for sure.
    // Note: timer code _may_ run the callback; if it doesn't run it, the
    //  allocated weak pointer leaks.
    mTimer.reset(mLooper->createTimer(&IniFileAutoFlusher::writeCallback,
                                      new WeakPtr(shared_from_this())));
    mTimer->startRelative(kWriteCallbackTimeoutMs);
    return true;
}

// static
void IniFileAutoFlusher::writeCallback(void* opaque, Looper::Timer* timer) {
    const auto weakPtr = static_cast<WeakPtr*>(opaque);
    if (const auto thisPtr = weakPtr->lock()) {
        if (!thisPtr->mIniFile->writeIfChanged()) {
            LOG(VERBOSE) << "Attempt to auto flush IniFile \""
                         << thisPtr->mIniFile->getBackingFile() << "\" failed.";
        }
        thisPtr->mTimer->startRelative(kWriteCallbackTimeoutMs);
    } else {
        // looks like we've been already deleted in the main thread
        delete weakPtr;
    }
}

IniFileAutoFlusher::~IniFileAutoFlusher() {
    if (mTimer.get()) {
        mTimer->stop();
        mTimer.reset();
    }

    if (mIniFile.get()) {
        if (!mIniFile->writeIfChanged()) {
            LOG(WARNING) << "Final attempt to flush IniFile \""
                         << mIniFile->getBackingFile()
                         << "\" failed. Your data may be incomplete!";
        }
    }
}

}  // namespace metrics
}  // namespace android
