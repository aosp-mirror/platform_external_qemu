// Copyright (C) 2015 The Android Open Source Project
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

#ifndef ANDROID_BASE_FILES_TIME_VALUE_H
#define ANDROID_BASE_FILES_TIME_VALUE_H

#include "android/base/String.h"

namespace android {
namespace base{

// A very basic class to hold time in seconds/nanoseconds queried from the
// system. This is very loosely based on llvm's implementation in
// llvm/Support/TimeValue.h, without all its trappings. We can make this wine
// full bodied if need arises.
class TimeValue {
public:
    // These types are not optimal.
    // I can't find a portable basic types definition file (yet). Let's keep
    // moving.
    typedef long long int SecondsType;
    typedef long int NanoSecondsType;

    enum TimeConversions {
        NANOSECONDS_PER_MICROSECOND = 1000,
        NANOSECONDS_PER_SECOND = 1000000000
    };

    TimeValue() : seconds_(0), nanos_(0) {}
    explicit TimeValue(SecondsType seconds, NanoSecondsType nanos) :
        seconds_(seconds), nanos_(nanos) {}

    String StringifyToSeconds() const;

private:
    SecondsType seconds_;
    NanoSecondsType nanos_;
};

}  // namespace base
}  // namespace android
#endif  // ANDROID_BASE_FILES_TIME_VALUE_H
