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

#include "android/opengl/GLPipeControl.h"
#include "android/opengl/gl_pipe_control.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"

#include <string.h>

namespace android {
namespace opengl {

using base::Lock;
using base::AutoLock;

// The one global pipe control meant to work with
// actual users of pipe.
static base::LazyInstance<GLPipeControl> sGLPipeControl;

GLPipeControl* GLPipeControl::get() {
    return sGLPipeControl.ptr();
}

void GLPipeControl::registerRemoveCallback(void (*f)(void*)) {
    mCallback = f;
}

void GLPipeControl::addPipe(void* pipe) {
    AutoLock lock(mLock);
    fprintf(stderr, "%s: add %p\n", __FUNCTION__, pipe);
    uint64_t handle = (uint64_t)(uintptr_t)pipe;
    mPipes[handle] = pipe;
}

void GLPipeControl::removePipe(void* pipe) {
    AutoLock lock(mLock);
    fprintf(stderr, "%s: remove %p\n", __FUNCTION__, pipe);
    uint64_t handle = (uint64_t)(uintptr_t)pipe;
    mPipes.erase(handle);
}

void GLPipeControl::stopPipe(uint64_t handle) {
    // No lock here, because mCallback may end up
    // calling removePipe concurrently.
    fprintf(stderr, "%s: todo: stop %p\n", __FUNCTION__,
            mPipes[handle]);
    if (auto val = android::base::find(mPipes, handle)) {
        fprintf(stderr, "%s: found the pipe. would stop it here. val=%p\n", __FUNCTION__, *val);
        mCallback(*val);
    } else {
        fprintf(stderr, "%s: didn't find the pipe...\n", __FUNCTION__);
    }
}

void GLPipeControl::stopAll() {
    for (auto& it : mPipes) {
        stopPipe(it.first);
    }
}

std::vector<void*> GLPipeControl::listPipes() {
    AutoLock lock(mLock);
    std::vector<void*> res;
    for (auto elt : mPipes) {
        res.push_back(elt.second);
    }
    return res;
}

} // namespace opengl
} // namespace android

void opengl_pipe_control_stop_pipe(uint64_t handle) {
    fprintf(stderr, "%s: call. handle=0x%llx", __FUNCTION__, (unsigned long long)handle);
    android::opengl::GLPipeControl::get()->stopPipe(handle);
}

void opengl_pipe_control_list_pipes(char** dst) {
    std::vector<void*> pipes = android::opengl::GLPipeControl::get()->listPipes();

    std::string strList;
    char buf[256] = {};
    for (auto elt : pipes) {
        snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)(uintptr_t)elt);
        strList += buf;
        strList += "\n";
    }

    uint32_t res_sz = (uint32_t)(strList.size() + 1);
    char* res = (char*)malloc(res_sz);
    memset(res, 0, res_sz);
    memcpy(res, &strList[0], res_sz);
    *dst = res;
}
