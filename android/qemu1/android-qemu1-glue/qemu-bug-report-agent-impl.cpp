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

#include "android-qemu1-glue/qemu-control-impl.h"

#include "android/bugreport.h"

void generate_bugreport(BugreportCallback callback, void* context) {
    generateBugreport(callback, context);
}

int save_bugreport(int include_screenshot, int include_adbBugreport) {
    return saveBugreport(include_screenshot, include_adbBugreport);
}

int send_to_google() {
    return sendToGoogle();
}

void set_repro_steps(const char* reproSteps) {
    setReproSteps(reproSteps);
}

int set_save_path(const char* savePath) {
    return setSavePath(savePath);
}

static const QBugreportAgent bugreportAgent = {
        .generateBugreport = &generate_bugreport,
        .saveBugreport = &save_bugreport,
        .sendToGoogle = &send_to_google,
        .setReproSteps = &set_repro_steps,
        .setSavePath = &set_save_path};

const QBugreportAgent* const gQBugreportAgent = &bugreportAgent;