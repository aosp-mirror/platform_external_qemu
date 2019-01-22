// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/Compiler.h"

#include "android/base/FunctionView.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_map>

namespace android {
namespace base {

// Static map class for use with LazyInstance or in global structures
// as a process-wide registry of something. Safe for concurrent accress.
template <class K, class V>
class StaticMap {
public:
    StaticMap() = default;

    void set(const K& key, const V& value) {
        AutoLock lock(mLock);
        mItems.emplace(key, value);
    }

    void erase(const K& key) {
        AutoLock lock(mLock);
        mItems.erase(key);
    }

    bool isPresent(const K& key) const {
        AutoLock lock(mLock);
        auto it = mItems.find(key);
        return it != mItems.end();
    }

    android::base::Optional<V> get(const K& key) const {
        AutoLock lock(mLock);
        auto it = mItems.find(key);
        if (it == mItems.end()) {
            return android::base::kNullopt;
        }
        return it->second;
    }

    using ErasePredicate = android::base::FunctionView<bool(K, V)>;

    void eraseIf(ErasePredicate p) {
        AutoLock lock(mLock);
        auto it = mItems.begin();
        for (; it != mItems.end();) {
            if (p(it->first, it->second)) {
                it = mItems.erase(it);
            } else {
                ++it;
            }
        }
    }

    void clear() {
        AutoLock lock(mLock);
        mItems.clear();
    }
private:
    using AutoLock = android::base::AutoLock;
    using Lock = android::base::Lock;

    mutable android::base::Lock mLock;
    std::unordered_map<K, V> mItems;

    DISALLOW_COPY_ASSIGN_AND_MOVE(StaticMap);
};

} // namespace base
} // namespace android
