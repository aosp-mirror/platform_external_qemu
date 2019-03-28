// Copyright (C) 2019 The Android Open Source Project
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

#include <vector>

#include <inttypes.h>

namespace android {
namespace base {

template<size_t indexBits,
         size_t generationBits,
         size_t typeBits,
         class Item>
class EntityManager {
public:
    using EntityHandle = uint64_t;

    static size_t index(EntityHandle h) {
        return static_cast<size_t>(h & ((1 << indexBits) - 1));
    }

    static size_t generation(EntityHandle h) {
        return static_cast<size_t>(
            (h >> indexBits) &
            ((1 << generationBits) - 1));
    }

    static size_t handleType(EntityHandle h) {
        return static_cast<size_t>(
            (h >> (indexBits + generationBits)) &
            ((1 << typeBits) - 1));
        return h & ((1 << indexBits) - 1);
    }

    static EntityHandle asHandle(
        size_t index,
        size_t generation,
        size_t type) {
        EntityHandle res = index;
        res |= generation << indexBits;
        res |= type << (indexBits + generationBits);
        return res;
    }

};

} // namespace android
} // namespace base


