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

#include "android/base/Compiler.h"

#include <inttypes.h>
#include <string>
#include <unordered_map>

// This is a simple key-value store for metrics data.
// The salient features are:
// - The key-value store is backed by a disk file, but the users never need to
//   be concerned about the backing file beyond providing a valid path for the
//   file.
// - The key-value store data is regularly flushed to the backing disk file. The
//   store itself decides the appropriate time for persisting data. In addition,
//   it tries hard to persist the data when the object is destroyed.
// - This is multi-processing safe: Another independent process trying to
//   reference the same backing file via an IMetricsStore will fail to do so.
// - This is thread safe wrt. access to the backing store.
//   - It correctly handles updates to the backing store from other instances of
//     the store (in the same or other store) both wrt refreshing and flushing
//     data.
// - It's API is __NOT__ thread-safe. If you call |setString| on a single
//   instance from two separate threads, behaviour is undefined.
// - Has typed values. Keys are string.

// Typical usage:
//   #include "android/metrics/MetricsStore.h"
//   #include "android/metrics/MetricsStoreFactory.h"
//
//   std::uniqe_ptr<IMetricsStore> mstore =
//          MetricsStoreFactory::createDefaultMetricsStore(
//                  looper_getForCurrentThread(), "stupid.metrics.");
//   if (mstore == nullptr || !mstore->Init()) {
//       // Error handing code.
//   }
//
//   // At this point, |mstore| is ready to use. It may or may not have read the
//   backing store, but we're sure that our process owns the backing store and
//   can safely write to it.
//
//   // Maybe set some default values we have stashed away.
//   if (!mstore->setDefaults(kMyDefaultsMap)) {
//       // We could not set defaults, continue?
//   }
//
//   int stash = mstore->getInt("many_many_keys");
//   mstore->updateInt("many_many_keys", 4);
//   if (mstore->getInt("many_many_keys") == stash + 4) {
//       // This is __not__ always true, in a multi-threaded world!
//   }
//
//   // This will flush all values to disk.
//   mstore.reset();
//
namespace android {
namespace metrics {

class IMetricsStore {
public:

    virtual ~IMetricsStore() = default;

    // All heavy lifting for initialization happens here.
    virtual bool Init() = 0;

    // Data accessors. |otherwise| is returned if the key doesn't exist in the
    // store yet, or if the corresponding value is badly formatted.
    virtual std::string getString(const std::string& key, const std::string&
                                  otherwise) const = 0;
    virtual int getInt(const std::string& key, int otherwise) const = 0;
    virtual int64_t getInt64(const std::string& key, int64_t otherwise) const =
            0;
    virtual double getDouble(const std::string& key, double otherwise) const =
            0;
    // Overloads with obvious defaults.
    std::string getString(const std::string& key) const {
        return getString(key, "");
    }
    int getInt(const std::string& key) const {
        return getInt(key, 0);
    }
    int64_t getInt64(const std::string& key) const {
        return getInt64(key, 0);
    }
    double getDouble(const std::string& key) const {
        return getDouble(key, 0.0);
    }


    // Data setters.
    virtual void setString(const std::string& key, const std::string& value) = 0;
    virtual void setInt(const std::string& key, int value) = 0;
    virtual void setInt64(const std::string& key, int64_t value) = 0;
    virtual void setDouble(const std::string& key, double value) = 0;

    // Data update functions.
    // Prefer these functions to |set*| functions for real types when you want
    // to update a value. The store does not (can not) guarantee fresh value for
    // |get*| functions. So, it's not thread safe to say:
    //     setInt("key", getInt("key") + 4);
    // Instead, this is safer (and easier):
    //     updateInt("key", 4);  // Guarantees increment by 4.
    //
    // If the key doesn't exist in the store, or if the value in the store is
    // badly formatted, |otherwise| is used as the base value to begin with.
    virtual void updateInt(const std::string& key, int delta, int otherwise) = 0;
    virtual void updateInt64(const std::string& key, int64_t delta, int64_t otherwise) = 0;
    virtual void updateDouble(const std::string& key, double delta, double otherwise) = 0;
    // Overloads with obvious defaults.
    void updateInt(const std::string& key, int delta) {
        return updateInt(key, delta, 0);
    }
    void updateInt64(const std::string& key, int64_t delta) {
        return updateInt64(key, delta, 0);
    }
    void updateDouble(const std::string& key, double delta) {
        return updateDouble(key, delta, 0.0);
    }


    // This function is useful for setting default values en-masse.
    // It's a simplistic setter -- all values are accepted as strings.
    // It will first refresh the store from the backing disk file, and only
    // assign to keys that don't already exist. See |requestRefreshFromDisk|.
    // All caveats stated there apply.
    // Returns true if defaults were set, false in case of failure. In particular,
    // the call may fail if we were unable to read the backing disk file in a
    // reasonable amount of time.
    virtual bool setDefaults(const std::unordered_map<std::string, std::string>& defaults) = 0;

    // Refresh the store from the backing persistent representation.
    // - This action is blocking, but with a reasonable timeout.
    // - This never overwrites updates queued to the store through its API.
    // - The refresh is atomic -- it will never read a competing store's update
    //   halfway.
    // Returns true if the refresh was successfull, false otherwise. In
    // particular the call may fail if we were unable to read the backing disk
    // file in a reasonable amount of time.
    virtual bool requestRefreshFromDisk() = 0;

    // Request that current contents of the store be flushed to disk.
    // - This action is asynchronous.
    // - This action is best-effort. The attempt may fail or time out.
    // - You should usually __not__ have to call this function manually.
    virtual void requestFlushToDisk() = 0;

protected:
    // Use android::metrics::MetricsStoreFactory for creating instances.
    IMetricsStore() = default;

private:
    DISALLOW_COPY_AND_ASSIGN(IMetricsStore);
};

}  // namespace metrics
}  // namespace android
