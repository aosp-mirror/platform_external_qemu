// Copyright (C) 2023 The Android Open Source Project
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
#include "android/emulation/control/secure/JwkKeyLoader.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/logging/CLog.h"
#include "tink/jwt/jwk_set_converter.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...)                                                     \
    printf("JwkKeyLoader: %s:%d| " fmt "\n", __func__, __LINE__, \
           ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {

using android::base::PathUtils;

void JwkKeyLoader::clear() {
    std::lock_guard guard(mKeylock);
    mPublicKeys.clear();
}

bool JwkKeyLoader::empty() const {
    std::lock_guard guard(mKeylock);
    return mPublicKeys.empty();
}

int JwkKeyLoader::size() const {
    std::lock_guard guard(mKeylock);
    return mPublicKeys.size();
}

// Reads a file into a string.
static absl::StatusOr<std::string> readFile(JwkKeyLoader::Path fname) {
    std::ifstream fstream(PathUtils::asUnicodePath(fname.data()).c_str());
    std::string contents((std::istreambuf_iterator<char>(fstream)),
                         std::istreambuf_iterator<char>());

    if (!fstream) {
        auto message = absl::StrFormat("Failure reading %s due to: %s", fname,
                                       std::strerror(errno));
        return absl::Status(absl::StatusCode::kInternal, message);
    }
    return contents;
}

absl::Status JwkKeyLoader::add(Path toAdd) {
    auto jsonString = readFile(toAdd);
    if (!jsonString.ok())
        return jsonString.status();

    DD("Loaded %s, the json is %s.", jsonString.c_str(),
       json::accept(jsonString) ? "valid" : "invalid");

    return add(toAdd, *jsonString);
}

absl::Status JwkKeyLoader::add(Path toAdd, std::string jsonString) {
    auto handle = crypto::tink::JwkSetToPublicKeysetHandle(jsonString);
    if (!handle.ok()) {
        return absl::UnavailableError(
                absl::StrFormat("%s does not contain a valid jwk", toAdd));
    }

    auto jsonSnippet =
            crypto::tink::JwkSetFromPublicKeysetHandle(*handle->get());
    if (!jsonSnippet.ok()) {
        return absl::InternalError(absl::StrFormat(
                "Unable to convert key handle to json for file: %s", toAdd));
    }

    json object = json::parse(*jsonSnippet);
    assert(!object.is_discarded());

    // We should have a "keys" array (see:
    // https://datatracker.ietf.org/doc/html/rfc7517)
    assert(object.find("keys") != object.end());

    // Note: At least one key will be present.
    auto keys = object["keys"];
    auto kid = keys.front().find("kid");
    if (kid == keys.front().end()) {
        dwarning("No KeyID found in JWK %s", toAdd.c_str());
    } else {
        DD("KeyID %s, path: %s", kid->get<std::string>().c_str(),
           toAdd.c_str());
    }

    {
        std::lock_guard guard(mKeylock);
        mPublicKeys[toAdd] = keys;
    }
    return absl::OkStatus();
}

absl::Status JwkKeyLoader::remove(Path toRemove) {
    std::lock_guard guard(mKeylock);
    if (!mPublicKeys.count(toRemove)) {
        return absl::NotFoundError(absl::StrFormat(
                "Public key with path %s, does not exist", toRemove));
    }

    mPublicKeys.erase(toRemove);
    return absl::OkStatus();
}

JwkKeyLoader::json JwkKeyLoader::activeKeysetAsJson() const {
    // Now let's reconstruct our key set.
    std::vector<json> keys;
    {
        std::lock_guard guard(mKeylock);
        for (const auto& [fname, keyset] : mPublicKeys) {
            // contents is a json JWKS, so let's merge them together.
            for (const auto& key : keyset) {
                keys.emplace_back(key);
            }
        }
    }

    json combinedJwk = {{"keys", keys}};
    return combinedJwk;
}

std::string JwkKeyLoader::activeKeysetAsString() const {
    constexpr int indentation = 2;
    return activeKeysetAsJson().dump(indentation);
}

absl::StatusOr<JwkKeyLoader::Keyset> JwkKeyLoader::activeKeySet() const {
    auto jsonKeys = activeKeysetAsString();
    auto keyHandle = crypto::tink::JwkSetToPublicKeysetHandle(jsonKeys);
    if (!keyHandle.ok()) {
        return absl::InternalError(keyHandle.status().error_message());
    }

    return absl::StatusOr<Keyset>(std::move(keyHandle.value()));
}

}  // namespace control
}  // namespace emulation
}  // namespace android