// Copyright 2018 The Android Open Source Project
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
#include <cutils/properties.h>

#include "AndroidHostCommon.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_map>
#include <string.h>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;

class AndroidSystemProperties {
public:
    AndroidSystemProperties() = default;
    ~AndroidSystemProperties() = default;

    int get(const char* key, char* value, const char* default_value) {
        AutoLock lock(mLock);
        auto it = mProps.find(key);
        if (it == mProps.end()) {
            if (!default_value) return 0;
            return getPropertyWithValueLimit(value, default_value);
        }
        const char* storedValue = it->second.data();
        return getPropertyWithValueLimit(value, storedValue);
    }

    int set(const char *key, const char *value) {
        AutoLock lock(mLock);
        mProps[key] = value;
        return 0;
    }

private:
    int getPropertyWithValueLimit(char* dst, const char* src) {
        size_t totalStrlen = strlen(src);
        size_t maxPropStrlen = PROPERTY_VALUE_MAX - 1;
        size_t copyStrlen =
            maxPropStrlen < totalStrlen ? maxPropStrlen : totalStrlen;
        memcpy(dst, src, copyStrlen);
        dst[copyStrlen] = '\0';
        return (int)copyStrlen;
    }

    Lock mLock;
    std::unordered_map<std::string, std::string> mProps;
};

static LazyInstance<AndroidSystemProperties> sProps = LAZY_INSTANCE_INIT;

extern "C" {

EXPORT int property_get(const char* key, char* value, const char* default_value) {
    return sProps->get(key, value, default_value);
}

EXPORT int property_set(const char *key, const char *value) {
    return sProps->set(key, value);
}

} // extern "C"
