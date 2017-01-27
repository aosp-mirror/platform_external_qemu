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

#pragma once

#include "android/base/Compiler.h"

namespace android {
namespace emulation {

class AudioCapturer;

// Audio engine to capture audio, signleton
class AudioCaptureEngine {
protected:
     AudioCaptureEngine();
     virtual ~AudioCaptureEngine();

public:
     // start the capturer to capture audio, subclass should implement this method
     virtual int start(AudioCapturer *capturer) = 0;

     // stop the capturer, subclass should implement this method
     virtual int stop(AudioCapturer *capturer) = 0;

     // set the audio capture engine
     static void set(AudioCaptureEngine* engine) {
         mInstance = engine;
     }

     static AudioCaptureEngine* get() {
         return mInstance;
     }

private:
    static AudioCaptureEngine *mInstance;
 
    DISALLOW_COPY_AND_ASSIGN(AudioCaptureEngine);
};

}  // namespace emulation
}  // namespace android

