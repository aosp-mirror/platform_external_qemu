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

#include "android/base/memory/MemoryTracker.h"

#include "android/base/memory/LazyInstance.h"

#if defined(__linux__) || defined(__APPLE__)
#include <malloc_extension_c.h>
#include <malloc_hook.h>

#include <execinfo.h>
#include <libunwind.h>
#include <sstream>
#include <unordered_map>
#include <vector>
#endif

using android::base::LazyInstance;

#define E(fmt, ...) fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__);

namespace android {
namespace base {

struct MallocStats {
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

class MemoryTracker::Impl {
public:
#if defined(__linux__) || defined(__APPLE__)
    bool addToGroup(const std::string& group, const std::string& func) {
        if (!enabled)  return false;

        std::string key = group + func;
        if (mData.find(group + func) != mData.end()) {
            return false;
        }
        unw_cursor_t cursor;
        unw_context_t uc;
        unw_getcontext(&uc);
        unw_init_local(&cursor, &uc);
        /* We step for 3 times because, in <GLcommon/GLESmacros.h>, addToGroup()
        is invoked by MEM_TRACE_IF using std::call_once().
        This approach to reduce unnecessary checks and potential
        race conditions. The typical backtrace looks like:
        10c121b2c (android::base::MemoryTracker::addToGroup()
        10c0cebb6 (void std::__1::__call_once_proxy<std::__1::tuple<>>(void*))
        7fff4e9f336e (std::__1::__call_once(unsigned long volatile&, void*, void (*)(void*)))
        10c0cc489 (eglClientWaitSyncKHR)+89
        where eglClientWaitSyncKHR is what we want to register. Do not change the
        number of times unw_step() unless the implementation for MEM_TRACE_IF in
        <GLcommon/GLESmacros.h> has changed.
        */
        unw_step(&cursor);
        unw_step(&cursor);
        unw_step(&cursor);
        unw_proc_info_t info;
        if (0 != unw_get_proc_info(&cursor, &info)) {
            return false;
        }
        mData.emplace(key, new FuncRange{func, (void*)info.start_ip,
                                         info.end_ip - info.start_ip});
        return true;
    }

    void aggregate(std::string label, MallocStats* ms) {
        for (auto& it : mData) {
            if (it.first.compare(0, label.size(), label) == 0) {
                ms->mAllocated += it.second->mStats.mAllocated.load();
                ms->mLive += it.second->mStats.mLive.load();
            }
        }
    }

    void newHook(const void* ptr, size_t size) {
        size = std::max(size, MallocExtension_GetAllocatedSize(ptr));
        void* results[kStackTraceLimit];
#if defined(__linux__)
        int ret = unw_backtrace(results, kStackTraceLimit);
#else
        int ret = backtrace(results, kStackTraceLimit);
#endif
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
#if defined(__linux__)
        int ret = unw_backtrace(results, kStackTraceLimit);
#else
        int ret = backtrace(results, kStackTraceLimit);
#endif
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

    std::string printUsage(int verbosity) {
        std::stringstream ss;
        if (!enabled){
            ss << "Memory tracker not enabled\n";
            return ss.str();
        }

        MallocStats stats;
        aggregate("EMUGL", &stats);
        ss << "EMUGL memory allocated: ";
        ss << (float)stats.mAllocated.load() / 1048576.0f;
        ss << "mb live: ";
        ss << (float)stats.mLive.load() / 1048576.0f;
        ss << "mb\n";
        // If verbose, return the memory stats for each registered functions
        // sorted by live memory
        if (verbosity) {
            std::vector<FuncRange*> allStats;
            for (auto& it : mData) {
                allStats.push_back(it.second);
            }
            std::sort(allStats.begin(), allStats.end(),
                      [](FuncRange* a, FuncRange* b) {
                          return std::abs(a->mStats.mLive) >
                                 std::abs(b->mStats.mLive);
                      });
            for (auto it : allStats) {
                ss << it->mName + " memory allocated: ";
                ss << (float)it->mStats.mAllocated.load() / 1048576.0f;
                ss << "mb live: ";
                ss << (float)it->mStats.mLive.load() / 1048576.0f;
                ss << "mb\n";
            }
        }

        return ss.str();
    }

    static void new_hook(const void* ptr, size_t size) {
        MemoryTracker::get()->mImpl->newHook(ptr, size);
    }

    static void delete_hook(const void* ptr) {
        MemoryTracker::get()->mImpl->deleteHook(ptr);
    }

    void start() {
        if (!MallocHook::AddNewHook(&new_hook) ||
            !MallocHook::AddDeleteHook(&delete_hook)) {
            E("Failed to add malloc hooks.");
            enabled = false;
        } else {
            enabled = true;
        }
    }

    void stop() {
        if (enabled) {
            MallocHook::RemoveNewHook(&new_hook);
            MallocHook::RemoveDeleteHook(&delete_hook);
        }
    }

private:
    bool enabled = false;
    std::unordered_map<std::string, FuncRange*> mData;
    static const int kStackTraceLimit = 32;
#else
    bool addToGroup(std::string group, std::string func) {
        E("Not implemented");
        return false;
    }

    void start() { E("Not implemented"); }

    void stop() { E("Not implemented"); }

    std::string printUsage(int verbosity) {
        return "<memory usage tracker not implemented>";
    }

#endif
};

MemoryTracker::MemoryTracker() : mImpl(new MemoryTracker::Impl()) {}

bool MemoryTracker::addToGroup(const std::string& group,
                             const std::string& func) {
    return mImpl->addToGroup(group, func);
}

std::string MemoryTracker::printUsage(int verbosity) {
    return mImpl->printUsage(verbosity);
}

void MemoryTracker::start() {
    mImpl->start();
}

void MemoryTracker::stop() {
    mImpl->stop();
}

static LazyInstance<MemoryTracker> sMemoryTracker = LAZY_INSTANCE_INIT;

// static
MemoryTracker* MemoryTracker::get() {
#if defined(__linux__) || defined(__APPLE__)
    return sMemoryTracker.ptr();
#else
    return nullptr;
#endif
}

}  // namespace base
}  // namespace android
