// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/memory/MemoryUsage.h"

#include "android/base/Backtrace.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include <malloc_extension_c.h>
#include <malloc_hook.h>

#include <execinfo.h>
#include <libunwind.h>
#include <map>
#include <sstream>
#include <vector>
using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;

namespace android {
namespace base {

struct MallocStats {
    std::string label;
    int64_t allocated = 0;
    int64_t live = 0;
    int64_t peak = 0;
};

struct FuncRange {
    std::string name;
    void* addr;
    size_t length;
    MallocStats stats;
};

class MemoryUsage::Impl {
public:
    bool addToGroup(std::string group, std::string func) {
        std::string key = group + func;
        if (mData.find(group + func) != mData.end()) {
            return false;
        }

        unw_cursor_t cursor;
        unw_context_t uc;
        unw_getcontext(&uc);
        unw_init_local(&cursor, &uc);
        unw_step(&cursor);
        unw_step(&cursor);
        unw_proc_info_t info;
        if (0 != unw_get_proc_info(&cursor, &info)) {
            return false;
        }
        FuncRange fr;
        fr.name = func;
        fr.addr = (void*)info.start_ip;
        fr.length = info.end_ip - info.start_ip;
        mData[key] = fr;
        // for (auto& it : mData) {
        //    fprintf(stderr, "%s: %p %lu\t", it.second.name.c_str(),
        //    it.second.addr, it.second.length);
        //}
        // fprintf(stderr, "\n");

        // fprintf(stderr, "%s\n", android::base::bt().c_str());
        return true;
    }

    MallocStats aggregate(std::string label) {
        AutoLock lock(mLock);
        MallocStats ms;
        for (auto& it : mData) {
            if (it.first.compare(0, label.size(), label) == 0) {
                ms.allocated += it.second.stats.allocated;
                // ms.peak += it.second.stats.peak;
                ms.live += it.second.stats.live;
            }
        }
        return ms;
    }

    /*std::vector<MallocStats> aggregateAll() {

    }*/

    void setEnabled(bool enabled) {}

    void start();

    void stop();

    void newHook(const void* ptr, size_t size) {
        size = std::max(size, MallocExtension_GetAllocatedSize(ptr));

        AutoLock lock(mLock);
        int totalSteps = 0;
        void* results[kStackTraceLimit];
        int ret = backtrace(results, kStackTraceLimit);
        bool found = false;
        for (int i = 5; i < ret; i++) {
            if ((intptr_t)results[i] < 0x100) {
                break;
            }
            for (auto& it : mData) {
                if (std::abs((intptr_t)results[i] - (intptr_t)it.second.addr) <
                    it.second.length) {
                    auto& funcRange = it.second;
                    funcRange.stats.allocated += size;
                    funcRange.stats.live += size;
                    funcRange.stats.peak = std::max(funcRange.stats.peak,
                                                    funcRange.stats.live);
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
    }

    void deleteHook(const void* ptr) {
        size_t size = MallocExtension_GetAllocatedSize(ptr);
        AutoLock lock(mLock);
        int totalSteps = 0;
        void* results[kStackTraceLimit];
        int ret = backtrace(results, kStackTraceLimit);
        for (int i = 5; i < ret; i++) {
            if ((intptr_t)results[i] < 0x100) {
                break;
            }
            for (auto& it : mData) {
                if (std::abs((intptr_t)results[i] - (intptr_t)it.second.addr) <
                    it.second.length) {
                    auto& funcRange = it.second;
                    funcRange.stats.live -= size;
                    break;
                }
            }
        }
    }

    std::string printUsage() {
        std::stringstream ss;
        MallocStats stats = aggregate("GLES");
        /*for (auto& it : mData) {
            auto& stats = it.second.stats;
            ss << it.second.name;
            ss << " memory allocated: ";
            ss << (float)stats.allocated/1048576.0f;
            ss << "mb live: ";
            ss << (float)stats.live/1048576.0f;
            ss << "mb peak: ";
            ss << (float)stats.peak/1048576.0f;
            ss << "mb\n";
        }
        ss << "\n\n";*/
        ss << "GLES memory allocated: ";
        ss << (float)stats.allocated / 1048576.0f;
        ss << "mb live: ";
        ss << (float)stats.live / 1048576.0f;
        // ss << "mb peak: ";
        // ss << (float)stats.peak/1048576.0f;
        ss << "mb\n";
        stats = aggregate("EGL");
        ss << "EGL memory allocated: ";
        ss << (float)stats.allocated / 1048576.0f;
        ss << "mb live: ";
        ss << (float)stats.live / 1048576.0f;
        // ss << "mb peak: ";
        // ss << (float)stats.peak/1048576.0f;
        ss << "mb\n";
        return ss.str();
    }

    static void new_hook(const void* ptr, size_t size) {
        MemoryUsage::get()->mImpl->newHook(ptr, size);
    }

    static void delete_hook(const void* ptr) {
        MemoryUsage::get()->mImpl->deleteHook(ptr);
    }

private:
    std::map<std::string, FuncRange> mData;
    Lock mLock;
    static const int kStackTraceLimit = 32;
};

void MemoryUsage::Impl::start() {
    MallocHook_AddNewHook(new_hook);
    MallocHook_AddDeleteHook(delete_hook);
}

void MemoryUsage::Impl::stop() {
    MallocHook_RemoveNewHook(new_hook);
    MallocHook_RemoveDeleteHook(delete_hook);
}

MemoryUsage::MemoryUsage() : mImpl(new MemoryUsage::Impl()) {}

bool MemoryUsage::addToGroup(std::string group, std::string func) {
    return mImpl->addToGroup(group, func);
}


std::string MemoryUsage::printUsage() {
    return mImpl->printUsage();
}

void MemoryUsage::setEnabled(bool enabled) {
    mImpl->setEnabled(enabled);
}

void MemoryUsage::start() {
    mImpl->start();
}

void MemoryUsage::stop() {
    mImpl->stop();
}
static LazyInstance<MemoryUsage> sMemoryUsage = LAZY_INSTANCE_INIT;

// static
MemoryUsage* MemoryUsage::get() {
    return sMemoryUsage.ptr();
}

}  // namespace base
}  // namespace android