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

// the audio formats supported by QEMU 1 and 2
enum class AudioFormat { U8, S8, U16, S16, U32, S32 };

// audio callback, should fill audio buffer up to the length specified by the
// avail parameter
typedef void (*audio_callback_fn)(void* opaque, int avail);

// A generic engine for audio output

// AudioOutputEngine provides a generic inteface for audio output.
// The specific vm engine must implements a subclass of AudioOutputEngine.
//
// Usage:
//       At vm starts up, register this enigne by calling the
//       AudioOutputEngine::set() static method bool
//       qemu_android_emulation_early_setup() {
//          ...
//          AudioOutputEngine::set(new YouAudioOutputEngine());
//          ...
//       }
//
// Example:
//
// static QByteArray* g_audio_buffer = nullptr;
//
//  void MyClass::audioCallback(void *opaque, int free)
//{
//    MyClass* pThis = (MyClass*)opaque;
//    if (g_audio_buffer == nullptr) {
//        return;
//    }
//
//    char *data = g_audio_buffer->data();
//    int size = g_audio_buffer->size();
//    int pos = 0;
//    while (free > 0 && pos < size) {
//        int avail = size - pos;
//        if (avail > free)
//            avail = free;
//
//        pThis->mAudioOutputEngine->write(data + pos, avail);
//        pos += avail;
//        free -= avail;
//    }
//}
//
//
//  mAudioOutputEngine = AudioOutputEngine::get();
//  mAudioOutputEngine->open("video-player", AUDIO_FMT_S16, 48000, 2, audioCallback, this);

class AudioOutputEngine {
protected:
    AudioOutputEngine() = default;
    virtual ~AudioOutputEngine() = default;

public:
    // Open the audio output, subclass should implement this method
    // params:
    //     name - an arbitrary name for the audio output
    //     format - audio format, one of the AudioFormat enums,
    //         AUDIO_FMT_S16 is the most common format
    //     freq - audio sampling rate, usually 44100 or 48000
    //     nchannels - number of channels, 2 for stero audio
    //     callback - audio output callback, write() call should be made
    //         inside this callback
    //     callback_opaque - arbitrary parameter for user to pass custom
    //         data reference
    //
    // return:
    //     0 if successful
    //     < 0  if failed
    virtual int open(const char* name,
                     AudioFormat format,
                     int freq,
                     int nchannels,
                     audio_callback_fn callback,
                     void* callback_opaque) = 0;

    // render the buffer, this function should be called from inside the audio
    // audio_callback_fn function
    virtual int write(void* pcm_buf, int size) = 0;

    // close the audio output, subclass should implement this method
    virtual void close() = 0;

    // set the audio capture engine
    // The set() needs to be called at the point where no races are possible
    // for example, during the period where vm setups
    static void set(AudioOutputEngine* engine) { mInstance = engine; }

    static AudioOutputEngine* get();

private:
    static AudioOutputEngine* mInstance;

    DISALLOW_COPY_AND_ASSIGN(AudioOutputEngine);
};

}  // namespace emulation
}  // namespace android
