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
    explicit MruCache(size_t maxEntries) : mMaxEntries(maxEntries) {}

    bool put(const K& key, V&& value) {
        evictIfNecessary();
        auto exists = mCache.find(key);
        if (exists != mCache.end()) {
            auto iter =
                    std::find(mMostRecents.begin(), mMostRecents.end(), key);
            mMostRecents.splice(mMostRecents.begin(), mMostRecents, iter);
            mCache.erase(exists);
        } else {
            mMostRecents.push_front(key);
        }

        auto res = mCache.insert({key, std::forward<V>(value)});
        return true;
    }

    bool get(const K& key, const V** value) {
        auto res = mCache.find(key);
        if (res == mCache.end()) {
            return false;
        }

        *value = &res->second;
        auto iter = std::find(mMostRecents.begin(), mMostRecents.end(), key);
        mMostRecents.splice(mMostRecents.begin(), mMostRecents, iter);
        return true;
    }

private:
    using MruCacheMap = std::map<K, V>;
    using MostRecentList = std::list<K>;

    void evictIfNecessary() {
        auto entryCount = mMostRecents.size();
        if (entryCount >= mMaxEntries) {
            auto threshold = entryCount * 0.9;

            for (int i = mMostRecents.size(); i > threshold; i--) {
                K key = mMostRecents.front();
                mMostRecents.pop_front();
                mCache.erase(key);
            }
        }
    }

    MruCacheMap mCache;
    MostRecentList mMostRecents;
    const size_t mMaxEntries;
};

}  // namespace base
}  // namespace android
