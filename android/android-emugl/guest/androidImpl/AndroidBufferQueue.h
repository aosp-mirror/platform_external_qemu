// Copyright 2018 The Android Open Source Project
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
// limitations under the License.#pragma once
#pragma once

#include <memory>

class ANativeWindowBuffer;

namespace aemu {

class AndroidBufferQueue {
public:
    static constexpr int kCapacity = 3;

    class Item {
    public:
        Item(ANativeWindowBuffer* buf = 0, int fd = -1) :
            buffer(buf), fenceFd(fd) { }
        ANativeWindowBuffer* buffer = 0;
        int fenceFd = -1;
    };

    AndroidBufferQueue();
    ~AndroidBufferQueue();

    void cancelBuffer(const Item& item);

    void queueBuffer(const Item& item);
    void dequeueBuffer(Item* outItem);

    bool try_dequeueBuffer(Item* outItem);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
}; // namespace aemu

} // namespace qemu