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
    bool registerMemoryRange(void* start, size_t length);
    void doneRegistering();
    bool fillPage(void* ptr, size_t length, const void* data,
                  bool isQuickboot);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace snapshot
}  // namespace android
