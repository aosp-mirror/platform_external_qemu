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

#include "android-qemu2-glue/android_renderer.h"

#include "android/opengl-snapshot.h"
#include "android-qemu2-glue/utils/stream.h"

extern "C" {
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qom/object.h"
#include "migration/vmstate.h"
}

#define OPENGL_SAVE_VERSION 1

static void openglSave(QEMUFile* f, void* opaque) {
    Stream* stream = stream_from_qemufile(f);
    android_saveOpenglRenderer(
        reinterpret_cast<android::base::Stream *>(stream));
    stream_free(stream);
}

static int openglLoad(QEMUFile* f, void* opaque, int version) {
    Stream* stream = stream_from_qemufile(f);
    int ret = android_loadOpenglRenderer(
        reinterpret_cast<android::base::Stream *>(stream),
        version);
    stream_free(stream);
    return ret;
}

void android_qemu2_register_opengl_snapshot(void) {
    register_savevm(nullptr,
                    "renderer",
                    0,
                    OPENGL_SAVE_VERSION,
                    openglSave,
                    openglLoad,
                    nullptr);
}

