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
#include "android/emulation/control/utils/SimpleEmulatorControlClient.h"

#include <grpcpp/grpcpp.h>
#include <tuple>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "aemu/base/logging/CLog.h"
#include "android/emulation/control/utils/GenericCallbackFunctions.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"

// #define DEBUG 0
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace emulation {
namespace control {

void SimpleEmulatorControlClient::setBattery(BatteryState state,
                                             OnCompleted<Empty> onDone) {
    DD("setBattery %s", state.ShortDebugString().c_str());
    auto [request, response, context] =
            createGrpcRequestContext<BatteryState, Empty>(mClient);
    request->CopyFrom(state);
    mService->async()->setBattery(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

}  // namespace control
}  // namespace emulation
}  // namespace android