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

extern "C" {
#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "audio/audio.h"
}

#include <map>

typedef struct {
    int bytes;
    int freq;
    int bits;
    int nchannels;
    android::emulation::AudioCapturer *capturer;
    CaptureVoiceOut *cap;
} MyState;

static std::map<android::emulation::AudioCapturer *, MyState *> g_capturer_maps;

static void my_notify (void *opaque, audcnotification_e cmd)
{
    (void) opaque;
    (void) cmd;
}

static void my_destroy (void *opaque)
{
    (void) opaque;
}

static void my_capture (void *opaque, void *buf, int size)
{
    MyState *state = (MyState *)opaque;
    state->bytes += size;

    if (state->capturer != nullptr) {
        state->capturer->onSample(buf, size);
    }
}

static void my_capture_destroy (void *opaque)
{
    MyState *state = (MyState *)opaque;

    AUD_del_capture (state->cap, state);
}

static void my_capture_info (void *opaque)
{
    (void) opaque;
}

static struct capture_ops my_capture_ops = {
    /*.destroy =*/ my_capture_destroy,
    /*.info =*/ my_capture_info
};

namespace android {
namespace qemu {

int QemuAudioCaptureEngine::start(android::emulation::AudioCapturer *capturer)
{
    CaptureState s;

    int freq = capturer->getSamplingRate();
    int bits = capturer->getBits();
    int nchannels = capturer->getChannels();
    struct audsettings as;
    struct audio_capture_ops ops;
    int stereo, bits16, shift;
    CaptureVoiceOut *cap;

    if (bits != 8 && bits != 16) {
        return -1;
    }

    if (nchannels != 1 && nchannels != 2) {
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

    MyState *state = (MyState *)g_malloc0 (sizeof (MyState));

    state->bits = bits;
    state->nchannels = nchannels;
    state->freq = freq;
    state->capturer = capturer;

    cap = AUD_add_capture (&as, &ops, state);
    if (!cap) {
        printf ("Failed to add audio capture\n");
        goto error_free;
    }

    state->cap = cap;
    s.opaque = state;
    s.ops = my_capture_ops;

    g_capturer_maps[capturer] = state;

    return 0;

error_free:
    g_free (state);
    return -1;
}

int QemuAudioCaptureEngine::stop(android::emulation::AudioCapturer *capturer)
{
    MyState *state = g_capturer_maps[capturer];
    CaptureVoiceOut *cap = state->cap; // g_capturer_maps[capturer];
    if (cap == nullptr)
        return -1;

    my_capture_destroy(state);
    return 0;
}

}  // namespace qemu
}  // namespace android

