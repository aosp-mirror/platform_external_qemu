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
        mPtrs[(uint64_t)(uintptr_t)ptr] = ptr;
    }

    void removePtr(T* ptr) {
        android::base::AutoWriteLock lock(mLock);
        uint64_t handle = (uint64_t)(uintptr_t)ptr;
        mPtrs.erase(handle);
        mStalePtrs.erase(handle);
    }

    void remapStalePtr(uint64_t handle, T* newptr) {
        android::base::AutoWriteLock lock(mLock);
        mStalePtrs[handle] = newptr;
    }

    T* getPtr(uint64_t handle, bool removeFromStaleOnGet = false) {
        android::base::AutoReadLock lock(mLock);

        if (auto elt = android::base::find(mPtrs, handle))
            return *elt;

        auto it = mStalePtrs.find(handle);

        if (it == mStalePtrs.end())
            return nullptr;

        T* res = it->second;

        if (removeFromStaleOnGet) {
            lock.unlockRead();
            android::base::AutoWriteLock wrlock(mLock);
            mStalePtrs.erase(it);
        }

        return res;
    }

    void makeCurrentPtrsStale() {
        android::base::AutoWriteLock lock(mLock);
        for (const auto& it : mPtrs) {
            mStalePtrs[it.first] = it.second;
        }
        mPtrs.clear();
    }

    size_t numCurrEntries() {
        android::base::AutoReadLock lock(mLock);
        return mPtrs.size();
    }

    size_t numStaleEntries() {
        android::base::AutoReadLock lock(mLock);
        return mStalePtrs.size();
    }

    void onSave(android::base::Stream* stream) {
        android::base::AutoReadLock lock(mLock);
        saveCollection(
                stream, mStalePtrs,
                [](android::base::Stream* stream,
                   const std::pair<uint64_t, T*>& entry) {
                    stream->putBe64(entry.first);
                    stream->putBe64((uint64_t)(uintptr_t)entry.second);
                });
    }

    void onLoad(android::base::Stream* stream) {
        android::base::AutoWriteLock lock(mLock);
        loadCollection(
                stream, &mStalePtrs,
                [](android::base::Stream* stream) {
                    return std::make_pair(
                               stream->getBe64(),
                               (T*)(uintptr_t)stream->getBe64());
                });
    }
private:
    android::base::ReadWriteLock mLock; 
    std::unordered_map<uint64_t, T*> mPtrs;
    std::unordered_map<uint64_t, T*> mStalePtrs;
    DISALLOW_COPY_AND_ASSIGN(StalePtrRegistry);
};
