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
#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "nlohmann/json.hpp"
#include "tink/keyset_handle.h"

namespace android {
namespace emulation {
namespace control {

// A class for loading JSON Web Key (JWK) sets from files and managing the
// active set of keys.
class JwkKeyLoader {
public:
    using Path = std::string;
    using json = nlohmann::json;
    using Keyset = std::unique_ptr<crypto::tink::KeysetHandle>;

    JwkKeyLoader() = default;
    ~JwkKeyLoader() = default;

    void clear();
    bool empty() const;
    int size() const;

    /**
     * Loads a JWK set from a file and adds it to the collection.
     *
     * @param toAdd The path to the file containing the JWK set.
     * @return An absl::Status indicating success or failure of the operation.
     */
    absl::Status add(Path toAdd);

    /**
     * Loads a JWK set from a JSON string and adds it to the collection.
     *
     * @param toAdd The path to associate with the JWK set.
     * @param jsonString The JSON string containing the JWK set.
     * @return An absl::Status indicating success or failure of the operation.
     */
    absl::Status add(Path toAdd, std::string jsonString);

    /**
     * Removes a JWK set from the collection.
     *
     * @param toRemove The path associated with the JWK set to remove.
     * @return An absl::Status indicating success or failure of the operation.
     */
    absl::Status remove(Path toRemove);

    /**
     * Returns the active JWK keyset as a Tink KeysetHandle.
     *
     * @return An absl::StatusOr containing the active keyset or an error
     * status.
     */
    absl::StatusOr<Keyset> activeKeySet() const;

    /**
     * Returns the active JWK keyset as a JSON object.
     *
     * @return The active keyset as a JSON object.
     */
    json activeKeysetAsJson() const;

    /**
     * Returns the active JWK keyset as a JSON string.
     *
     * @return The active keyset as a JSON string.
     */
    std::string activeKeysetAsString() const;

private:
    std::unordered_map<Path, json> mPublicKeys;
    mutable std::mutex mKeylock;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
