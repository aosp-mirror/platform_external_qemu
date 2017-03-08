/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include "android/base/containers/Lookup.h"
#include "android/base/files/Stream.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/Compiler.h"

#include <algorithm>
#include <unordered_map>

#include <inttypes.h>

// The purpose of StalePtrRegistry is to track integers corresponding to
// host-side pointers that may be invalidated after snapshots.
template <class T>
class StalePtrRegistry {
public:
    StalePtrRegistry() = default;

    void addPtr(T* ptr) {
        android::base::AutoWriteLock lock(mLock);
        mPtrs[(uint64_t)(uintptr_t)ptr] = { ptr, Staleness::Live };
    }

    void removePtr(T* ptr) {
        android::base::AutoWriteLock lock(mLock);
        uint64_t handle = (uint64_t)(uintptr_t)ptr;
        mPtrs.erase(handle);
    }

    void remapStalePtr(uint64_t handle, T* newptr) {
        android::base::AutoWriteLock lock(mLock);
        mPtrs[handle] = { newptr, Staleness::PrevSnapshot };
    }

    T* getPtr(uint64_t handle, bool removeFromStaleOnGet = false) {
        android::base::AutoReadLock lock(mLock);

        T* res = defaultPtr;

        Entry* it = nullptr;

        if ((it = android::base::find(mPtrs, handle)))
            res = it->ptr;

        if (removeFromStaleOnGet &&
            it && it->staleness == Staleness::PrevSnapshot) {
            lock.unlockRead();
            android::base::AutoWriteLock wrlock(mLock);
            mPtrs.erase(handle);
        }

        return res;
    }

    void makeCurrentPtrsStale() {
        android::base::AutoWriteLock lock(mLock);
        for (auto& it : mPtrs) {
            it.second.staleness =
                Staleness::PrevSnapshot;
        }
    }

    size_t numCurrEntries() const {
        return countWithStaleness(Staleness::Live);
    }

    size_t numStaleEntries() const {
        return countWithStaleness(Staleness::PrevSnapshot);
    }

    void onSave(android::base::Stream* stream) {
        android::base::AutoReadLock lock(mLock);
        saveCollection(
                stream, mPtrs,
                [](android::base::Stream* stream,
                   const std::pair<uint64_t, Entry>& entry) {
                    stream->putBe64(entry.first);
                    stream->putBe64((uint64_t)(uintptr_t)entry.second.ptr);
                });
    }

    void onLoad(android::base::Stream* stream) {
        android::base::AutoWriteLock lock(mLock);
        loadCollection(
                stream, &mPtrs,
                [](android::base::Stream* stream) {
                    uint64_t handle = stream->getBe64();
                    T* ptr = (T*)(uintptr_t)stream->getBe64();
                    return std::make_pair(
                               handle,
                               (Entry){ ptr, Staleness::PrevSnapshot });
                });
    }
private:
    enum Staleness {
        Live,
        PrevSnapshot,
    };
    struct Entry {
        T* ptr;
        Staleness staleness;
    };

    size_t countWithStaleness(Staleness check) const {
        android::base::AutoReadLock lock(mLock);
        return std::count_if(mPtrs.begin(), mPtrs.end(),
                   [check](const std::pair<uint64_t, Entry>& entry) {
                       return entry.second.staleness == check;
                   });
    }

    mutable android::base::ReadWriteLock mLock;
    std::unordered_map<uint64_t, Entry> mPtrs;

    DISALLOW_COPY_AND_ASSIGN(StalePtrRegistry);
};
