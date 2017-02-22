// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "FeatureControl.h"

#include "android/featurecontrol/FeatureControlImpl.h"
#include "android/featurecontrol/proto/emulator_feature_patterns.pb.h"

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/text_format.h"

namespace android {
namespace featurecontrol {

bool isEnabled(Feature feature) {
    return FeatureControlImpl::get().isEnabled(feature);
}

void setEnabledOverride(Feature feature, bool isEnabled) {
    FeatureControlImpl::get().setEnabledOverride(feature, isEnabled);
}

void resetEnabledToDefault(Feature feature) {
    FeatureControlImpl::get().resetEnabledToDefault(feature);
}

#include <string>
#include <stdio.h>
void testParseFeaturePatterns() {
    fprintf(stderr, "%s: call\n", __func__);
    emulator_features::EmulatorHost emuHostTest;
    emuHostTest.set_cpu_manufacturer("AMD");
    std::string out;
    emuHostTest.SerializeToString(&out);
    fprintf(stderr, "%s: test out %s %s\n", __func__, out.c_str(), emuHostTest.DebugString().c_str());
    if (google::protobuf::TextFormat::ParseFromString("cpu_manufacturer: \"AMD\"", &emuHostTest)) {
        fprintf(stderr, "%s: succesful parse\n", __func__);
    } else {
        fprintf(stderr, "%s: failt parse\n", __func__);
    }
    fprintf(stderr, "%s: cpu manufact %s\n", __func__, emuHostTest.cpu_manufacturer().c_str());
}

}  // namespace featurecontrol
}  // namespace android
