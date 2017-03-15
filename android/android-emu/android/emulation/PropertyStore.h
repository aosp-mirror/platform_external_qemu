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
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/PropertyWaiter.h"

#include <memory>
#include <string>

namespace android {
namespace emulation {

//
// PropertyStore - the interface for storing and notifying new Android
// Properties.
//
// An Android property is a string in one of two forms:
//
//   "<key>" or "<key>=<value>"
//
// where <value> may be empty.
//
// Below is an diagram of the PropertyStore architecture:
//
//                       ---------------
//                       |PropertyStore|
//                       ---------------
//                       |      |      |
//        ------------------         ------------------
//        |PropertyWaiter 1|   ...   |PropertyWaiter n|
//        ------------------         ------------------
//
// The PropertyStore is responsible for storing any new data given to it
// and notifying any PropertyWaiters that are subscribed to it if this
// new property received is the one they are waiting for.
//
// Every write is queued and a notification is given to the PropertyStoreThread.
// Once the PropertyStoreThread has processed the property, the item is
// removed from the queue. Any PropertyWaiters that have been notified also are
// removed from the waiting list.
//
// Below is an example on how to use the PropertyStore to send data:
//
//   PropertyStore::get()->write("my_key=my_value");
//

class PropertyStore {
    DISALLOW_COPY_ASSIGN_AND_MOVE(PropertyStore);

public:
    using PropertyWaiterPtr = std::shared_ptr<PropertyWaiter>;

    // Constructor
    PropertyStore();
    // Writes prop into the property store.
    // This call is not thread-safe.
    void write(android::base::StringView prop);
    // Adds another PropertyWaiter into the property store. Once subscribed to
    // this property store, you will be notified once your key-value property
    // has been received. This call is thread-safe.
    void addPropertyWaiter(PropertyWaiterPtr waiter);

    // Get the PropertyStore instance. This call is thread-safe.
    static PropertyStore& get();

    // Parse |prop| from the form "<key>=<value>". Returns
    // std::pair< <pkey>, <pvalue> >. If there is no <value> portion,
    // then <pvalue> will be empty.
    static std::pair<std::string, std::string> parseProperty(
            android::base::StringView prop);

    virtual ~PropertyStore();

private:
    // Ran on the propertyThread. Responsible for receiving new
    // properties and notifying PropertyWaiters.
    void service();

private:
    struct PropertyWaiters {
        android::base::Lock lock;
        std::vector<PropertyWaiterPtr> items;
    };

    // Responsible for queuing new properties and notifying
    // PropertyWaiters.
    android::base::FunctorThread mPropertyThread;
    // Message channel for the Android properties. The size may need to
    // be adjusted later if we start feeding more data through the pipe.
    android::base::MessageChannel<std::string, 256> mPropertyQueue;
    PropertyWaiters mPropertyWaiters;
};

}  // namespace emulation
}  // namespace android
