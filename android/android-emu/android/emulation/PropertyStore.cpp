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

#include "android/emulation/PropertyStore.h"

#include "android/base/memory/LazyInstance.h"
#include "android/utils/debug.h"

#include <algorithm>

#define D(...) VERBOSE_TID_DPRINT(aprops, __VA_ARGS__)
#define DF(...) VERBOSE_TID_FUNCTION_DPRINT(aprops, __VA_ARGS__)

using android::base::AutoLock;
using android::base::FunctorThread;
using android::base::LazyInstance;
using android::base::StringView;
using android::base::System;

using std::string;

namespace android {
namespace emulation {

namespace {
static LazyInstance<PropertyStore> sInstance = LAZY_INSTANCE_INIT;
}  // namespace

// static
std::pair<string, string> PropertyStore::parseProperty(StringView prop) {
    string str(prop);
    auto pos = str.find("=");
    if (pos != string::npos) {
        return std::make_pair(str.substr(0, pos),
                              str.substr(pos + 1));
    }
    return std::make_pair(str, string(""));
}

// static
PropertyStore& PropertyStore::get() {
    return sInstance.get();
}

PropertyStore::PropertyStore() :
        mPropertyThread([this]() { service(); }) {
    DF("Created");
    mPropertyThread.start();
}

PropertyStore::~PropertyStore() {
    mPropertyQueue.send("");
    mPropertyThread.wait();
    DF("Destroyed");
}

void PropertyStore::write(StringView prop) {
    DF("Data=%s", prop.c_str());
    mPropertyQueue.send(std::string(prop));
}

void PropertyStore::addPropertyWaiter(PropertyWaiterPtr waiter) {
    AutoLock lock(mPropertyWaiters.lock);
    mPropertyWaiters.items.push_back(waiter);
}

void PropertyStore::service() {
    DF("started");
    while (auto prop = mPropertyQueue.receive()) {
        if (prop->empty()) {
            DF("Received stop signal");
            break;
        }

        auto propPair = parseProperty(*prop);
        DF("Received (key=%s, val=%s)", propPair.first.c_str(), propPair.second.c_str());

        AutoLock lock(mPropertyWaiters.lock);
        mPropertyWaiters.items.erase(std::remove_if(
                mPropertyWaiters.items.begin(),
                mPropertyWaiters.items.end(),
                [propPair](PropertyWaiterPtr waiter) {
                    if (waiter->getKey().compare(propPair.first) == 0) {
                        if (waiter->getValue().empty()) {
                            // not waiting on specific value
                            waiter->setValue(propPair.second);
                            // this call is already locked
                            waiter->signal(true);
                            return true;
                        } else {
                            if (waiter->getValue().compare(propPair.second) == 0) {
                                waiter->signal(true);
                                return true;
                            }
                        }
                    }
                    return false;
                }),
                mPropertyWaiters.items.end());
    }
}

}  // namespace emulation
}  // namespace android
