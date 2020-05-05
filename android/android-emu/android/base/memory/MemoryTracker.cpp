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

#ifndef AEMU_TCMALLOC_ENABLED
#error "Need to know whether we enabled TCMalloc!"
#endif

#if AEMU_TCMALLOC_ENABLED && defined(__linux__)
#include <malloc_extension_c.h>
#include <malloc_hook.h>

#include <execinfo.h>
#include <libunwind.h>
#include <algorithm>
#include <set>
#include <sstream>
#include <vector>
#endif

using android::base::LazyInstance;

#define E(fmt, ...) fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__);

namespace android {
namespace base {

struct FuncRange {
    std::string mName;
    intptr_t mAddr;
    size_t mLength;
    MemoryTracker::MallocStats mStats;
};

class MemoryTracker::Impl {
public:
#if AEMU_TCMALLOC_ENABLED && defined(__linux__)
    Impl()
        : mData([](const FuncRange* a, const FuncRange* b) {
              return a->mAddr + a->mLength < b->mAddr + b->mLength;
          }) {}

    bool addToGroup(const std::string& group, const std::string& func) {
        std::string key = group + func;
        if (mRegisterFuncs.find(group + func) != mRegisterFuncs.end()) {
            return false;
        }

        unw_cursor_t cursor;
        unw_context_t uc;
        unw_getcontext(&uc);
        unw_init_local(&cursor, &uc);
        /* We step for 3 times because, in <GLcommon/GLESmacros.h>, addToGroup()
         * is invoked by MEM_TRACE_IF using std::call_once().
         * This approach to reduce unnecessary checks and potential
         * race conditions. The typical backtrace looks like:
         * 10c121b2c (android::base::MemoryTracker::addToGroup()
         * 10c0cebb6 (void
         * std::__1::__call_once_proxy<std::__1::tuple<>>(void*)) 7fff4e9f336e
         * (std::__1::__call_once(unsigned long volatile&, void*, void
         * (*)(void*))) 10c0cc489 (eglClientWaitSyncKHR)+89 where
         * eglClientWaitSyncKHR is what we want to register. Do not change the
         * number of times unw_step() unless the implementation for MEM_TRACE_IF
         * in <GLcommon/GLESmacros.h> has changed.
         */
        unw_step(&cursor);
        unw_step(&cursor);
        unw_step(&cursor);
        unw_proc_info_t info;
        if (0 != unw_get_proc_info(&cursor, &info)) {
            return false;
        }
        mRegisterFuncs.insert(key);
        mData.insert(new FuncRange{key, (intptr_t)info.start_ip,
                                   info.end_ip - info.start_ip});

        return true;
    }

    void newHook(const void* ptr, size_t size) {
        size = std::max(size, MallocExtension_GetAllocatedSize(ptr));
        void* results[kStackTraceLimit];
#if defined(__linux__)
        int ret = unw_backtrace(results, kStackTraceLimit);
#endif
        for (int i = 5; i < ret; i++) {
            intptr_t addr = (intptr_t)results[i];
            /*
             * The invariant is that all registered functions will be sorted
             * based on the starting address. We can use binary search to test
             * if an address falls within a specific function and reduce the
             * look up time.
             */
            FuncRange func{"", addr, 0};
            auto it = mData.upper_bound(&func);
            if (it != mData.end() && (*it)->mAddr <= addr) {
                (*it)->mStats.mAllocated += size;
                (*it)->mStats.mLive += size;
                break;
            }
        }
    }

    void deleteHook(const void* ptr) {
        size_t size = MallocExtension_GetAllocatedSize(ptr);
        void* results[kStackTraceLimit];
#if defined(__linux__)
        int ret = unw_backtrace(results, kStackTraceLimit);
#endif
        for (int i = 5; i < ret; i++) {
            intptr_t addr = (intptr_t)results[i];
            FuncRange func{"", addr, 0};
            auto it = mData.upper_bound(&func);
            if (it != mData.end() && (*it)->mAddr <= addr) {
                (*it)->mStats.mLive -= size;
                break;
            }
        }
    }

    std::string printUsage(int verbosity) {
        std::stringstream ss;
        if (!enabled){
            ss << "Memory tracker not enabled\n";
            return ss.str();
        }

        auto stats = getUsage("EMUGL");
        ss << "EMUGL memory allocated: ";
        ss << (float)stats->mAllocated.load() / 1048576.0f;
        ss << "mb live: ";
        ss << (float)stats->mLive.load() / 1048576.0f;
        ss << "mb\n";
        // If verbose, return the memory stats for each registered functions
        // sorted by live memory
        if (verbosity) {
            std::vector<FuncRange*> allStats;
            for (auto it : mData) {
                allStats.push_back(it);
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

    bool isEnabled() { return enabled; }

    std::unique_ptr<MallocStats> getUsage(const std::string& group) {
        std::unique_ptr<MallocStats> ms(new MallocStats());
        for (auto& it : mData) {
            if (it->mName.compare(0, group.size(), group) == 0) {
                ms->mAllocated += it->mStats.mAllocated.load();
                ms->mLive += it->mStats.mLive.load();
            }
        }
        return std::move(ms);
    }

private:
    bool enabled = false;
    std::set<FuncRange*, bool (*)(const FuncRange*, const FuncRange*)> mData;
    std::set<std::string> mRegisterFuncs;
    static const int kStackTraceLimit = 32;
#else
    bool addToGroup(std::string group, std::string func) {
        (void)group;
        (void)func;
        return false;
    }

    void start() { E("Not implemented"); }

    void stop() { E("Not implemented"); }

    std::string printUsage(int verbosity) {
        return "<memory usage tracker not implemented>";
    }

    bool isEnabled() { return false; }

    std::unique_ptr<MallocStats> getUsage(const std::string& group) {
        return nullptr;
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

bool MemoryTracker::isEnabled() {
    return mImpl->isEnabled();
}

std::unique_ptr<MemoryTracker::MallocStats> MemoryTracker::getUsage(
        const std::string& group) {
    return mImpl->getUsage(group);
}

static LazyInstance<MemoryTracker> sMemoryTracker = LAZY_INSTANCE_INIT;

// static
MemoryTracker* MemoryTracker::get() {
#if defined(__linux__)
    return sMemoryTracker.ptr();
#else
    return nullptr;
#endif
}

}  // namespace base
}  // namespace android
