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

#include <grpcpp/grpcpp.h>
#include "android/emulation/control/waterfall/WaterfallService.h"

namespace android {
namespace emulation {
namespace control {

grpc::Service* getWaterfallService(const char* type) {
    WaterfallProvider variant = WaterfallProvider::none;
    if (type && strcmp("adb", type) == 0)
        variant = WaterfallProvider::adb;
    else if (type && strcmp("forward", type) == 0)
        variant = WaterfallProvider::forward;

    return getWaterfallService(variant);
}

grpc::Service* getWaterfallService(WaterfallProvider variant) {
    switch (variant) {
#ifndef _WIN32
        case WaterfallProvider::adb:
            return getAdbWaterfallService();
        case WaterfallProvider::forward:
            return getWaterfallService();
#endif
        default:
            return nullptr;
    }
}
}  // namespace control
}  // namespace emulation
}  // namespace android