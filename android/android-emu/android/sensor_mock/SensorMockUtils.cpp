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

#include "android/sensor_mock/SensorMockUtils.h"

#include "android/base/Log.h"
#include "offworld.pb.h"

namespace android {
namespace sensor_mock {

namespace {

emulator_automation::LocationOverrideEvent toLocationEvent(
    const ::offworld::LocationData& location_data,
    uint64_t timestamp_us) {
    emulator_automation::LocationOverrideEvent event;
    event.set_latitude(location_data.latitude());
    event.set_longitude(location_data.longitude());
    event.set_meters_elevation(location_data.meters_elevation());
    event.set_speed_knots(location_data.knots_speed());
    event.set_heading_degrees(location_data.degrees_heading());
    event.set_num_satellites(location_data.num_satellites());
    event.set_timestamp_in_us(timestamp_us);
    return event;
}

}

emulator_automation::RecordedEvent toEvent(
    const ::offworld::SensorMockRequest& request) {
    emulator_automation::RecordedEvent event;
    switch (request.function_case()) {
      case ::offworld::SensorMockRequest::kMockLocation:
        *event.mutable_location_override() =
            toLocationEvent(
              request.mock_location().location_data(),
              request.timestamp_us());
        break;
      default:
        LOG(ERROR) << "Unknown sensor mock request type.";
        break;
    }
    return event;
}

}  // namespace sensor_mock
}  // namespace android
