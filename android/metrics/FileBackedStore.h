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

#include "android/metrics/MetricsStore.h"

#include "android/base/Compiler.h"

#include <inttypes.h>
#include <string>
#include <unordered_map>

namespace android {
namespace metrics {

class FileBackedStore : public IMetricsStore {
public:
    virtual ~FileBackedStore() = default;

    // All heavy lifting for initialization happens here.
    bool Init() override;

    // Data accessors.
    std::string getString(const std::string& key, const std::string& otherwise) const override;
    int getInt(const std::string& key, int otherwise) const override;
    int64_t getInt64(const std::string& key, int64_t otherwise) const override;
    double getDouble(const std::string& key, double otherwise) const override;

    // Data setters.
    void setString(const std::string& key, const std::string& value) override;
    void setInt(const std::string& key, int value) override;
    void setInt64(const std::string& key, int64_t value) override;
    void setDouble(const std::string& key, double value) override;

    // Data update functions.
    void updateInt(const std::string& key, int delta, int otherwise) override;
    void updateInt64(const std::string& key, int64_t delta, int64_t otherwise) override;
    void updateDouble(const std::string& key, double delta, double otherwise) override;

    bool setDefaults(const std::unordered_map<std::string, std::string>& defaults) override;
    bool requestRefreshFromDisk() override;
    void requestFlushToDisk() override;

protected:
    // Use android::metrics::MetricsStoreFactory for creating instances.
    FileBackedStore() = default;

    // Data conversion functions.
    std::string toString(int value);
    std::string toString(int64_t value);
    std::string toString(double value);
    int fromString(std::string value, int otherwise);
    int64_t fromString(std::string value, int64_t otherwise);
    double fromString(std::string value, double otherwise);

    // Used to synchronize access to the backing file.
    bool lockBackingFile();
    bool releaseBackingFile();

private:
    // All values are stored as std::string to ease reading/writing from the
    // backing file.
    // This is our current view of the backing file.
    std::unordered_map<std::string, std::string> persistentData;
    // These are direct overwrites request to the persistent data (throguh the
    // |set*| functions.
    std::unordered_map<std::string, std::string> overwriteData;
    // These are updates requested on top of the persistent data (through the
    // |update*| functions.
    std::unordered_map<std::string, std::string> updateData;
    // These are stashed away |otherwise| values from an |update*| call.
    // We'll use these to replace the value from the backing store when flushing
    // the update *only if that value is corrupted when we try to flush*.
    std::unordered_map<std::string, std::string> overwriteIfCorruptedData;

    DISALLOW_COPY_AND_ASSIGN(FileBackedStore);
};

}  // namespace metrics
}  // namespace android
