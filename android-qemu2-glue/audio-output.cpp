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
#include <glib.h>
#include "android-qemu2-glue/audio-output.h"

#include "android/utils/debug.h"

namespace android {
namespace qemu {

int QemuAudioOutputEngine::open(const char *name,
    android::emulation::AudioFormat format, int freq, int nchannels,
    android::emulation::audio_callback_fn callback, void *callback_opaque)
{
    struct audsettings as;

    if (nchannels != 1 && nchannels != 2) {
        derror("Invalid channels value audio capture\n");
        return -1;
    }

    AUD_register_card(name, &mCard);

    as.freq       = freq;
    as.nchannels  = nchannels;
    as.fmt        = convert(format);
    as.endianness = AUDIO_HOST_ENDIANNESS;

    mVoice = AUD_open_out(
        &mCard,
        mVoice,
        name,
        callback_opaque,
        callback,
        &as);

    if (mVoice == nullptr) {
        derror("Cannot open audio output\n");
        return -1;
    }

    AUD_set_active_out(mVoice, 1);

    return 0;
}

int QemuAudioOutputEngine::write(void *pcm_buf, int size)
{
    if (mVoice != nullptr) {
        return AUD_write(mVoice, pcm_buf, size);
    } else {
        return -1;
    }
}

void QemuAudioOutputEngine::close()
{
    if (mVoice != nullptr) {
        AUD_close_out(&mCard, mVoice);
        mVoice = nullptr;
    }

    AUD_remove_card(&mCard);
}

// convert audio format
audfmt_e QemuAudioOutputEngine::convert(android::emulation::AudioFormat format)
{
    audfmt_e internal;
    switch (format) {
    case android::emulation::AudioFormat::U8:
        internal = AUD_FMT_U8;
        break;
    case android::emulation::AudioFormat::S8:
        internal = AUD_FMT_S8;
        break;
    case android::emulation::AudioFormat::U16:
        internal = AUD_FMT_U16;
        break;
    case android::emulation::AudioFormat::S16:
        internal = AUD_FMT_S16;
        break;
    case android::emulation::AudioFormat::U32:
        internal = AUD_FMT_U32;
        break;
    case android::emulation::AudioFormat::S32:
        internal = AUD_FMT_S32;
        break;
    default:
        derror("Unsupported audio format, reverting to AUD_FMT_S16\n");
        internal = AUD_FMT_S16;
        break;
    }

    return internal;
}

}  // namespace qemu
}  // namespace android

