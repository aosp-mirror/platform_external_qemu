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

#pragma once

#include "android/emulation/AudioCapture.h"
#include "android/emulation/AudioCaptureEngine.h"
#include "android/utils/compiler.h"
#include <atomic>

namespace android {
namespace qemu {

// Qemu engine for audio capturing, a single instance should be used.
// QemuAudioCaptureEngine uses QEMU's builtin audio capture apis
// to capture audio and send the stream to the caller.
//
// Usage:
//    1) At qemu glue starts up, register this enigne by calling
//       the AudioCaptureEngine::set() static method:
//       bool qemu_android_emulation_early_setup() {
//          ...
//          QemuAudioCaptureEngine::set(
//             new android::qemu::QemuAudioCaptureEngine());
//          ...
//       }
//
//    2) Create a new AudioCapturer instance which impelents onSample() method
//
//    3) Call QemuAudioCaptureEngine::start() with the instance created in (1)
//       to start capture. After this call, audio byte stream will arrive
//       in AudioCapturer::onSample() method
//
//    4) Call QemuAudioCaptureEngine::stop() with the instance created in (1)
//       to stop capture
//
class QemuAudioCaptureEngine : public android::emulation::AudioCaptureEngine {
public:
    QemuAudioCaptureEngine() = default;
    virtual ~QemuAudioCaptureEngine() = default;

    virtual int start(android::emulation::AudioCapturer* capturer) override;
    virtual int stop(android::emulation::AudioCapturer* capturer) override;

private:
    DISALLOW_COPY_AND_ASSIGN(QemuAudioCaptureEngine);
};


// A minimalistic input engine that allows you to override the active microphone input.
//
// Some things to note:
//   - Only one can be active, no mixing of sources is taking place. This means
//     that calling start will disable the *real* microphone, and starting a 2nd
//     input engine will fail.
//   - Overrides the UI setting that supresses "real" microphone usage.
class QemuAudioInputEngine : public android::emulation::AudioCaptureEngine {
public:
    QemuAudioInputEngine() = default;
    virtual ~QemuAudioInputEngine() = default;

    virtual int start(android::emulation::AudioCapturer* capturer) override;
    virtual int stop(android::emulation::AudioCapturer* capturer) override;

private:
    std::atomic_bool mRunning;
    DISALLOW_COPY_AND_ASSIGN(QemuAudioInputEngine);
};

}  // namespace qemu
}  // namespace android
