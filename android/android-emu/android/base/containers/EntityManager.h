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

#include "android/base/containers/Lookup.h"
#include "android/base/Optional.h"

#include <functional>
#include <unordered_map>
#include <vector>

#include <inttypes.h>
#include <stdio.h>

#define ENTITY_MANAGER_DEBUG 0

#if ENTITY_MANAGER_DEBUG

#define EM_DBG(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);

#else
#define EM_DBG(...)
#endif

#define INVALID_ENTITY_HANDLE 0
#define INVALID_COMPONENT_HANDLE 0

namespace android {
namespace base {

// EntityManager: A way to represent an abstrat space of objects with handles.
// Each handle is associated with data of type Item for quick access from handles to data.
// Otherwise, entity data is spread through ComponentManagers.
template<size_t indexBits,
         size_t generationBits,
         size_t typeBits,
         class Item>
class EntityManager {
public:

    static_assert(64 == (indexBits + generationBits + typeBits),
                  "bits of index, generation, and type must add to 64");

    using EntityHandle = uint64_t;
    using IteratorFunc = std::function<void(bool live, EntityHandle h, Item& item)>;

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

    EntityHandle add(const Item& item, size_t type) {

        if (!type) return INVALID_ENTITY_HANDLE;

        size_t maxElements = (1ULL << indexBits);
        if (mLiveEntries == maxElements) return INVALID_ENTITY_HANDLE;

        size_t newIndex = mFirstFreeIndex;

        EM_DBG("newIndex/firstFree: %zu type: %zu", newIndex, type);

        size_t neededCapacity = newIndex + 1;
        if (maxElements < neededCapacity) return INVALID_ENTITY_HANDLE;

        size_t currentCapacity = mEntries.size();
        size_t nextCapacity = neededCapacity << 1;
        if (nextCapacity > maxElements) nextCapacity = maxElements;

        EM_DBG("needed/current/next capacity: %zu %zu %zu",
               neededCapacity,
               currentCapacity,
               nextCapacity);

        if (neededCapacity > mEntries.size()) {
            mEntries.resize(nextCapacity);
            for (size_t i = currentCapacity; i < nextCapacity; ++i) {
                mEntries[i].handle = makeHandle(i, 0, type);
                mEntries[i].nextFreeIndex = i + 1;
                EM_DBG("new un-init entry: index %zu nextFree %zu",
                       i, i + 1);
            }
        }

        mEntries[newIndex].handle =
            makeHandle(newIndex, mEntries[newIndex].liveGeneration, type);
        mEntries[newIndex].item = item;

        mFirstFreeIndex = mEntries[newIndex].nextFreeIndex;
        EM_DBG("created. new first free: %zu", mFirstFreeIndex);

        ++mLiveEntries;

        EM_DBG("result handle: 0x%llx", (unsigned long long)mEntries[newIndex].handle);

        return mEntries[newIndex].handle;
    }

