#include "vm_mem_mappings.h"

#include "android/base/memory/LazyInstance.h"

#include <vector>

using android::base::LazyInstance;

struct Range {
    void* begin;
    uint64_t len;
    uint64_t gpa;
};

void* firstHva = 0;

typedef std::vector<Range> MemRanges;

static LazyInstance<MemRanges> sRanges;

void add_hva_gpa(void* hva, uint64_t len, uint64_t gpa) {
    // fprintf(stderr, "%s: host [%p %p] -> [%p %p]\n", __func__, hva, ((char*)hva) + len, gpa, gpa + len);
    sRanges.get().push_back({ hva, len, gpa });
    if (gpa == 0) { firstHva = hva; }
}

void remove_hva_gpa(uint64_t len, uint64_t gpa) {
    // fprintf(stderr, "%s: [%p %p]\n", __func__, gpa, gpa + len);
    sRanges.get().erase(std::remove_if(
                sRanges.get().begin(),
                sRanges.get().end(),
                [gpa, len](const Range& r) {
                return gpa == r.gpa && len == r.len;
                }),
            sRanges.get().end());
}

uint64_t hva2gpa(void* hva, bool* found) {
    uintptr_t me = (uintptr_t)hva;
    for (const auto& elt : sRanges.get()) {
        uintptr_t other1 = (uintptr_t)elt.begin;
        uintptr_t other2 = (uintptr_t)(((char*)elt.begin) + elt.len);
        if (me < other2 && me >= other1) {
            uintptr_t res = me - other1;
            // fprintf(stderr, "%s: hva %p -> gpa 0x%llx\n", __func__, hva, elt.gpa + res);
            *found = true;
            return elt.gpa + res;
        }
    }
    
    // fprintf(stderr, "%s: hva %p not found in any range!\n", __func__, hva);
    *found = false;
    return 0;
}

void* gpa2hva(uint64_t gpa, bool* found) {
    // *found = true;
    // return ((char*)firstHva) + gpa;

    uintptr_t me = (uintptr_t)gpa;
    for (const auto& elt : sRanges.get()) {
        uintptr_t other1 = (uintptr_t)elt.gpa;
        uintptr_t other2 = (uintptr_t)(((char*)elt.gpa) + elt.len);
        if (me < other2 && me >= other1) {
            uintptr_t res = me - other1;
            *found = true;
            return (void*)(((char*)elt.begin) + res);
        }
    }
    
    // fprintf(stderr, "%s: gpa 0x%llx not found in any range!\n", __func__, gpa);
    *found = false;
    return nullptr;
}
