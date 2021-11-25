// Copyright (C) 2021 The Android Open Source Project
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
#include "qemu/osdep.h"
#include "qemu/host-utils.h"
#include "qemu/timer.h"
#include "audio.h"
#include "audio_forwarder.h"
#include "qemu/thread.h"

#define AUDIO_CAP "fwd"
#include "audio_int.h"
#include "audio.h"
#include "hw/audio/intel-hda.h"

// Hack attack, we are going to be modifying the global audio state to interject a new active driver.
extern SWVoiceIn *g_active_sw;
extern struct audsettings g_active_settings;
extern struct audio_capture_ops g_active_capture;
extern void *g_active_opaque;

typedef struct FWDVoiceIn
{
    HWVoiceIn hw;
    int64_t old_ticks;
    void *pcm_buf;
    int pcm_size;
} FWDVoiceIn;

// The global audio state, containing the driver information
extern AudioState glob_audio_state;

// Our previous active configuration that is swapped out.
struct audio_driver *g_prev_drv = NULL;
void *g_prev_drv_opaque = NULL;
SWVoiceIn *g_prev_sw = NULL;
bool g_prev_is_real_audio_allowed;
QemuMutex g_deactivation_mutex = {.initialized = false };


struct audio_pcm_info g_prev_info;
struct audio_pcm_info g_empty_info = { 0 };
HWVoiceIn *g_prev_hw;

// Our current active configuration that is swapped in.
SWVoiceIn *g_active_sw = NULL;
struct audsettings g_active_settings;
struct audio_capture_ops g_active_capture;
void *g_active_opaque;
read_available g_active_avail;

void qemu_allow_real_audio(bool allow);
bool qemu_is_real_audio_allowed(void);
void disable_fixed_input_conf();
void enabled_fixed_input_conf();


int enable_forwarder(struct audsettings *as, struct audio_capture_ops *ops,
                     read_available available_fn,
                     void *opaque)
{
    if (!g_deactivation_mutex.initialized) {
        qemu_mutex_init(&g_deactivation_mutex);
    }

    // Initialize the forward driver..
    AudioState *s = &glob_audio_state;
    disable_fixed_input_conf();

    // Store previous state, and disable current microphone.
    g_prev_drv = s->drv;
    g_prev_drv_opaque = s->drv_opaque;
    g_prev_sw = get_hda_voice_in();
    g_prev_hw = g_prev_sw->hw;
    g_prev_info = g_prev_hw->info;

    g_prev_hw->info = g_empty_info;
    g_prev_is_real_audio_allowed = qemu_is_real_audio_allowed();
    g_active_avail = available_fn;
    AUD_set_active_in(g_prev_sw, 0);

    // Activate our forward driver.
    audio_driver *driver = audio_driver_lookup("fwd");
    if (!driver ||
        audio_driver_init(s, driver))
    {
        // This shouldn't happen..
        return -1;
    }

    // Make sure we have at least 1 in voice channel.
    s->nb_hw_voices_in = 1;

    g_active_capture = *ops;
    g_active_opaque = opaque;
    g_active_settings = *as;

    g_active_sw = set_hda_voice_in(NULL);

    // gRPC means we will allow audio for now..
    qemu_allow_real_audio(true);
    AUD_set_active_in(g_active_sw, 1);
    return 0;
}

void disable_forwarder()
{
    qemu_mutex_lock(&g_deactivation_mutex);
    enabled_fixed_input_conf();
    g_prev_hw->info = g_prev_info;

    // Disable us if we were active.
    if (g_active_sw)
    {
        AUD_set_active_in(g_active_sw, 0);
        AUD_close_in(g_active_sw->card, g_active_sw);
        g_active_sw = NULL;
        g_active_opaque = NULL;
    }

    // Restore the previous driver, and activate the previous input channel.
    AudioState *s = &glob_audio_state;
    s->drv = g_prev_drv;
    s->drv_opaque = g_prev_drv_opaque;
    set_hda_voice_in(g_prev_sw);
    AUD_set_active_in(g_prev_sw, 1);


    // TODO(jansene): Is this what we really want?
    // (We could also refactor this by disabling the sw voice, which would do the same)
    qemu_allow_real_audio(g_prev_is_real_audio_allowed);
    qemu_mutex_unlock(&g_deactivation_mutex);
}

