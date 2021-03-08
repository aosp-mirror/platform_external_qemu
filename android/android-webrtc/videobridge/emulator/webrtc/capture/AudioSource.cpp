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
#include "emulator/webrtc/capture/AudioSource.h"

#include <api/media_stream_interface.h>  // for AudioTrackSinkInterface

namespace emulator {
namespace webrtc {
void AudioSource::OnData(const void* audio_data,
                         int bits_per_sample,
                         int sample_rate,
                         size_t number_of_channels,
                         size_t number_of_frames) {
    const std::lock_guard<std::mutex> lock(mMutex);
    if ((mSink != nullptr) && (audio_data != nullptr)) {
        mSink->OnData(audio_data, bits_per_sample, sample_rate,
                      number_of_channels, number_of_frames);
    }
}
}  // namespace webrtc
}  // namespace emulator
