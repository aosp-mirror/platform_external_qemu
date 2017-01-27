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
#include "android/emulation/AudioCapture.h"
#include "android/emulation/AudioCaptureEngine.h"

using namespace android::emulation;

ANDROID_BEGIN_HEADER

// Qemu audio engine to capture audio, signleton
class QemuAudioCaptureEngine : public AudioCaptureEngine {
public:
     QemuAudioCaptureEngine() {}
     virtual ~QemuAudioCaptureEngine() {}

     virtual int start(AudioCapturer *capturer);
     virtual int stop(AudioCapturer *capturer);

private:
    DISALLOW_COPY_AND_ASSIGN(QemuAudioCaptureEngine);
};

ANDROID_END_HEADER
