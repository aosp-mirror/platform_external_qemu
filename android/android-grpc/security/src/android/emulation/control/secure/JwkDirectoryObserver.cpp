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
#include "android/emulation/control/secure/JwkDirectoryObserver.h"

#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/logging/CLog.h"
#include "aemu/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...)                                                     \
    printf("JwkDirectoryObserver: %s:%d| " fmt "\n", __func__, __LINE__, \
           ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {

using json = nlohmann::json;
using base::PathUtils;
using base::System;

JwkDirectoryObserver::JwkDirectoryObserver(Path jwksDir,
                                           KeysetUpdatedCallback callback,
                                           PathFilterPredicate filter,
                                           bool startImmediately)
    : mPathFilter(filter), mJwkPath(jwksDir), mCallback(callback) {
    mWatcher = FileSystemWatcher::getFileSystemWatcher(
            jwksDir,
            [=](auto change, auto path) { fileChangeHandler(change, path); });

    if (startImmediately) {
        if (!start()) {
            dfatal("Unable to start observing %s, jwks will not be updated. "
                   "%d were keysets loaded.",
                   jwksDir.c_str(), mLoadedKeys.size());
        }
    }
}

JwkDirectoryObserver::~JwkDirectoryObserver() {
    stop();
}

bool JwkDirectoryObserver::start() {
    bool expected = false;
    if (!mRunning.compare_exchange_strong(expected, true)) {
        return false;
    }
    // Initial scan..
    scanJwkPath();
    notifyKeysetUpdated();
    return mWatcher ? mWatcher->start() : false;
}

void JwkDirectoryObserver::scanJwkPath() {
    mLoadedKeys.clear();
    dinfo("Scanning %s for jwk keys.", mJwkPath.c_str());
    for (auto strPath : System::get()->scanDirEntries(mJwkPath.c_str(), true)) {
        auto status = mLoadedKeys.add(strPath);
        if (!status.ok()) {
            derror("Failed add jwk key: %s, due to: %s, access will be "
                   "denied to this provider and the file deleted.",
                   strPath.c_str(), status.message().data());
            System::get()->deleteFile(strPath);
        }
    };
}

void JwkDirectoryObserver::stop() {
    bool expected = true;
    if (!mRunning.compare_exchange_strong(expected, false)) {
        mWatcher->stop();
    }

    mLoadedKeys.clear();
}

void JwkDirectoryObserver::fileChangeHandler(
        FileSystemWatcher::WatcherChangeType change,
        Path path) {
    DD("Filechange handler %d for %s", change, path.c_str());
    if (!mPathFilter(path)) {
        // Ignore files of the given type.
        DD("Ignoring %s", path.c_str());
        return;
    }

    switch (change) {
        case FileSystemWatcher::WatcherChangeType::Created:
            [[fallthrough]];
        case FileSystemWatcher::WatcherChangeType::Changed: {
            DD("Changed/Created event for: %s", path.c_str());

            // Wait at most 1 second for non-empty files
            auto status = mLoadedKeys.addWithRetryForEmpty(
                    path, 8, std::chrono::milliseconds(125));
            if (!status.ok()) {
                derror("Failed add jwk key: %s, due to: %s, access will be "
                       "denied to this provider and the file deleted.",
                       path.c_str(), status.message().data());
                System::get()->deleteFile(path);
                return;
            }
            VERBOSE_INFO(grpc,
                         "Added JSON Web Key Sets from %s, %d keys loaded",
                         path.c_str(), mLoadedKeys.size());
            break;
        }
        case FileSystemWatcher::WatcherChangeType::Deleted:
            DD("Deleted %s", path.c_str());
            auto status = mLoadedKeys.remove(path);
            if (!status.ok()) {
                // This usually means it is already deleted.
                derror("Failed to remove jwk key: %s, due to: %s", path.c_str(),
                       status.message().data());
                return;
            }
            VERBOSE_INFO(grpc,
                         "Removed JSON Web Key Sets from %s, %d keys loaded",
                         path.c_str(), mLoadedKeys.size());
    };

    notifyKeysetUpdated();
}

static absl::Status notify(JwkDirectoryObserver::KeysetUpdatedCallback callback,
                           const JwkKeyLoader& loader) {
    if (loader.empty()) {
        callback(nullptr);
        return absl::OkStatus();
    }

    auto keyset = loader.activeKeySet();
    if (keyset.ok()) {
        callback(std::move(*keyset));
        return absl::OkStatus();
    }

    return keyset.status();
}

void JwkDirectoryObserver::notifyKeysetUpdated() {
    auto notified = notify(mCallback, mLoadedKeys);

    // Notification failed, lets see if we can rescan and update
    // our keyset..
    if (!notified.ok()) {
        derror("Failed construct jwk keyset due to: %s, rescanning.",
               notified.message().data());
        scanJwkPath();
        notify(mCallback, mLoadedKeys).IgnoreError();
    }
}

bool JwkDirectoryObserver::acceptJwkExtOnly(Path path) {
    return android::base::EndsWith(path, kJwkExt);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
