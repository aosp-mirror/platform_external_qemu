// Copyright 2019 The Android Open Source Project
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

#include <functional>
#include <unordered_map>

namespace android {
namespace emulation {

class CleanupDevice {
public:
    enum Command {
        GetHandle = 0,
    };

    CleanupDevice();
    ~CleanupDevice();

    using Callback = std::function<void()>;

    void addCallback(void* key, Callback cb);
    void removeCallback(void* key);

private:
    std::unordered_map<void*, Callback> mCallbacks;
};

} // namespace emulation
} // namespace android