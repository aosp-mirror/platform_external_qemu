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

#include "aemu/base/async/Looper.h"
#include "waterfall.grpc.pb.h"

namespace android {
namespace emulation {
namespace control {

using Stub = waterfall::Waterfall::Stub;

/**
 * A WaterfallServiceLibrary can be used to borrow Waterfall stubs.
 * You must always return a borrowed stub. You will leak resources if you do
 * not.
 *
 * The waterfall service will recycle existing stubs. This means that the
 * library will not close the channel (socket) associated with the stub. Before
 * handing out a stub the library will validate that the channel is still open.
 *
 * A Stub can be borrowed only once, and hence a channel will never be shared
 * between various borrowers.
 */
class WaterfallServiceLibrary {
public:
    virtual ~WaterfallServiceLibrary() {}

    // Borrow a unique stub to a waterfall client. The library owns the
    // reference. The channel used by the stub is open at time of borrowing.
    //
    // This method will return a nullptr in case of failure. A nullptr usually
    // indicates that the library was unable to establish a grpc channel to
    // the service destination.
    virtual Stub* borrow() = 0;

    // Return the stub, making it available for the next borrower.
    virtual void restore(Stub*) = 0;
};

/**
 * A waterfall stub that will borrow a stub from the library upon creation, and
 * return it once it goes out of scope.
 */
class ScopedWaterfallStub {
public:
    ScopedWaterfallStub(WaterfallServiceLibrary* fwd)
        : mFwd(fwd), mStub(fwd->borrow()) {}
    ~ScopedWaterfallStub() { mFwd->restore(mStub); }

    Stub* get() { return mStub; }
    Stub& operator*() { return *mStub; }
    Stub* operator->() { return mStub; }

private:
    WaterfallServiceLibrary* mFwd;
    Stub* mStub;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