static int fwd_init_in(HWVoiceIn *hw, struct audsettings *unused, void *drv_opaque)
{
    struct audsettings *as = (struct audsettings *)drv_opaque;
    FWDVoiceIn *fwdin = (FWDVoiceIn *)hw;
    audio_pcm_init_info(&hw->info, as);
    hw->samples = 4096;
    fwdin->pcm_buf = audio_calloc(__func__, 2 * hw->samples, 1 << hw->info.shift);
    fwdin->pcm_size = hw->samples * (1 << hw->info.shift);
    return 0;
}

static void fwd_fini_in(HWVoiceIn *hw)
{
    FWDVoiceIn *fwdin = (FWDVoiceIn *)hw;
    g_free(fwdin->pcm_buf);
    fwdin->pcm_buf = NULL;
 }

static int fwd_run_in(HWVoiceIn *hw)
{
    qemu_mutex_lock(&g_deactivation_mutex);
    if (!g_active_sw) {
        qemu_mutex_unlock(&g_deactivation_mutex);
        return 0;
    }

    FWDVoiceIn *fwdin = (FWDVoiceIn *)hw;
    int live = audio_pcm_hw_get_live_in(hw);
    int dead = hw->samples - live;
    int samples = 0;

    // dead contains the number of samples that can be filled in.
    if (dead)
    {
        // We are now going to calculate at what rate we consume samples.
        int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        int64_t ticks = now - fwdin->old_ticks;
        int64_t bytes =
            muldiv64(ticks, hw->info.bytes_per_second, NANOSECONDS_PER_SECOND);

        fwdin->old_ticks = now;

        // And see how much we can pull from our external queue.
        int64_t avail = g_active_avail(g_active_opaque);
        bytes = audio_MIN(bytes, avail);
        samples = bytes >> hw->info.shift;
        samples = audio_MIN(samples, dead);
    }

    // Now we are going to fill the ring buffer.
    int to_grab = samples;
    while (to_grab)
    {
        // Check how many samples we can fill until we reach the end of our ring
        int chunk = audio_MIN(to_grab, hw->samples - hw->wpos);

        // Obtain the samples from our input module (converting samples to # of bytes)
        g_active_capture.capture(g_active_opaque, fwdin->pcm_buf, chunk << hw->info.shift);

        // And convert them to the SW voice format.
        hw->conv(hw->conv_buf + hw->wpos, fwdin->pcm_buf, chunk);

        // And get the next.
        hw->wpos = (hw->wpos + chunk) % hw->samples;
        to_grab -= chunk;
    }

    qemu_mutex_unlock(&g_deactivation_mutex);
    return samples;
}

static int fwd_read_in(SWVoiceIn *sw, void *buf, int len)
{
    return audio_pcm_sw_read(sw, buf, len);
}

static int fwd_ctl_in(HWVoiceIn *hw, int cmd, ...)
{
    (void)hw;
    (void)cmd;
    return 0;
}

static void *fwd_audio_init(void)
{
    return &g_active_settings;
}

static void fwd_audio_fini(void *opaque)
{
    ldebug("fwd_fini");
    g_free(opaque);
}

static struct audio_option fwd_options[] = {
    {/* End of list */}};

static struct audio_pcm_ops fwd_pcm_ops = {
    .init_in = fwd_init_in,
    .fini_in = fwd_fini_in,
    .run_in = fwd_run_in,
    .read = fwd_read_in,
    .ctl_in = fwd_ctl_in};

static struct audio_driver fwd_audio_driver = {
    .name = "fwd",
    .descr = "Forwards the microphone from AEMU gRPC to QEMU.",
    .options = fwd_options,
    .init = fwd_audio_init,
    .fini = fwd_audio_fini,
    .pcm_ops = &fwd_pcm_ops,
    .can_be_default = 0,
    .max_voices_out = 0,
    .max_voices_in = 1,
    .voice_size_out = 0,
    .voice_size_in = sizeof(FWDVoiceIn),
};

static void register_audio_fwd(void)
{
    audio_driver_register(&fwd_audio_driver);
}
type_init(register_audio_fwd);
