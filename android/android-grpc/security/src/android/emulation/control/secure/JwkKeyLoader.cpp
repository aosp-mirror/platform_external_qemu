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
#include <chrono>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <thread>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/logging/Log.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"
#include "tink/jwt/jwk_set_converter.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"

#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("JwkKeyLoader: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
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
    if (!base::System::get()->pathIsFile(fname)) {
        return absl::NotFoundError("The path: " + fname + " does not exist.");
    }

    if (!base::System::get()->pathCanRead(fname)) {
        return absl::NotFoundError("The path: " + fname + " is not readable.");
    }

    // Open stream at the end so we can learn the size.
    std::ifstream fstream(PathUtils::asUnicodePath(fname.data()).c_str(),
                          std::ios::binary | std::ios::ate);
    std::streampos fileSize = fstream.tellg();

    if (fileSize == 0) {
        auto message = absl::StrFormat("%s is empty", fname);
        return absl::UnavailableError(message);
    }

    if (fstream.fail()) {
        auto message = absl::StrFormat("%s failed to open.", fname);
        return absl::UnavailableError(message);
    }

    // Usually a jkw file is around 300 bytes... so...
    constexpr int MAX_JWK_SIZE = 8 * 1024;
    if (fileSize > MAX_JWK_SIZE) {
        auto message = absl::StrFormat(
                "Refusing to read %s of size %d, which is over our max of %d.",
                fname, (int)fileSize, MAX_JWK_SIZE);
        return absl::PermissionDeniedError(message);
    }

    // Allocate the string with the required size and read it.
    std::string contents(fileSize, ' ');
    fstream.seekg(0, std::ios::beg);
    fstream.read(&contents[0], fileSize);

    if (fstream.bad()) {
        auto message = absl::StrFormat("Failure reading %s due to: %s", fname,
                                       std::strerror(errno));
        return absl::InternalError(message);
    }

    return contents;
}

absl::Status JwkKeyLoader::addWithRetryForEmpty(
        Path toAdd,
        int retries,
        std::chrono::milliseconds wait_for) {
    absl::Status status;
    do {
        status = add(toAdd);
        if (status.code() == absl::StatusCode::kUnavailable) {
            VERBOSE_INFO(grpc, "Token not yet available, waiting %d ms.",
                         wait_for.count());
            std::this_thread::sleep_for(wait_for);
        }
        retries--;
    } while (status.code() == absl::StatusCode::kUnavailable && retries > 0);

    return status;
}

absl::Status JwkKeyLoader::add(Path toAdd) {
    auto jsonString = readFile(toAdd);
    if (!jsonString.ok())
        return jsonString.status();

    if (!json::accept(*jsonString)) {
        return absl::InternalError(
                absl::StrFormat("%s contains: %s, an invalid json object.",
                                toAdd, *jsonString));
    }

    return add(toAdd, *jsonString);
}

absl::Status JwkKeyLoader::add(Path toAdd, std::string jsonString) {
    auto handle = crypto::tink::JwkSetToPublicKeysetHandle(jsonString);
    if (!handle.ok()) {
        VERBOSE_INFO(grpc, "%s contains %s, which is invalid.", toAdd,
                     jsonString);
        return absl::InternalError(
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
        return absl::InternalError(keyHandle.status().message());
    }

    return absl::StatusOr<Keyset>(std::move(keyHandle.value()));
}

}  // namespace control
}  // namespace emulation
}  // namespace android
