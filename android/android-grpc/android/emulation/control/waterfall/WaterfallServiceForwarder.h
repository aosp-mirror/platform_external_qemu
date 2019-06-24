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

#include <memory>

#include "android/base/async/Looper.h"
#include "android/emulation/control/waterfall/WaterfallConnection.h"
#include "waterfall.grpc.pb.h"

namespace android {
namespace emulation {
namespace control {

using android::base::Looper;

class WaterfallServiceForwarder {
public:
    WaterfallServiceForwarder(WaterfallConnection* con);
    std::unique_ptr<waterfall::Waterfall::Stub> getWaterfallStub();

private:
    std::shared_ptr<grpc::Channel> openChannel();
    std::unique_ptr<WaterfallConnection> mWaterfallConnection;
    Looper* mLooper;
};

WaterfallServiceForwarder* getDefaultWaterfallServiceForwarder();

}  // namespace control
}  // namespace emulation
}  // namespace android
