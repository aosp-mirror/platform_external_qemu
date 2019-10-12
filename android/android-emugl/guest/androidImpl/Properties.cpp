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

    Lock mutex;
    std::unordered_map<std::string, std::string> props;
};

static LazyInstance<AndroidSystemProperties> sProps = LAZY_INSTANCE_INIT;

extern "C" {

EXPORT int property_get(const char* key, char* value, const char* default_value) {
    auto props = sProps.ptr();

    AutoLock lock(props->mutex);

    auto it = props->props.find(key);

    if (it == props->props.end()) {

        if (!default_value) return 0;

        size_t totalStrlen = strlen(default_value);
        size_t maxPropStrlen = PROPERTY_VALUE_MAX - 1;
        size_t copyStrlen =
            maxPropStrlen < totalStrlen ? maxPropStrlen : totalStrlen;
        memcpy(value, default_value, copyStrlen);
        value[copyStrlen] = '\0';
        return copyStrlen;
    }

    const char* storedValue = it->second.data();
    size_t totalStrlen = it->second.size();
    size_t maxPropStrlen = PROPERTY_VALUE_MAX - 1;
    size_t copyStrlen =
        maxPropStrlen < totalStrlen ? maxPropStrlen : totalStrlen;
    memcpy(value, storedValue, copyStrlen);
    value[copyStrlen] = '\0';

    return (int)copyStrlen;
}

EXPORT int property_set(const char *key, const char *value) {
    auto props = sProps.ptr();
    AutoLock lock(props->mutex);

    props->props[key] = value;

    return 0;
}

} // extern "C"
