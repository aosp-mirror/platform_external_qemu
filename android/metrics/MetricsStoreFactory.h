// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/async/Looper.h"
#include "android/metrics/FakeMetricsStore.h"
#include "android/metrics/FileBackedStore.h"
#include "android/metrics/MetricsStore.h"

#include <string>

namespace android {
namespace metrics {

class MetricsStoreFactory {
public:
    // |fileName| is the full path of the file backing the store.
    // The IMetricsStore |get*| and |set*| methods prefix all key searches with
    // the |keyPrefix|. Use empty string to not use prefixes.
    static IMetricsStore* createMetricsStore(
            const std::string& filePath,
            android::base::Looper* looper,
            const std::string& keyPrefix);

    // Returns an instance of the default metrics store for the current process.
    static IMetricsStore* createDefaultMetricsStore(
            android::base::Looper* looper,
            const std::string& keyPrefix);

    // Returns an instance of a fake metrics store.
    static IMetricsStore* createFakeMetricsStore();

    // Extra methods to get information about the backing files for stores.
    static std::string getStoreDirectory();
    static std::string getDefaultStoreFilePath();
};

}  // namespace metrics
}  // namespace android
