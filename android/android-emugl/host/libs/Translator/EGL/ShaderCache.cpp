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
// 3200 is ~32MB of shaders, very rough estimate.
android::base::MruCache<std::vector<uint8_t>, std::vector<uint8_t>> mruCache(
        3200);
}  // namespace

void SetBlob(const void* key,
             EGLsizeiANDROID keySize,
             const void* value,
             EGLsizeiANDROID valueSize) {
    std::vector<uint8_t> keyVec(keySize);
    memcpy(keyVec.data(), key, keySize);

    std::vector<uint8_t> valueVec(valueSize);
    memcpy(valueVec.data(), value, valueSize);

    mruCache.put(keyVec, std::move(valueVec));
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
