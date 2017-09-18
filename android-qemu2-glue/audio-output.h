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

#include "android/emulation/AudioOutputEngine.h"

extern "C" {
#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "audio/audio.h"
}

namespace android {
namespace qemu {

// Qemu engine for audio output, a single instance should be used.
// QemuAudioOutputEngine uses QEMU's builtin audio output apis
// to render audio.
//
// Usage:
//    1) At qemu glue starts up, register this enigne by calling
//       the AudioOutputEngine::set() static method:
//       bool qemu_android_emulation_early_setup() {
//          ...
//          QemuAudioOutputEngine::set(
//             new android::qemu::QemuAudioOutputEngine());
//          ...
//       }
//
//    2) Call QemuAudioOutputEngine::open() with the instance created in (1)
//       to open the audio output.
//
//    3) Call QemuAudioOutputEngine::close() with the instance created in (1)
//       to close audio output
//
class QemuAudioOutputEngine : public android::emulation::AudioOutputEngine {
public:
    QemuAudioOutputEngine() = default;
    virtual ~QemuAudioOutputEngine() = default;

    virtual int open(const char *name,
                     android::emulation::AudioFormat format,
                     int freq,
                     int nchannels,
                     android::emulation::audio_callback_fn callback,
                     void *callback_opaque) override;
    virtual int write(void *pcm_buf, int size) override;
    virtual void close() override;

private:
    // convert audio format
    static audfmt_e convert(android::emulation::AudioFormat format);

private:
    QEMUSoundCard  mCard;
    SWVoiceOut* mVoice = nullptr;

    DISALLOW_COPY_AND_ASSIGN(QemuAudioOutputEngine);
};

}  // namespace qemu
}  // namespace android
