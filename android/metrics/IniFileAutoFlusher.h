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

#pragma once

#include "android/base/async/Looper.h"
#include "android/base/files/IniFile.h"

#include <memory>

namespace android {
namespace metrics {

// Takes an |iniFile| and routinely flushes the contents to disk.
// This is especially useful if the |iniFile| object is expected to receive many
// updates, and you'd like to batch together the flushes to disk.
//
// __If there are any changes to the IniFile__, the file is flushed to disk when
// - Routinely when a timer expires.
// - When this object is destructed.
//
// Additionally, this object takes ownership of the underlying |iniFile|. This
// way, upon destruction, it can flush the file and delete the object together.
//
// Currently, it is not possible to release / reset the |iniFile| being
// monitored.
class IniFileAutoFlusher {
public:
    explicit IniFileAutoFlusher(android::base::Looper* looper)
        : mLooper(looper) {}

    ~IniFileAutoFlusher();

    // Start autoFlushing |iniFile|.
    // Returns true if we were successfully able to flush the file once, and set
    // up future flushes.
    // Will return false if called multiple times.
    bool start(std::unique_ptr<android::base::IniFile> iniFile);

    // Accessor, in case you don't want to keep a separate reference to the
    // underlying |iniFile| around.
    android::base::IniFile* iniFile() { return mIniFile.get(); }

protected:
    static android::base::Looper::Duration kWriteCallbackTimeoutMs;

    static void writeCallback(void* opaqueThis,
                              android::base::Looper::Timer* timer);

private:
    android::base::Looper* mLooper;
    std::unique_ptr<android::base::Looper::Timer> mTimer;
    std::unique_ptr<android::base::IniFile> mIniFile;

    DISALLOW_COPY_AND_ASSIGN(IniFileAutoFlusher);
};

}  // namespace metrics
}  // namespace android
