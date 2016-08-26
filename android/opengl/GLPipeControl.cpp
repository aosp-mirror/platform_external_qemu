// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/containers/Lookup.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidPipe.h"
#include "android/opengl/GLPipeControl.h"
#include "android/opengles-pipe.h"

#include <string>
#include <unordered_map>

using android::base::Lock;
using android::base::AutoLock;

static Lock sPipeControlLock;
static std::unordered_map<uint64_t, void*> sPipeControlGLPipes;

void pipe_control_add_pipe(void* pipe) {
    AutoLock lock(sPipeControlLock);
    fprintf(stderr, "%s: add %p\n", __FUNCTION__, pipe);
    uint64_t handle = (uint64_t)(uintptr_t)pipe;
    sPipeControlGLPipes[handle] = pipe;
}

void pipe_control_remove_pipe(void* pipe) {
    AutoLock lock(sPipeControlLock);
    fprintf(stderr, "%s: remove %p\n", __FUNCTION__, pipe);
    uint64_t handle = (uint64_t)(uintptr_t)pipe;
    sPipeControlGLPipes.erase(handle);
}

void pipe_control_stop_pipe(uint64_t handle) {
    fprintf(stderr, "%s: todo: stop %p\n", __FUNCTION__,
            sPipeControlGLPipes[handle]);
    if (auto val = android::base::find(sPipeControlGLPipes, handle)) {
        fprintf(stderr, "%s: found the pipe. would stop it here. val=%p\n", __FUNCTION__, *val);
        android_close_opengles_pipe(*val);
    } else {
        fprintf(stderr, "%s: didn't find the pipe...\n", __FUNCTION__);
    }
}

void pipe_control_list_pipes(char** out) {
    AutoLock lock(sPipeControlLock);

    std::string pipelist;
    char buf[256] = {};
    for(auto elt : sPipeControlGLPipes) {
        snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)elt.first);
        pipelist += buf;
        pipelist += "\n";
    }

    char* res = (char*)malloc(pipelist.size() + 1);
    memset(res, 0, pipelist.size() + 1);
    memcpy(res, &pipelist[0], pipelist.size());
    *out = res;
}

