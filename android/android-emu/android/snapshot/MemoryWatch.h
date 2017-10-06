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

typedef void* (*gpa2hva_t)(uint64_t gpa, bool* found);
typedef uint64_t (*hva2gpa_t)(void* hva, bool* found);

extern hva2gpa_t hva2gpa_call;
extern gpa2hva_t gpa2hva_call;

void set_address_translation_funcs(hva2gpa_t hva2gpa, gpa2hva_t gpa2hva);

namespace android {
namespace snapshot {

class MemoryAccessWatch {
public:
    // The source and policy of the access, which can be important
    // for dirty page tracking on non-Linux systems:
    enum class AccessType {
        Guest = 0, // directly from hypervisor
        HostOnceOnly = 1, // from host, and unsafe to
                          // delay any more for dirty tracking
                          // (immediately dirty)
        Host = 2, // from host, safe to delay for dirty tracking
    };
    static bool isSupported();
    static bool dirtyTrackingSupported();

    enum class IdleCallbackResult {
        RunAgain, Wait, AllDone
    };

    using AccessCallback = std::function<void(AccessType, void*)>;
    using IdleCallback = std::function<IdleCallbackResult()>;
    using DirtyCallback = std::function<void(void*)>;

    MemoryAccessWatch(AccessCallback&& accessCallback,
                      IdleCallback&& idleCallback,
                      DirtyCallback&& dirtyCallback);

    ~MemoryAccessWatch();

    bool valid() const;
    bool registerMemoryRange(void* start, size_t length);
    void doneRegistering();
    bool fillPage(bool loadedAlready,
                  AccessType accessType, void* ptr, size_t length, const void* data,
                  bool isQuickboot);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace snapshot
}  // namespace android
