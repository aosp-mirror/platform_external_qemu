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

#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/bugreport.h"

BugreportData* generate_bugreport(BugreportCallback callback, void* context) {
    return generateBugreport(callback, context);
}

int save_bugreport(BugreportData* bugreport, int savingFlag) {
    return saveBugreport(bugreport, savingFlag);
}

void set_repro_steps(BugreportData* bugreport, const char* reproSteps) {
    setReproSteps(bugreport, reproSteps);
}

int set_save_path(BugreportData* bugreport, const char* savePath) {
    return setSavePath(bugreport, savePath);
}

static const QBugreportAgent bugreportAgent = {
    .generateBugreport = &generate_bugreport,
    .saveBugreport = &save_bugreport,
    .setReproSteps = &set_repro_steps,
    .setSavePath = &set_save_path
};

const QBugreportAgent* const gQBugreportAgent = &bugreportAgent;
