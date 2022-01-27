// Copyright (C) 2022 The Android Open Source Project
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
#pragma once
#include <atomic>                                  // for atomic_bool
#include <functional>                              // for function
#include <memory>                                  // for unique_ptr
#include <string>                                  // for operator==, string
#include <string_view>                             // for string_view
#include <unordered_map>                           // for unordered_map

#include "absl/status/statusor.h"                  // for StatusOr
#include "android/base/files/FileSystemWatcher.h"  // for FileSystemWatcher
#include "nlohmann/json.hpp"                       // for json

namespace crypto {
namespace tink {
class KeysetHandle;
}  // namespace tink
}  // namespace crypto

namespace android {
namespace emulation {
namespace control {

using Path = std::string;
using base::FileSystemWatcher;
using crypto::tink::KeysetHandle;

// A Jwk Directory Observer observes the given directory for JSON Web Key files.
// A JSON Web Key (JWK) is a JavaScript Object Notation (JSON) data
// structure that represents a cryptographic key.
// (see: https://datatracker.ietf.org/doc/html/rfc7517)
//
// The JwkDirectoryObserver will read all the .jwk files in a directory and
// construct a Tink keyset handle that is a combination of all the keys.
//
// - A callback will be invoked after creation, once all .jwk files have been
//   processed.
// - A callback will be invoked when a .jwk file is added, removed or changed.
//
// Some things to note:
//
// - Only public keys will be handled.
// - Only files with the extension .jwk will be taken in consideration.
// - KID (Key IDentifiers) should be unique accross all files.
class JwkDirectoryObserver {
public:
    using json = nlohmann::json;
    using KeysetUpdatedCallback =
            std::function<void(std::unique_ptr<KeysetHandle>)>;

    using PathFilterPredicate = std::function<bool(Path)>;

    // Observe the given directory for jwk files, calling
    // the given callback when keyset changes are detected.
    // Set startImmediately to true if start should be called.
    //
    // \jwksDir| Path to observe for file changes
    // \callback| callback to invoke whenever we detect changes
    // |filter| Filter to use, when this predicate return true the file will be processed.
    // |startImmediately| True if we immediately should attemp to observe file changes.
    //
    // Note: setting startImmediately to true means you will not detect failures in the watcher.
    JwkDirectoryObserver(Path jwksDir,
                         KeysetUpdatedCallback callback,
                         PathFilterPredicate filter = JwkDirectoryObserver::acceptJwkExtOnly,
                         bool startImmediately = true);
    ~JwkDirectoryObserver() = default;

    // Start observing the directory structure.
    // This will invoke the callback if any files are present in the
    // directory that is being observed.
    //
    // Returns false if we are unable to observe the directory.
    bool start();

    // Stops observing the directory. No new events will be delivered.
    void stop();

private:
    static bool acceptJwkExtOnly(Path path);
    void fileChangeHandler(FileSystemWatcher::WatcherChangeType change,
                           Path path);

    void constructHandleAndNotify();
    std::string mergeKeys();
    static ::absl::StatusOr<json> extractKeys(Path file);

    PathFilterPredicate mPathFilter;
    std::unordered_map<Path, json> mPublicKeys;
    Path mJwkPath;
    KeysetUpdatedCallback mCallback;
    std::unique_ptr<FileSystemWatcher> mWatcher;
    std::atomic_bool mRunning{false};


    static constexpr const std::string_view kJwkExt{".jwk"};
};

}  // namespace control
}  // namespace emulation
}  // namespace android
