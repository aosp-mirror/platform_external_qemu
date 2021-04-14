// Copyright (C) 2021 The Android Open Source Project
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

#include "ShaderCache.h"

#include <cstdio>
#include <cstring>
#include "android/base/MruCache.h"

#include <map>
#include <vector>

namespace {
using BlobCacheType = std::vector<uint8_t>;

template <class K, class V>
class CacheObserver : android::base::MruCache<K, V>::MruCacheObserver {
public:
    int count = 0;
    CacheObserver(android::base::MruCache<K, V>* cacheReference) {
        mCacheRef = cacheReference;
        cacheReference->setObserver(this);
    }

    void cacheChanged() override {
        // TODO: This will dictate when we flatten the cache.
    }

private:
    android::base::MruCache<K, V>* mCacheRef;
};

template <class K, class V>
class CacheFlattener : public android::base::MruCache<K, V>::CacheFlattener {
    using MruCache = std::map<
            typename android::base::MruCache<K, V>::template EntryWithSize<K>,
            typename android::base::MruCache<K, V>::template EntryWithSize<V>>;

public:
    void handleFlatten(MruCache& mCache, void* buf, size_t bufSize) {
        // TODO: This will handle the actual serialization.
    }
};

CacheFlattener<BlobCacheType, BlobCacheType> testFlattener;
// 3200 is ~32MB of shaders, very rough estimate.
android::base::MruCache<BlobCacheType, BlobCacheType> mruCache(3200,
                                                               &testFlattener);
CacheObserver<BlobCacheType, BlobCacheType> sss(&mruCache);
}  // namespace

void SetBlob(const void* key,
             EGLsizeiANDROID keySize,
             const void* value,
             EGLsizeiANDROID valueSize) {
    std::vector<uint8_t> keyVec(keySize);
    memcpy(keyVec.data(), key, keySize);

    std::vector<uint8_t> valueVec(valueSize);
    memcpy(valueVec.data(), value, valueSize);

    mruCache.put(keyVec, keySize, std::move(valueVec), valueSize);
}

EGLsizeiANDROID GetBlob(const void* key,
                        EGLsizeiANDROID keySize,
                        void* value,
                        EGLsizeiANDROID valueSize) {
    std::vector<uint8_t> keyVec(keySize);
    memcpy(keyVec.data(), key, keySize);

    const std::vector<uint8_t>* result;
    auto found = mruCache.get(keyVec, &result);

    if (!found) {
        return 0;
    }

    if (result->size() <= static_cast<size_t>(valueSize)) {
        memcpy(value, result->data(), result->size());
    }

    // If the size provided was too small, return the right size regardless.
    return result->size();
}
