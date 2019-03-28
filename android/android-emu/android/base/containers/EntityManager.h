// Copyright (C) 2019 The Android Open Source Project
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

#include "android/base/Optional.h"

#include <vector>

#include <inttypes.h>

namespace android {
namespace base {

template<size_t indexBits,
         size_t generationBits,
         size_t typeBits,
         class Item>
class EntityManager {
public:
    using EntityHandle = uint64_t;

    static size_t getHandleIndex(EntityHandle h) {
        return static_cast<size_t>(h & ((1ULL << indexBits) - 1ULL));
    }

    static size_t getHandleGeneration(EntityHandle h) {
        return static_cast<size_t>(
            (h >> indexBits) &
            ((1ULL << generationBits) - 1ULL));
    }

    static size_t getHandleType(EntityHandle h) {
        return static_cast<size_t>(
            (h >> (indexBits + generationBits)) &
            ((1ULL << typeBits) - 1ULL));
        return h & ((1ULL << indexBits) - 1ULL);
    }

    static EntityHandle makeHandle(
        size_t index,
        size_t generation,
        size_t type) {
        EntityHandle res = index;
        res |= generation << indexBits;
        res |= type << (indexBits + generationBits);
        return res;
    }

    static EntityHandle withIndex(EntityHandle h, size_t i) {
        return makeHandle(i, getHandleGeneration(h), getHandleType(h));
    }

    static EntityHandle withGeneration(EntityHandle h, size_t nextGen) {
        return makeHandle(getHandleIndex(h), nextGen, getHandleType(h));
    }

    static EntityHandle withType(EntityHandle h, size_t newType) {
        return makeHandle(getHandleIndex(h), getHandleGeneration(h), newType);
    }

    EntityManager() : EntityManager(0) { }

    EntityManager(size_t initialItems) :
        mEntries(initialItems),
        mFirstFreeIndex(0),
        mLiveEntries(0) { }

    ~EntityManager() { clear(); }

    struct EntityEntry {
        EntityHandle handle = 0;
        size_t nextFreeIndex = 0;
        // 0 is a special generation for brand new entries
        // that are not used yet
        size_t liveGeneration = 1;
        Item item;
    };

    void clear() {
        mEntries.clear();
        mFirstFreeIndex = 0;
        mLiveEntries = 0;
    }

    EntityHandle add(Item item, size_t type) {
        size_t newIndex = mFirstFreeIndex;

        size_t neededCapacity = newIndex + 1;
        size_t currentCapacity = mEntries.size();
        size_t nextCapacity = neededCapacity * 2;

        if (neededCapacity > mEntries.size()) {
            mEntries.resize(nextCapacity);
            for (size_t i = currentCapacity; i < nextCapacity; ++i) {
                mEntries[i].handle = makeHandle(i, 0, type);
                mEntries[i].nextFreeIndex = i + 1;
            }
        }

        mEntries[newIndex].handle =
            makeHandle(newIndex, mEntries[newIndex].liveGeneration, type);

        mFirstFreeIndex = mEntries[newIndex].nextFreeIndex;

        ++mLiveEntries;

        return mEntries[newIndex].handle;
    }

    void remove(EntityHandle h) {
        size_t index = getHandleIndex(h);
        auto& entry = mEntries[index];

        ++entry.liveGeneration;
        if ((entry.liveGeneration + 1 == 0) ||
            (entry.liveGeneration == (1ULL << generationBits))) {
            entry.liveGeneration = 1;
        }

        entry.nextFreeIndex = mFirstFreeIndex;

        mFirstFreeIndex = index;

        --mLiveEntries;
    }

    Item* get(EntityHandle h) {
        size_t index = getHandleIndex(h);
        if (index >= mEntries.size()) return nullptr;

        auto& entry = mEntries[index];
        if (entry.liveGeneration != getHandleGeneration(h)) return nullptr;

        return &entry.item;
    }

private:

    std::vector<EntityEntry> mEntries;
    size_t mFirstFreeIndex;
    size_t mLiveEntries;
};

} // namespace android
} // namespace base


