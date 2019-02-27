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

#include <libunwind.h>
#include <sstream>
#include <unordered_map>
#include <vector>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;

namespace android {
namespace base {

struct MallocStats {
    std::string label;
    std::atomic<int64_t> mAllocated{0};
    std::atomic<int64_t> mLive{0};
    std::atomic<int64_t> mPeak{0};
};

struct FuncRange {
    std::string mName;
    void* mAddr;
    size_t mLength;
    MallocStats mStats;
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
        AutoLock lock(mLock);
        mData.emplace(key, new FuncRange{
                func, (void*)info.start_ip, info.end_ip - info.start_ip});
        /*for (auto& it : mData) {
            fprintf(stderr, "%s: %p %lu\t", it.second->mName.c_str(),
            it.second->mAddr, it.second->mLength);
        }
        fprintf(stderr, "\n");*/

        return true;
    }

    void aggregate(std::string label, MallocStats* ms) {
        for (auto& it : mData) {
            if (it.first.compare(0, label.size(), label) == 0) {
                ms->mAllocated += it.second->mStats.mAllocated.load();
                ms->mLive += it.second->mStats.mLive.load();
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
        void* results[kStackTraceLimit];
        int ret = unw_backtrace(results, kStackTraceLimit);
        bool found = false;
        for (int i = 5; i < ret; i++) {
            for (auto& it : mData) {
                if ((intptr_t)results[i] >= (intptr_t)it.second->mAddr &&
                    (intptr_t)results[i] - (intptr_t)it.second->mAddr <
                            it.second->mLength) {
                    it.second->mStats.mAllocated += size;
                    it.second->mStats.mLive += size;
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
        void* results[kStackTraceLimit];
        int ret = unw_backtrace(results, kStackTraceLimit);
        bool found = false;
        for (int i = 5; i < ret; i++) {
            for (auto& it : mData) {
                if ((intptr_t)results[i] >= (intptr_t)it.second->mAddr &&
                    (intptr_t)results[i] - (intptr_t)it.second->mAddr <
                            it.second->mLength) {
                    it.second->mStats.mLive -= size;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
    }

    std::string printUsage() {
        std::stringstream ss;
        MallocStats stats;
        aggregate("EMUGL", &stats);
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
        ss << "EMUGL memory allocated: ";
        ss << (float)stats.mAllocated.load() / 1048576.0f;
        ss << "mb live: ";
        ss << (float)stats.mLive.load() / 1048576.0f;
        // ss << "mb peak: ";
        // ss << (float)stats.peak/1048576.0f;
        ss << "mb\n";
        //aggregate("EGL", &stats);
        //ss << "EGL memory allocated: ";
        //ss << (float)stats.mAllocated.load() / 1048576.0f;
        //ss << "mb live: ";
        //ss << (float)stats.mLive.load() / 1048576.0f;
        // ss << "mb peak: ";
        // ss << (float)stats.peak/1048576.0f;
        //ss << "mb\n";
        return ss.str();
    }

    static void new_hook(const void* ptr, size_t size) {
        MemoryUsage::get()->mImpl->newHook(ptr, size);
    }

    static void delete_hook(const void* ptr) {
        MemoryUsage::get()->mImpl->deleteHook(ptr);
    }

private:
    Lock mLock;
    std::unordered_map<std::string, FuncRange*> mData;
    static const int kStackTraceLimit = 32;

};

void MemoryUsage::Impl::start() {
    if(!MallocHook::AddNewHook(&new_hook) || !MallocHook::AddDeleteHook(&delete_hook)) {
        fprintf(stderr, "Failed to add malloc hooks. \n");
    }
}

void MemoryUsage::Impl::stop() {
    MallocHook::RemoveNewHook(&new_hook);
    MallocHook::RemoveDeleteHook(&delete_hook);
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