    EntityHandle addFixed(EntityHandle fixedHandle, const Item& item, size_t type) {
        // 3 cases:
        // 1. handle is not allocated and doesn't correspond to mFirstFreeIndex
        bool isFreeListNonHead = false;
        // 2. handle already exists (replace)
        bool isAlloced = false;
        // 3. index(handle) == mFirstFreeIndex
        bool isFreeListHead = false;

        if (!type) return INVALID_ENTITY_HANDLE;

        size_t maxElements = (1ULL << indexBits);
        if (mLiveEntries == maxElements) return INVALID_ENTITY_HANDLE;

        size_t newIndex = getHandleIndex(fixedHandle);

        EM_DBG("newIndex/firstFree: %zu type: %zu", newIndex, type);

        size_t neededCapacity = newIndex + 1;

        if (maxElements < neededCapacity) return INVALID_ENTITY_HANDLE;

        size_t currentCapacity = mEntries.size();
        size_t nextCapacity = neededCapacity << 1;
        if (nextCapacity > maxElements) nextCapacity = maxElements;

        EM_DBG("needed/current/next capacity: %zu %zu %zu",
               neededCapacity,
               currentCapacity,
               nextCapacity);

        if (neededCapacity > mEntries.size()) {
            mEntries.resize(nextCapacity);
            for (size_t i = currentCapacity; i < nextCapacity; ++i) {
                mEntries[i].handle = makeHandle(i, 0, type);
                mEntries[i].nextFreeIndex = i + 1;
                EM_DBG("new un-init entry: index %zu nextFree %zu",
                       i, i + 1);
            }
        }

        // Now we ensured that there is enough space to talk about the entry of
        // this |fixedHandle|.
        if (mFirstFreeIndex == newIndex) {
            isFreeListHead = true;
        } else {
            auto& entry = mEntries[newIndex];
            if (entry.liveGeneration == getHandleGeneration(entry.handle)) {
                isAlloced = true;
            } else {
                isFreeListNonHead = true;
            }
        }

        mEntries[newIndex].handle = fixedHandle;
        mEntries[newIndex].liveGeneration = getHandleGeneration(fixedHandle);
        mEntries[newIndex].item = item;

        EM_DBG("new index: %zu", newIndex);

        if (isFreeListHead) {

            EM_DBG("first free index reset from %zu to %zu",
                    mFirstFreeIndex, mEntries[newIndex].nextFreeIndex);

            mFirstFreeIndex = mEntries[newIndex].nextFreeIndex;

            ++mLiveEntries;

        } else if (isAlloced) {
            // Already replaced whatever is there, and since it's already allocated,
            // no need to update freelist.
            EM_DBG("entry at %zu already alloced. replacing.", newIndex);
        } else if (isFreeListNonHead) {
            // Go through the freelist and skip over the entry we just added.
            size_t prevEntryIndex = mFirstFreeIndex;

            EM_DBG("in free list but not head. reorganizing freelist. "
                   "start at %zu -> %zu",
                   mFirstFreeIndex, mEntries[prevEntryIndex].nextFreeIndex);

            while (mEntries[prevEntryIndex].nextFreeIndex != newIndex) {
                EM_DBG("next: %zu -> %zu",
                       prevEntryIndex,
                       mEntries[prevEntryIndex].nextFreeIndex);
                prevEntryIndex =
                    mEntries[prevEntryIndex].nextFreeIndex;
            }

            EM_DBG("finished. set prev entry %zu to new entry's next, %zu",
                    prevEntryIndex, mEntries[newIndex].nextFreeIndex);

            mEntries[prevEntryIndex].nextFreeIndex =
                mEntries[newIndex].nextFreeIndex;

            ++mLiveEntries;
        }

        return fixedHandle;
    }
    void remove(EntityHandle h) {

        if (get(h) == nullptr) return;

        size_t index = getHandleIndex(h);

        EM_DBG("remove handle: 0x%llx -> index %zu", (unsigned long long)h, index);

        auto& entry = mEntries[index];

        EM_DBG("handle gen: %zu entry gen: %zu", getHandleGeneration(h), entry.liveGeneration);

        ++entry.liveGeneration;
        if ((entry.liveGeneration == 0) ||
            (entry.liveGeneration == (1ULL << generationBits))) {
            entry.liveGeneration = 1;
        }

        entry.nextFreeIndex = mFirstFreeIndex;

        mFirstFreeIndex = index;

        EM_DBG("new first free: %zu next free: %zu", mFirstFreeIndex, entry.nextFreeIndex);

        --mLiveEntries;
    }

    Item* get(EntityHandle h) {
        size_t index = getHandleIndex(h);
        if (index >= mEntries.size()) return nullptr;

        auto& entry = mEntries[index];
        if (entry.liveGeneration != getHandleGeneration(h)) return nullptr;

        return &entry.item;
    }

    bool isLive(EntityHandle h) const {
        size_t index = getHandleIndex(h);
        if (index >= mEntries.size()) return false;

        const auto& entry = mEntries[index];

        return (entry.liveGeneration == getHandleGeneration(h));
    }

    void forEachEntry(IteratorFunc func) {
        for (auto& entry: mEntries) {
            auto handle = entry.handle;
            bool live = isLive(handle);
            auto& item = entry.item;
            func(live, handle, item);
        }
    }

    void forEachLiveEntry(IteratorFunc func) {
        for (auto& entry: mEntries) {
            auto handle = entry.handle;
            bool live = isLive(handle);

            if (!live) continue;

            auto& item = entry.item;
            func(live, handle, item);
        }
    }

private:
    EntityManager(size_t initialItems) :
        mEntries(initialItems),
        mFirstFreeIndex(0),
        mLiveEntries(0) { }

    std::vector<EntityEntry> mEntries;
    size_t mFirstFreeIndex;
    size_t mLiveEntries;
};

// Tracks components over a given space of entities.
// Looking up by entity index is slower, but takes less space overall versus
// a flat array that parallels the entities.
template<size_t indexBits,
         size_t generationBits,
         size_t typeBits,
         class Data>
class ComponentManager {
public:

    static_assert(64 == (indexBits + generationBits + typeBits),
                  "bits of index, generation, and type must add to 64");

    using ComponentHandle = uint64_t;
    using EntityHandle = uint64_t;
    using ComponentIteratorFunc = std::function<void(bool, ComponentHandle componentHandle, EntityHandle entityHandle, Data& data)>;

