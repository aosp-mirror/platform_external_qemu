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


#include "android/utils/compiler.h"
#include "android/emulation/control/audio_agent.h"
extern "C" {
#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "audio/audio.h"
}

namespace android {
namespace qemu {


class QemuAudioInputEngine {
public:
    QemuAudioInputEngine() = default;
    ~QemuAudioInputEngine() = default;

    int open(AudioSettings as, onSample sample);
    void close();

private:
    int read(void *pcm_buf, int size);

    static void audio_callback_fn(void* opaque, int avail);

    QEMUSoundCard  mCard;
    SWVoiceIn* mVoice = nullptr;
};

}  // namespace qemu
}  // namespace android
