// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "android/base/containers/Lookup.h"
#include "android/base/containers/EntityManager.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_map>

namespace android {
namespace base {

template <size_t maxIndex,
          class IndexType, // must be castable to uint64_t
          class Data>
class HybridEntityManager {
public:
    using EM = EntityManager<32, 16, 16, Data>;
    using IterFunc = typename EM::IteratorFunc;
    using ConstIterFunc = typename EM::ConstIteratorFunc;
    using Handle = typename EM::EntityHandle;

    HybridEntityManager() : mEntityManager(maxIndex + 1) { }
    ~HybridEntityManager() { clear(); }

    uint64_t add(const Data& data, size_t type) {
        uint64_t nextIndex = 0;
        {
            SeqLock::ScopedWrite sw(&mEntityManagerLock);
            nextIndex = (uint64_t)mEntityManager.nextFreeIndex();
            if (nextIndex < maxIndex) {
                uint64_t resultHandle = mEntityManager.add(data, type);
                if (EM::getHandleIndex(resultHandle) != nextIndex) {
                    dfatal("%s: fatal: handle indices mismatch. wanted 0x%llx got 0x%llx", __func__,
                            (unsigned long long)nextIndex,
                            (unsigned long long)EM::getHandleIndex(resultHandle));
                    abort();
                }
                return resultHandle;
            }
        }

        AutoLock lock(mMapLock);
        if (mIndexForMap == 0) {
            mIndexForMap = maxIndex;
        }
        nextIndex = mIndexForMap;
        mMap[nextIndex] = data;
        ++mIndexForMap;
        return EM::makeHandle(nextIndex, 1, type);
    }

    uint64_t addFixed(IndexType index, const Data& data, size_t type) {
        uint64_t index_u64 = (uint64_t)EM::getHandleIndex(index);
        if (index_u64 < maxIndex) {
            SeqLock::ScopedWrite sw(&mEntityManagerLock);
            return mEntityManager.addFixed(index, data, type);
        } else {
            AutoLock lock(mMapLock);
            // Fixed allocations require us to update mIndexForMap to catch up with it.
            // We assume that addFixed/add for the map case do not interleave badly
            // (addFixed at i, add at i+1, addFixed at i+1)
            mIndexForMap = index_u64;
            mMap[index_u64] = data;
            ++mIndexForMap;
            return index;
        }
    }

    void clear() {
        {
            SeqLock::ScopedWrite sw(&mEntityManagerLock);
            mEntityManager.clear();
        }
        {
            AutoLock lock(mMapLock);
            mMap.clear();
            mIndexForMap = 0;
        }
    }

    void remove(IndexType index) {
        uint64_t index_u64 = (uint64_t)EM::getHandleIndex(index);
        if (index_u64 < maxIndex) {
            SeqLock::ScopedWrite sw(&mEntityManagerLock);
            mEntityManager.remove(index);
        } else {
            AutoLock lock(mMapLock);
            mMap.erase(index_u64);
        }
    }

    Data* get(IndexType index) {
        uint64_t index_u64 = (uint64_t)EM::getHandleIndex(index);
        if (index_u64 < maxIndex) {
            SeqLock::ScopedWrite sw(&mEntityManagerLock);
            return mEntityManager.get(index);
        } else {
            AutoLock lock(mMapLock);
            auto res = android::base::find(mMap, index_u64);
            return res;
        }
    }

    const Data* get_const(IndexType index) const {
        uint64_t index_u64 = (uint64_t)EM::getHandleIndex(index);
        if (index_u64 < maxIndex) {
            const Data* res;
            AEMU_SEQLOCK_READ_WITH_RETRY(&mEntityManagerLock,
                res = mEntityManager.get_const(index));
            return res;
        } else {
            AutoLock lock(mMapLock);
            auto res = android::base::find(mMap, index_u64);
            return res;
        }
    }

    Data* getExceptZero(IndexType index) {
        Data* res = get(index);
        if (!res) return nullptr;
        if (!(*res)) return nullptr;
        return res;
    }

    const Data* getExceptZero_const(IndexType index) const {
        const Data* res = get_const(index);
        if (!res) return nullptr;
        if (!(*res)) return nullptr;
        return res;
    }

    void forEach(IterFunc func) {
        {
            SeqLock::ScopedWrite sw(&mEntityManagerLock);
            mEntityManager.forEach(func);
        }

        AutoLock lock(mMapLock);
        for (auto it : mMap) {
            auto handle = index2Handle(it.first);
            func(true /* live */, handle, handle, it.second);
        }
    }

    void forEachLive(IterFunc func) {
        {
            SeqLock::ScopedWrite sw(&mEntityManagerLock);
            mEntityManager.forEachLiveComponent(func);
        }

        AutoLock lock(mMapLock);
        for (auto it : mMap) {
            auto handle = index2Handle(it.first);
            func(true /* live */, handle, handle, it.second);
        }
    }

    void forEachLive_const(ConstIterFunc func) const {
        mEntityManager.forEachLiveEntry_const([this, func](bool live, Handle h, Data& d) {
            AEMU_SEQLOCK_READ_WITH_RETRY(
                &mEntityManagerLock,
                func(live, h, d));
        });

        AutoLock lock(mMapLock);
        for (const auto it : mMap) {
            auto handle = index2Handle(it.first);
            func(true /* live */, handle, handle, it.second);
        }
    }

private:
    static Handle index2Handle(uint64_t index) {
        return EM::makeHandle((uint32_t)index, 1, 1);
    }

    EM mEntityManager;
    std::unordered_map<IndexType, Data> mMap;
    uint64_t mIndexForMap = 0;
    mutable SeqLock mEntityManagerLock;
    mutable Lock mMapLock;
};

} // namespace android
} // namespace base
