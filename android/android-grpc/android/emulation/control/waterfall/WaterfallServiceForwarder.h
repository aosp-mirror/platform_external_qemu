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
#include "waterfall.grpc.pb.h"

namespace android {
namespace emulation {
namespace control {

using Stub = waterfall::Waterfall::Stub;

/**
 * A WaterfallServiceForwarder can be used to open and close a waterfall stub.
 * You must always close an opened stub. You will leak resources if you do not.
 *
 * The waterfall service will recycle existing stubs, only opening a new
 * channel when needed.
 */
class WaterfallServiceForwarder {
public:
    virtual ~WaterfallServiceForwarder() {}

    // Open up a new stub to a waterfall client. The forwarder owns the reference.
    virtual Stub* open() = 0;

    // Close the stub, making it available for the next call.
    virtual void close(Stub*) = 0;

    // Permanently erase this stub. It will never be made available
    // again.
    virtual void erase(Stub*) = 0;
};

/**
 * A service forwarder that will be closed when it gets out of scope.
 */
class ScopedWaterfallServiceForwarder {
public:
    ScopedWaterfallServiceForwarder(WaterfallServiceForwarder* fwd)
        : mFwd(fwd), mStub(fwd->open()) {}
    ~ScopedWaterfallServiceForwarder() { mFwd->close(mStub); }

    Stub* get() { return mStub; }
    Stub& operator*() { return *mStub; }
    Stub* operator->() { return mStub; }

private:
    WaterfallServiceForwarder* mFwd;
    Stub* mStub;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
