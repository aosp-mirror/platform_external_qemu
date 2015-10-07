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

#include "android/metrics/MetricsStoreFactory.h"

#include "android/metrics/FakeMetricsStore.h"

namespace android {
namespace metrics {

using android::base::Looper;
using std::string;

IMetricsStore* MetricsStoreFactory::createMetricsStore(const string& filePath,
                                                       Looper* looper, const
                                                       string& keyPrefix) {
    return nullptr;
}

IMetricsStore* MetricsStoreFactory::createDefaultMetricsStore(Looper* looper,
                                                              const string&
                                                              keyPrefix) {
    return nullptr;
}

IMetricsStore* MetricsStoreFactory::createFakeMetricsStore() {
    return new FakeMetricsStore();
}

string MetricsStoreFactory::getStoreDirectory() {
    return "";
}

string MetricsStoreFactory::getDefaultStoreFilePath() {
    return "";
}

}  // namespace metrics
}  // namespace android
