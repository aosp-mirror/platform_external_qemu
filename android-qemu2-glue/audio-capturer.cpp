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

struct MyState {
    int bytes;
    int freq;
    int bits;
    int nchannels;
    android::emulation::AudioCapturer* capturer;
    CaptureVoiceOut* cap;
};

struct Globals {
    using MapType = std::unordered_map<android::emulation::AudioCapturer*, MyState*>;
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
    MyState* state = (MyState *)opaque;
    if (state != nullptr)
        g_free(state);
}

static void my_capture(void* opaque, void* buf, int size)
{
    MyState* state = (MyState *)opaque;
    state->bytes += size;

    if (state->capturer != nullptr) {
        state->capturer->onSample(buf, size);
    }
}

static void my_capture_destroy(void* opaque)
{
    MyState* state = (MyState *)opaque;

    AUD_del_capture (state->cap, state);
}

static void my_capture_info(void* opaque)
{
    (void) opaque;
}

static const capture_ops my_capture_ops = {
    /*.destroy =*/ my_capture_destroy,
    /*.info =*/ my_capture_info
};

namespace android {
namespace qemu {

int QemuAudioCaptureEngine::start(android::emulation::AudioCapturer* capturer)
{
    CaptureState s;

    int freq = capturer->getSamplingRate();
    int bits = capturer->getBits();
    int nchannels = capturer->getChannels();
    audsettings as;
    audio_capture_ops ops;
    int stereo, bits16, shift;
    CaptureVoiceOut* cap;

    if (bits != 8 && bits != 16) {
        derror("Invalid bits value audio capture\n");
        return -1;
    }

    if (nchannels != 1 && nchannels != 2) {
        derror("Invalid channels value audio capture\n");
        return -1;
    }

    stereo = nchannels == 2;
    bits16 = bits == 16;

    as.freq = freq;
    as.nchannels = 1 << stereo;
    as.fmt = bits16 ? AUD_FMT_S16 : AUD_FMT_U8;
    as.endianness = 0;

    ops.notify = my_notify;
    ops.capture = my_capture;
    ops.destroy = my_destroy;

    MyState* state = (MyState *)g_malloc0(sizeof(MyState));

    state->bits = bits;
    state->nchannels = nchannels;
    state->freq = freq;
    state->capturer = capturer;

    cap = AUD_add_capture(&as, &ops, state);
    if (cap == nullptr) {
        derror("Failed to add audio capture\n");
        goto error_free;
    }

    state->cap = cap;
    s.opaque = state;
    s.ops = my_capture_ops;

    sGlobals->mCapturerMap[capturer] = state;

    return 0;

error_free:
    g_free(state);
    return -1;
}

int QemuAudioCaptureEngine::stop(android::emulation::AudioCapturer* capturer)
{
    MyState* state = sGlobals->mCapturerMap[capturer];
    sGlobals->mCapturerMap.erase(capturer);
    if (state == nullptr)
        return -1;

    CaptureVoiceOut* cap = state->cap;
    if (cap == nullptr)
        return -1;

    my_capture_destroy(state);
    return 0;
}

}  // namespace qemu
}  // namespace android

