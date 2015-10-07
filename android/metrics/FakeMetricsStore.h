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
#include "android/base/Compiler.h"
#include "android/metrics/MetricsStore.h"

#include <inttypes.h>
#include <unordered_map>
#include <string>

// Typical usage example:
//   #include "android/metrics/MetricsStore.h"
//
//   IMetricsStore*
namespace android {
namespace metrics {

// A fake metrics store for testing users of |IMetricsStore|.
// This fake implementation reneges on most promises of IMetricsStore:
// - There's not backing disk file.
// - All functions that have to do with persisting data, and some more, are
// no-ops.
class FakeMetricsStore : public IMetricsStore {
public:
    // Unlike its real counterpart, you _can_ directly instantiate a
    // FakeMetricsStore.
    FakeMetricsStore() {};
    bool Init() override { return true; }
    virtual ~FakeMetricsStore() = default;

    // These will blow up if a non-existent |key| is requested. So much the
    // better, for a fake object.
    std::string getString(const std::string& key,
                                 const std::string& otherwise) const override {
        if (mStringStore.find(key) != std::end(mStringStore)) {
            mStringStore.at(key);
        }
        return otherwise;
    }
    int getInt(const std::string& key,
                     int otherwise) const override {
        if (mIntStore.find(key) != std::end(mIntStore)) {
            return mIntStore.at(key);
        }
        return otherwise;
    }
    int64_t getInt64(const std::string& key,
                           int64_t otherwise) const override {
        if (mInt64Store.find(key) != std::end(mInt64Store)) {
            return mInt64Store.at(key);
        }
        return otherwise;
    }
    double getDouble(const std::string& key,
                           double otherwise) const {
        if (mDoubleStore.find(key) != std::end(mDoubleStore)) {
            return mDoubleStore.at(key);
        }
        return otherwise;
    }

    // Data setters that accept complete keys.
    void setString(const std::string& key, const std::string& value) override {
        mStringStore[key] = value;
    }
    void setInt(const std::string& key, int value) override {
        mIntStore[key] = value;
    }
    void setInt64(const std::string& key, int64_t value) override {
        mInt64Store[key] = value;
    }
    void setDouble(const std::string& key, double value) override {
        mDoubleStore[key] = value;
    }

    // Data update functions.
    void updateInt(const std::string& key, int delta, int otherwise) override {
        if (mIntStore.find(key) != std::end(mIntStore)) {
            mIntStore[key] += delta;
        } else {
            mIntStore[key] = otherwise + delta;
        }
    }
    void updateInt64(const std::string& key, int64_t delta, int64_t otherwise) override {
        if (mInt64Store.find(key) != std::end(mInt64Store)) {
            mInt64Store[key] += delta;
        } else {
            mInt64Store[key] = otherwise + delta;
        }
    }
    void updateDouble(const std::string& key, double delta, double otherwise) override {
        if (mDoubleStore.find(key) != std::end(mDoubleStore)) {
            mDoubleStore[key] += delta;
        } else {
            mDoubleStore[key] = otherwise + delta;
        }
    }

    // We simply bail on the following requests.
    // TODO(pprabhu): I'll probably be forced to implement this one.
    bool setDefaults(const std::unordered_map<std::string, std::string>& defaults) override {
        return false;
    }
    bool requestRefreshFromDisk() override {
        return false;
    }
    void requestFlushToDisk() override {}

private:
    std::unordered_map<std::string, std::string> mStringStore;
    std::unordered_map<std::string, int> mIntStore;
    std::unordered_map<std::string, int64_t> mInt64Store;
    std::unordered_map<std::string, double> mDoubleStore;

    DISALLOW_COPY_AND_ASSIGN(FakeMetricsStore);
};

}  // namespace metrics
}  // namespace android

