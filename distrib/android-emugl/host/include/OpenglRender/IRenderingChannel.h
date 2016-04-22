// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/EnumFlags.h"

#include <functional>
#include <memory>
#include <vector>

namespace emugl {

using namespace ::android::base::EnumFlags;

using ChannelBuffer = std::vector<char>;

class IRenderingChannel {
public:
    enum class Operation {
        Read = 1 << 0,
        Write = 1 << 1
    };
    using OperationReadyCallback = std::function<void(Operation)>;

    virtual void setOperationReadyCallback(OperationReadyCallback callback) = 0;

    virtual bool write(ChannelBuffer&& buffer) = 0;
    virtual ChannelBuffer read() = 0;

    virtual void stop() = 0;

protected:
    ~IRenderingChannel() = default;
};

using IRenderingChannelPtr = std::shared_ptr<IRenderingChannel>;

}  // namespace emugl
