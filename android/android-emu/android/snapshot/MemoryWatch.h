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
typedef int (*hva2gpa_t)(void* hva, uint64_t length, int max,
                         uint64_t* gpa, uint64_t* size);

typedef int (*guest_mem_map_t)(void* hva, uint64_t gpa, uint64_t size, uint64_t flags);
typedef int (*guest_mem_unmap_t)(uint64_t gpa, uint64_t size);
typedef int (*guest_mem_protect_t)(uint64_t gpa, uint64_t size, uint64_t flags);
typedef int (*guest_mem_remap_t)(void* hva, uint64_t gpa, uint64_t size, uint64_t flags);

extern hva2gpa_t hva2gpa_call;
extern gpa2hva_t gpa2hva_call;

extern guest_mem_map_t guest_mem_map_call;
extern guest_mem_unmap_t guest_mem_unmap_call;
extern guest_mem_protect_t guest_mem_protect_call;
extern guest_mem_remap_t guest_mem_remap_call;

void set_address_translation_funcs(hva2gpa_t hva2gpa, gpa2hva_t gpa2hva);
void set_memory_mapping_funcs(guest_mem_map_t guest_mem_map,
                              guest_mem_unmap_t guest_mem_unmap,
                              guest_mem_protect_t guet_mem_protect,
                              guest_mem_remap_t guet_mem_remap);

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

    void join();

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace snapshot
}  // namespace android
