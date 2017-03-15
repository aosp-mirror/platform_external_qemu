// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/Compiler.h"
#include "android/base/StringView.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

#include <memory>
#include <string>

namespace android {
namespace emulation {

//
// PropertyWaiter - the interface to wait on an Android property.
//
// Below is an diagram of the PropertyWaiter architecture:
//
//                       ---------------
//                       |PropertyStore|
//                       ---------------
//                       |      |      |
//        ------------------         ------------------
//        |PropertyWaiter 1|   ...   |PropertyWaiter n|
//        ------------------         ------------------
//
// A PropertyWaiter needs to be subscribed to a PropertyStore to
// get notifications. Once subscribed, the PropertyStore will give
// a notification of when your property key and/or value has been
// received.
//
// Below is an example on how to use a PropertyWaiter to wait:
//
//   std::shared_ptr<PropertyWaiter> waiter =
//           std::make_shared(new PropertyWaiter("my_key"));
//   PropertyStore::get()->addPropertyWaiter(waiter);
//   waiter->wait(); // blocking call
//
//  The wait() call will block until the PropertyStore has received
//  your property.
//
//  Once you have received a notification, the PropertyStore will
//  remove you from the waiting list. Thus, to re-wait on a property,
//  you must re-subscribe.
//
//  If you did not set the <value> part prior to subscribing to a PropertyStore,
//  the PropertyStore will override this value if it receives it along with
//  <key>.
//

class PropertyWaiter {
    DISALLOW_COPY_ASSIGN_AND_MOVE(PropertyWaiter);

public:
    // Ctor. |value| is optional. Only set it if you are waiting for
    // |key| to have a particular value.
    PropertyWaiter(android::base::StringView key,
                   android::base::StringView value = "");
    virtual ~PropertyWaiter();

    // Note: on the client-side, if you need to call the setters, make sure
    // to do it BEFORE subscribing to the PropertyStore, as the PropertyStore
    // will call the setters too.
    const std::string& getKey();
    const std::string& getValue();
    void setKey(android::base::StringView key);
    void setValue(android::base::StringView value);

    // Blocks until receiving mKey and mValue (if applicable). Returns true iff
    // the property was received.
    bool wait();

    // Signals the wait() thread to unblock. |found| is whether or not the
    // property was found.
    void signal(bool found);

private:
    // Ran on the propertyThread. Responsible for receiving new
    // properties and notifying PropertyWaiters.
    void service();

private:
    struct WaitSignal {
        android::base::Lock lock;
        android::base::ConditionVariable cvDone;
        bool done; // cvDone.wait() may give false positive.
        bool found; // Whether property was found.
    };

    WaitSignal mWaitSignal;
    std::string mKey;
    std::string mValue;
};

}  // namespace emulation
}  // namespace android
