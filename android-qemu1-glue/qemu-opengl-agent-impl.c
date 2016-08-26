/* Copyright (C) 2016 The Android Open Source Project
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

#include "android-qemu1-glue/qemu-control-impl.h"

#include "android/emulation/control/opengl_agent.h"
#include "android/opengl/GLPipeControl.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

static void opengl_stopGLPipe(uint64_t handle) {
    fprintf(stderr, "%s: call. handle=0x%llx", __FUNCTION__, (unsigned long long)handle);
    pipe_control_stop_pipe(handle);
}

static void opengl_listGLPipes(char** dst) {
    pipe_control_list_pipes(dst);
}

static const QAndroidOpenglAgent sQAndroidOpenglAgent = {
    .stopGLPipe = opengl_stopGLPipe,
    .listGLPipes = opengl_listGLPipes,
};

const QAndroidOpenglAgent* const gQAndroidOpenglAgent =
    &sQAndroidOpenglAgent;
