// Copyright 2017 The Android Open Source Project
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

#include "android/opengl/NativeGpuInfo.h"
#include "android/utils/system.h"

#include <GL/glx.h>

// Based on http://web.mit.edu/jhawk/mnt/spo/3d/src/Mesa-3.0/samples/oglinfo.c
// and EglosApi_glx.cpp

int visual_request_none[] = { None };
int visual_request_rgba[] = { GLX_RGBA, None };

void getGpuInfoListNative(GpuInfoList* out) {

    fprintf(stderr, "%s: start at %" PRIu64 " ms\n", __func__, get_uptime_ms());

    Display* dpy = XOpenDisplay(nullptr);

    if (!dpy) return;

    XVisualInfo* vis;

    if (!(vis = glXChooseVisual(dpy, 0, visual_request_none))) {
        if (!(vis = glXChooseVisual(dpy, 0, visual_request_rgba))) {
            return;
        }
    }

    int numConfigs;
    GLXFBConfig* fbConfigs = glXGetFBConfigs(dpy, 0, &numConfigs);
    if (!numConfigs) return;

    const int pbAttribs[] = {
        GLX_PBUFFER_WIDTH, 1,
        GLX_PBUFFER_HEIGHT, 1,
        None
    };

    GLXPbuffer pb = glXCreatePbuffer(dpy, fbConfigs[0], pbAttribs);
    GLXContext ctx = glXCreateContext(dpy, vis, 0, GL_TRUE);

    bool currentResult = glXMakeContextCurrent(dpy, pb, pb, ctx);

    if (!currentResult) return;

    const char* versionString = (const char*)glGetString(GL_VERSION);
    const char* rendererString = (const char*)glGetString(GL_RENDERER);

    for (auto& gpu : out->infos) {
        gpu.version = versionString;
        gpu.renderer = rendererString;
    }

    glXMakeContextCurrent(dpy, 0, 0, NULL);
    glXDestroyPbuffer(dpy, pb);
    glXDestroyContext(dpy, ctx);

    fprintf(stderr, "%s: end at %" PRIu64 " ms\n", __func__, get_uptime_ms());
}
