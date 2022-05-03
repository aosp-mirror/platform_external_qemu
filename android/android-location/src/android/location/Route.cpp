// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/location/Route.h"

#include "android/protobuf/LoadSave.h"

using android::protobuf::loadProtobuf;
using android::protobuf::ProtobufLoadResult;

namespace android {
namespace location {

Route::Route(const char* name) : mName(name) {
    ProtobufLoadResult loadResult = loadProtobuf(name, mRoutePb, &mSize);
    isValid = (loadResult == ProtobufLoadResult::Success);
}

const emulator_location::RouteMetadata* Route::getProtoInfo() {
    return isValid ? &mRoutePb : nullptr;
}


}  // namespace location
}  // namespace android
