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

#include "android-qemu2-glue/audio-capturer.h"
#include "android-qemu2-glue/utils/stream.h"

#include "android/base/memory/LazyInstance.h"
#include "android/utils/debug.h"

extern "C" {
#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "audio/audio.h"
}

#include <unordered_map>

struct AudioState {
    int bytes = 0;
    int freq = 0;
    int bits = 0;
    int nchannels = 0;
    android::emulation::AudioCapturer* capturer = nullptr;
    CaptureVoiceOut* cap = nullptr;
};

struct Globals {
    using MapType =
            std::unordered_map<android::emulation::AudioCapturer*, AudioState>;
    MapType mCapturerMap;
};

static android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

static void my_notify(void* opaque, audcnotification_e cmd)
{
    (void) opaque;
    (void) cmd;
}

static void my_destroy(void* opaque)
{
    AudioState* state = (AudioState*)opaque;
    if (state != nullptr)
        delete state;
}

static void my_capture(void* opaque, void* buf, int size)
{
    AudioState* state = (AudioState*)opaque;
    state->bytes += size;

    if (state->capturer != nullptr) {
        state->capturer->onSample(buf, size);
    }
}

namespace android {
namespace qemu {

int QemuAudioCaptureEngine::start(android::emulation::AudioCapturer* capturer)
{
    int freq = capturer->getSamplingRate();
    int bits = capturer->getBits();
    int nchannels = capturer->getChannels();

    if (bits != 8 && bits != 16) {
        derror("Invalid bits value audio capture\n");
        return -1;
    }

    if (nchannels != 1 && nchannels != 2) {
        derror("Invalid channels value audio capture\n");
        return -1;
    }

    int stereo = (nchannels == 2);
    int bits16 = (bits == 16);

    audsettings as;
    as.freq = freq;
    as.nchannels = 1 << stereo;
    as.fmt = bits16 ? AUD_FMT_S16 : AUD_FMT_U8;
    as.endianness = 0;

    audio_capture_ops ops;
    ops.notify = my_notify;
    ops.capture = my_capture;
    ops.destroy = my_destroy;

    AudioState* state = &sGlobals->mCapturerMap[capturer];

    state->bits = bits;
    state->nchannels = nchannels;
    state->freq = freq;
    state->capturer = capturer;

    CaptureVoiceOut* cap = AUD_add_capture(&as, &ops, state);
    if (cap == nullptr) {
        sGlobals->mCapturerMap.erase(capturer);
        derror("Failed to add audio capture\n");
        return -1;
    }

    state->cap = cap;

    return 0;
}

// the caller is responsible to free capturer
int QemuAudioCaptureEngine::stop(android::emulation::AudioCapturer* capturer)
{
    auto stateIt = sGlobals->mCapturerMap.find(capturer);
    if (stateIt == sGlobals->mCapturerMap.end())
        return -1;

    AudioState *state = &stateIt->second;
    int rc = -1;
    if (state->cap != nullptr) {
        AUD_del_capture(state->cap, state);
        rc = 0;
    }
    sGlobals->mCapturerMap.erase(stateIt);

    return rc;
}

}  // namespace qemu
}  // namespace android

