/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <algorithm>
#include <list>
#include <map>

namespace android {
namespace base {

// A trivial MRU cache. Not thread-safe.
template <class K, class V>
class MruCache {
public:
    template <class S>
    struct EntryWithSize {
        EntryWithSize(const S&& d) : EntryWithSize(std::move(d), 0) {}

        EntryWithSize(const S&& d, const size_t ds)
            : data(std::move(d)), dataSize(ds) {
            static_assert(
                    std::is_same<S, K>::value || std::is_same<S, V>::value,
                    "Cache entry instantiated with invalid types");
        }

        const S data;
        size_t dataSize;

        bool operator==(const EntryWithSize& rhs) const {
            return data == rhs.data;
        }
        bool operator<(const EntryWithSize& rhs) const {
            return data < rhs.data;
        }
    };

    class MruCacheObserver {
    public:
        virtual void cacheChanged() = 0;
        virtual ~MruCacheObserver() {}
    };

    class CacheFlattener {
    public:
        virtual void handleFlatten(
                std::map<EntryWithSize<K>, EntryWithSize<V>>& mCache,
                void* buf,
                size_t bufSize) = 0;
        virtual ~CacheFlattener() {}
    };

    MruCache(size_t maxEntries, CacheFlattener* cacheFlattener)
        : mMaxEntries(maxEntries), mCacheFlattener(cacheFlattener) {}

    bool put(const K& key, size_t keySize, V&& value, size_t valueSize) {
        evictIfNecessary();

        EntryWithSize<K> cacheKey(std::move(key), keySize);
        EntryWithSize<V> cacheValue(std::move(value), valueSize);

        auto exists = mCache.find(cacheKey);
        if (exists != mCache.end()) {
            auto iter = std::find(mMostRecents.begin(), mMostRecents.end(),
                                  cacheKey);
            mMostRecents.splice(mMostRecents.begin(), mMostRecents, iter);
            mCache.erase(exists);
        } else {
            mMostRecents.push_front(cacheKey);
        }

        const auto [_, res] = mCache.insert({cacheKey, std::move(cacheValue)});

        if (mCacheObserver != nullptr && res) {
            mCacheObserver->cacheChanged();
        }

        return res;
    }

    void flatten(void* buf, size_t bufSize) {
        if (mCacheFlattener) {
            mCacheFlattener->handleFlatten(mCache, buf, bufSize);
        }
    }

    bool get(const K& key, const V** value) {
        EntryWithSize<K> cacheKey(std::move(key));
        auto res = mCache.find(cacheKey);

        if (res == mCache.end()) {
            return false;
        }

        *value = &(res->second.data);
        auto iter =
                std::find(mMostRecents.begin(), mMostRecents.end(), cacheKey);
        mMostRecents.splice(mMostRecents.begin(), mMostRecents, iter);

        return true;
    }

    void setObserver(MruCacheObserver* observer) { mCacheObserver = observer; }

private:
    using MruCacheMap = std::map<EntryWithSize<K>, EntryWithSize<V>>;
    using MostRecentList = std::list<EntryWithSize<K>>;

    void evictIfNecessary() {
        auto entryCount = mMostRecents.size();
        if (entryCount >= mMaxEntries) {
            auto threshold = entryCount * 0.9;

            for (int i = mMostRecents.size(); i > threshold; i--) {
                EntryWithSize<K> key = mMostRecents.front();
                mMostRecents.pop_front();
                mCache.erase(key);
            }
        }
    }

    MruCacheMap mCache;
    MostRecentList mMostRecents;
    MruCacheObserver* mCacheObserver;
    CacheFlattener* mCacheFlattener;
    const size_t mMaxEntries;
};

}  // namespace base
}  // namespace android
