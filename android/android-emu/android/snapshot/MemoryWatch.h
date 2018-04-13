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
typedef bool (*guest_mem_protection_supported_t)(void);

extern hva2gpa_t hva2gpa_call;
extern gpa2hva_t gpa2hva_call;

extern guest_mem_map_t guest_mem_map_call;
extern guest_mem_unmap_t guest_mem_unmap_call;
extern guest_mem_protect_t guest_mem_protect_call;
extern guest_mem_remap_t guest_mem_remap_call;
extern guest_mem_protection_supported_t guest_mem_protection_supported_call;

void set_address_translation_funcs(hva2gpa_t hva2gpa, gpa2hva_t gpa2hva);
void set_memory_mapping_funcs(guest_mem_map_t guest_mem_map,
                              guest_mem_unmap_t guest_mem_unmap,
                              guest_mem_protect_t guet_mem_protect,
                              guest_mem_remap_t guet_mem_remap,
                              guest_mem_protection_supported_t
                              guest_mem_protection_supported);

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
    void initBulkFill(void* ptr, size_t length);
    bool fillPageBulk(void* ptr, size_t length, const void* data,
                      bool isQuickboot);
    void endBulkFill();

    void join();

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace snapshot
}  // namespace android
