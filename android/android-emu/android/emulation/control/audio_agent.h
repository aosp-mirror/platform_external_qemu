/* Copyright (C) 2021 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

enum SampleFormat {
    SAMPLE_FORMAT_U8,  // Unsigned 8 bit
    SAMPLE_FORMAT_S16 // Signed 16 bit (little endian)
};

typedef struct AudioSettings {
    // The frequency of the audio.
    int frequency;
    // format
    enum SampleFormat format;
    // Number of channels.
    int nChannels;
} AudioSettings;
//
typedef int (*onSample)(uint8_t *buf, int size);

typedef struct QAndroidAudioAgent {
    // Allows you to override the current microphone (which could be the physical device)
    // When the emulator requests microphone data the callback will be invoked and you will be
    // requested to fill the given buffer with size number of bytes.
    void (*overrideMicrophone)(const AudioSettings settings, onSample audioReadFunction);

    void (*close)(onSample* audioReadFunction);
} QAndroidAudioAgent;

ANDROID_END_HEADER
