// Copyright (C) 2020 The Android Open Source Project
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
#include <modules/audio_device/include/fake_audio_device.h>

namespace emulator {
namespace webrtc {

// This is a fake audio device module that declares that we have one
// stereo recording device. We install this to convince the webrtc
// module that we have a working audio device.
class GoldfishAudioDeviceModule : public ::webrtc::FakeAudioDeviceModule {
    int16_t RecordingDevices() override { return 1; }

    int32_t StereoRecordingIsAvailable(bool* available) const override {
        *available = true;
        return 0;
    }

    int32_t StereoRecording(bool* enabled) const override {
        *enabled = true;
        return 0;
    }
};
}  // namespace webrtc
}  // namespace emulator
