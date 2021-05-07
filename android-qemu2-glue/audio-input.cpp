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

#include "android-qemu2-glue/audio-input.h"

#include "android/utils/debug.h"

namespace android {
namespace qemu {

int QemuAudioInputEngine::open(AudioSettings settings, onSample sample)
{
    struct audsettings as;

    if (settings.nChannels != 1 && settings.nChannels != 2) {
        derror("Invalid channels value audio capture\n");
        return -1;
    }

    AUD_register_card("gRPC Microphone", &mCard);

    as.freq       = settings.frequency;
    as.nchannels  = settings.nChannels;
    as.fmt        = (settings.format == SampleFormat::SAMPLE_FORMAT_U8 ? audfmt_e::AUD_FMT_U8 : audfmt_e::AUD_FMT_S16);
    as.endianness = AUDIO_HOST_ENDIANNESS;

    mVoice = AUD_open_in(
        &mCard,
        mVoice,
        "gRPC Microphone",
        this,
        &QemuAudioInputEngine::audio_callback_fn,
        &as);

    if (mVoice == nullptr) {
        derror("Cannot open audio output\n");
        return -1;
    }

    AUD_set_active_in(mVoice, 1);

    return 0;
}



int QemuAudioInputEngine::read(void *pcm_buf, int size)
{
    if (mVoice != nullptr) {
        return AUD_read(mVoice, pcm_buf, size);
    } else {
        return -1;
    }
}

void QemuAudioInputEngine::close()
{
    if (mVoice != nullptr) {
        AUD_close_in(&mCard, mVoice);
        mVoice = nullptr;
    }

    AUD_remove_card(&mCard);
}

void QemuAudioInputEngine::audio_callback_fn(void* opaque, int avail) {
    QemuAudioInputEngine* pthis = reinterpret_cast<QemuAudioInputEngine*>(opaque);
    printf("Getting audio");
    // pthis->read(void *pcm_buf, int size)
}

}  // namespace qemu
}  // namespace android

