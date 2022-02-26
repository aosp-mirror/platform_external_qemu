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

#include <stdint.h>                         // for uint8_t
#include <fstream>                          // for basic_istream, basic_stre...
#include <initializer_list>                 // for initializer_list
#include <iterator>                         // for istreambuf_iterator
#include <tuple>                            // for tuple_element<>::type
#include <utility>                          // for move
#include <vector>                           // for vector

#include "absl/status/status.h"             // for Status, InternalError
#include "absl/strings/str_format.h"        // for StrFormat
#include "absl/strings/string_view.h"       // for string_view
#include "android/base/files/PathUtils.h"   // for PathUtils
#include "android/base/misc/StringUtils.h"  // for EndsWith
#include "android/base/system/System.h"     // for System
#include "android/utils/debug.h"            // for VERBOSE_INFO, VERBOSE_grpc
#include "tink/jwt/jwk_set_converter.h"     // for JwkSetToPublicKeysetHandle
#include "tink/util/statusor.h"             // for StatusOr

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
using base::System;
using base::PathUtils;

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
            dwarning(
                    "Unable to start observing %s, jwks will not be updated. "
                    "%d were keysets loaded.",
                    jwksDir.c_str(), mPublicKeys.size());
        }
    }
}

bool JwkDirectoryObserver::start() {
    bool expected = false;
    if (!mRunning.compare_exchange_strong(expected, true)) {
        return false;
    }
    // Initial scan..
    mPublicKeys.clear();
    for (auto strPath : System::get()->scanDirEntries(mJwkPath.c_str(), true)) {
        auto maybeJson = extractKeys(strPath);
        if (maybeJson.ok()) {
            mPublicKeys[strPath] = maybeJson.value();
        }
    };

    constructHandleAndNotify();
    return mWatcher ? mWatcher->start() : false;
}

void JwkDirectoryObserver::stop() {
    mWatcher->stop();
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
            DD("Change/Created event for: %s", path.c_str());
            auto maybeJson = extractKeys(path);
            if (!maybeJson.ok()) {
                derror("Failed to extract keys: %s",
                       maybeJson.status().message().data());
                return;
            }
            mPublicKeys[path] = maybeJson.value();
            break;
        }
        case FileSystemWatcher::WatcherChangeType::Deleted:
            DD("Deleted %s (in set: %s)", path.c_str(),
               mPublicKeys.count(path) ? "yes" : "no");
            if (mPublicKeys.erase(path)) {
                VERBOSE_INFO(grpc, "Removed JSON Web Key Sets from %s",
                             path.c_str());
            }
    };

    constructHandleAndNotify();
}
void JwkDirectoryObserver::constructHandleAndNotify() {
    DD("Have %lu keys.", mPublicKeys.size());
    if (mPublicKeys.empty()) {
        // mCallback(absl::NotFoundError(
        //         absl::StrFormat("No public keys found in %s", mJwkPath)));
        mCallback(nullptr);
        return;
    }
    // Create a valid JWK snippet with all the available keys.
    auto jsonKeys = mergeKeys();
    auto keyHandle = crypto::tink::JwkSetToPublicKeysetHandle(jsonKeys);
    if (!keyHandle.ok()) {
        derror("Internal error: merging keys resulted in an invalid JWK.");
        return;
    }

    mCallback(std::move(*keyHandle));
}

std::string JwkDirectoryObserver::mergeKeys() {
    // Now let's reconstruct our key set.
    std::vector<json> keys;
    for (const auto& [fname, keyset] : mPublicKeys) {
        // contents is a json JWKS, so let's merge them together.
        for (const auto& key : keyset) {
            keys.emplace_back(key);
        }
    }

    json combinedJwk = {{"keys", keys}};
    return combinedJwk.dump(2);
}

// Reads a file into a string.
static std::string readFile(Path fname) {
    std::ifstream fstream(PathUtils::asUnicodePath(fname).c_str());
    std::string contents((std::istreambuf_iterator<char>(fstream)),
                         std::istreambuf_iterator<char>());
    return contents;
}

bool JwkDirectoryObserver::acceptJwkExtOnly(Path path) {
    return android::base::EndsWith(path, kJwkExt);
}

absl::StatusOr<json> JwkDirectoryObserver::extractKeys(Path fname) {
    auto jsonFile = readFile(fname);
    DD("Loaded %s, the json is %s.", jsonFile.c_str(),
       json::jsonaccept(jsonFile) ? "valid" : "invalid");
    auto handle = crypto::tink::JwkSetToPublicKeysetHandle(jsonFile);
    if (!handle.ok()) {
        return absl::UnavailableError(
                absl::StrFormat("%s does not contain a valid jwk", fname));
    }

    auto jsonSnippet =
            crypto::tink::JwkSetFromPublicKeysetHandle(*handle->get());
    if (!jsonSnippet.ok()) {
        return absl::InternalError(absl::StrFormat(
                "Unable to convert key handle to json for file: %s ",
                jsonFile));
    }

    VERBOSE_INFO(grpc, "Loaded  JSON Web Key Sets from %s", fname.c_str());

    // We should have a "keys" array (see:
    // https://datatracker.ietf.org/doc/html/rfc7517)
    return json::parse(*jsonSnippet)["keys"];
}

}  // namespace control
}  // namespace emulation
}  // namespace android
