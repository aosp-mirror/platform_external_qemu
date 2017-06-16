// Copyright 2017 The Android Open Source Project
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

#include <functional>
#include <memory>

namespace android {
namespace snapshot {

struct GuestPageInfo {
    uint64_t addr;
    bool exists;
};

class MemoryAccessWatch {
public:
    static bool isSupported();

    enum class IdleCallbackResult {
        RunAgain, Wait, AllDone
    };


    using AccessCallback = std::function<void(void*)>;
    using IdleCallback = std::function<IdleCallbackResult()>;

    MemoryAccessWatch(AccessCallback&& accessCallback,
                      IdleCallback&& idleCallback);

    ~MemoryAccessWatch();

    bool valid() const;
    bool registerMemoryRange(void* start, size_t length, const GuestPageInfo& guestPageInfo);
    void doneRegistering();
    bool fillPage(void* ptr, size_t length, const void* data, const GuestPageInfo& guestPageInfo);
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace snapshot
}  // namespace android
