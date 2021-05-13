// Copyright (C) 2017 The Android Open Source Project
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

#include "android/emulation/AudioCaptureEngine.h"
#include "android/emulation/AudioCapture.h"

namespace android {
namespace emulation {

AudioCaptureEngine* AudioCaptureEngine::mOutputInstance = nullptr;
AudioCaptureEngine* AudioCaptureEngine::mInputInstance = nullptr;

AudioCaptureEngine* AudioCaptureEngine::get(AudioMode mode) {
    switch (mode) {
        case AudioMode::AUDIO_OUTPUT:
            return mOutputInstance;
        case AudioMode::AUDIO_INPUT:
            return mInputInstance;
    }
}

void AudioCaptureEngine::set(AudioCaptureEngine* engine, AudioMode type) {
    switch (type) {
        case AudioMode::AUDIO_OUTPUT:
            mOutputInstance = engine;
        case AudioMode::AUDIO_INPUT:
            mInputInstance = engine;
    }
}
}  // namespace emulation
}  // namespace android
