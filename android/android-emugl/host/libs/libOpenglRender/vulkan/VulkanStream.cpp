// Copyright (C) 2018 The Android Open Source Project
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

#include "VulkanStream.h"

#include "android/base/files/StreamSerializing.h"

namespace goldfish_vk {

VulkanStream::VulkanStream() {
}

void VulkanStream::alloc(void** ptrAddr, size_t bytes) {
    if (!bytes) {
        *ptrAddr = nullptr;
        return;
    }

    *ptrAddr = mPool.alloc(bytes);
}

void VulkanStream::loadStringInPlace(char** forOutput) {
    size_t len = this->getBe32();

    *forOutput = mPool.allocArray<char>(len + 1);

    memset(*forOutput, 0x0, len + 1);

    if (len > 0) {
        this->read(*forOutput, len);
    }
}

void VulkanStream::loadStringArrayInPlace(char*** forOutput) {
    size_t count = this->getBe32();

    if (!count) { *forOutput = nullptr; return; }

    *forOutput = mPool.allocArray<char*>(count);

    char** stringsForOutput = *forOutput;

    for (size_t i = 0; i < count; i++) {
        loadStringInPlace(stringsForOutput + i);
    }
}

} // namespace goldfish_vk