    // Adds the given |data| and associates it with EntityHandle.
    // We can also opt-in to immediately tracking the handle in the reverse mapping,
    // which has an upfront cost in runtime.
    // Many uses of ComponentManager don't really need to track the associated entity handle,
    // so it is opt-in.

    ComponentHandle add(
        EntityHandle h,
        const Data& data,
        size_t type,
        bool tracked = false) {

        InternalItem item = { h, data, tracked };
        auto res = static_cast<ComponentHandle>(mData.add(item, type));

        if (tracked) {
            mEntityToComponentMap[h] = res;
        }

        return res;
    }

    void clear() {
        mData.clear();
        mEntityToComponentMap.clear();
    }

    // If we didn't explicitly track, just fail.
    ComponentHandle getComponentHandle(EntityHandle h) const {
        auto componentHandlePtr = android::base::find(mEntityToComponentMap, h);
        if (!componentHandlePtr) return INVALID_COMPONENT_HANDLE;
        return *componentHandlePtr;
    }

    EntityHandle getEntityHandle(ComponentHandle h) const {
        return mData.get(h)->entityHandle;
    }

    void removeByEntity(EntityHandle h) {
        auto componentHandle = getComponentHandle(h);
        removeByComponent(componentHandle);
    }

    void removeByComponent(ComponentHandle h) {
        auto item = mData.get(h);

        if (!item) return;
        if (item->tracked) {
            mEntityToComponentMap.erase(item->entityHandle);
        }

        mData.remove(h);
    }

    Data* getByEntity(EntityHandle h) {
        return getByComponent(getComponentHandle(h));
    }

    Data* getByComponent(ComponentHandle h) {
        auto item = mData.get(h);
        if (!item) return nullptr;
        return &(item->data);
    }

    void forEachComponent(ComponentIteratorFunc func) {
        mData.forEachEntry(
            [func](bool live, typename InternalEntityManager::EntityHandle componentHandle, InternalItem& item) {
                func(live, componentHandle, item.entityHandle, item.data);
        });
    }

    void forEachLiveComponent(ComponentIteratorFunc func) {
        mData.forEachLiveEntry(
            [func](bool live, typename InternalEntityManager::EntityHandle componentHandle, InternalItem& item) {
                func(live, componentHandle, item.entityHandle, item.data);
        });
    }

private:
    struct InternalItem {
        EntityHandle entityHandle;
        Data data;
        bool tracked;
    };

    using InternalEntityManager = EntityManager<indexBits, generationBits, typeBits, InternalItem>;
    using EntityToComponentMap = std::unordered_map<EntityHandle, ComponentHandle>;

    mutable InternalEntityManager mData;
    EntityToComponentMap mEntityToComponentMap;
};

// ComponentManager, but unpacked; uses the same index space as the associated
// entities. Takes more space by default, but not more if all entities have this component.
template<size_t indexBits,
         size_t generationBits,
         size_t typeBits,
         class Data>
class UnpackedComponentManager {
public:
    using ComponentHandle = uint64_t;
    using EntityHandle = uint64_t;
    using ComponentIteratorFunc =
        std::function<void(bool, ComponentHandle componentHandle, EntityHandle entityHandle, Data& data)>;

    EntityHandle add(EntityHandle h, const Data& data) {

        size_t index = indexOfEntity(h);

        if (index + 1 > mItems.size()) {
            mItems.resize((index + 1) * 2);
        }

        mItems[index].live = true;
        mItems[index].handle = h;
        mItems[index].data = data;

        return h;
    }

    void clear() {
        mItems.clear();
    }

    void remove(EntityHandle h) {
        size_t index = indexOfEntity(h);
        mItems[index].live = false;
        // no-op
    }

    Data* get(EntityHandle h) {
        size_t index = indexOfEntity(h);

        if (index + 1 > mItems.size()) {
            mItems.resize((index + 1) * 2);
        }

        auto item = mItems.data() + indexOfEntity(h);
        if (!item->live) return nullptr;
        return &item->data;
    }

    void forEachComponent(ComponentIteratorFunc func) {
        for (auto& item : mItems) {
            func(item.live, item.handle, item.handle, item.data);
        }
    }

    void forEachLiveComponent(ComponentIteratorFunc func) {
        for (auto& item : mItems) {
            if (item.live) func(item.live, item.handle, item.handle, item.data);
        }
    }

private:
    size_t indexOfEntity(EntityHandle h) {
        return EntityManager<indexBits, generationBits, typeBits, int>::getHandleIndex(h);
    }

    struct InternalItem {
        bool live = false;
        EntityHandle handle = 0;
        Data data;
    };

    std::vector<InternalItem> mItems;
};

} // namespace android
} // namespace base
