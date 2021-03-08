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

#include <unordered_map>

namespace android {
namespace base {

template <size_t maxIndex,
          class IndexType, // must be castable to uint64_t
          class Data>
class HybridComponentManager {
public:
    using UCM = UnpackedComponentManager<32, 16, 16, Data>;
    using EM = EntityManager<32, 16, 16, Data>;
    using IterFunc = typename UCM::ComponentIteratorFunc;
    using ConstIterFunc = typename UCM::ConstComponentIteratorFunc;
    using Handle = typename EM::EntityHandle;

    void add(IndexType index, const Data& data) {
        uint64_t index_u64 = (uint64_t)index;
        if (index_u64 < maxIndex) {
            auto internal_handle = index2Handle(index_u64);
            mComponentManager.add(internal_handle, data);
        } else {
            mMap[index] = data;
        }

    }

    void clear() {
        mComponentManager.clear();
        mMap.clear();
    }

    void remove(IndexType index) {
        uint64_t index_u64 = (uint64_t)index;
        if (index_u64 < maxIndex) {
            auto internal_handle = index2Handle(index_u64);
            mComponentManager.remove(internal_handle);
        } else {
            mMap.erase(index);
        }
    }

    Data* get(IndexType index) {
        uint64_t index_u64 = (uint64_t)index;
        if (index_u64 < maxIndex) {
            auto internal_handle = index2Handle(index_u64);
            return mComponentManager.get(internal_handle);
        } else {
            return android::base::find(mMap, index);
        }
    }

    const Data* get_const(IndexType index) const {
        uint64_t index_u64 = (uint64_t)index;
        if (index_u64 < maxIndex) {
            auto internal_handle = index2Handle(index_u64);
            return mComponentManager.get_const(internal_handle);
        } else {
            return android::base::find(mMap, index);
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
        mComponentManager.forEach(func);

        for (auto it : mMap) {
            auto handle = index2Handle(it.first);
            func(true /* live */, handle, handle, it.second);
        }
    }

    void forEachLive(IterFunc func) {
        mComponentManager.forEachLiveComponent(func);

        for (auto it : mMap) {
            auto handle = index2Handle(it.first);
            func(true /* live */, handle, handle, it.second);
        }
    }

    void forEachLive_const(ConstIterFunc func) const {
        mComponentManager.forEachLiveComponent_const(func);

        for (const auto it : mMap) {
            auto handle = index2Handle(it.first);
            func(true /* live */, handle, handle, it.second);
        }
    }

private:
    static Handle index2Handle(uint64_t index) {
        return EM::makeHandle((uint32_t)index, 1, 1);
    }

    UCM mComponentManager;
    std::unordered_map<IndexType, Data> mMap;
};

} // namespace android
} // namespace base
