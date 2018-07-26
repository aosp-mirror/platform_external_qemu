// Copyright (C) 2017 The Android Open Source Project
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
#pragma once

#include "emugl/common/OpenGLDispatchLoader.h"

// gtest has its own definitions for None and Bool
#ifdef None
    #undef None
#endif
#ifdef Bool
    #undef Bool
#endif
#include <gtest/gtest.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

namespace emugl {

// Dimensions for test surface
static const int kTestSurfaceSize[] = {32, 32};

EGLDisplay getDisplay();
EGLConfig createConfig(EGLDisplay dpy, EGLint r, EGLint g, EGLint b, EGLint a, EGLint d, EGLint s, EGLint ms);
EGLSurface pbufferSurface(EGLDisplay dpy, ::EGLConfig config, EGLint w, EGLint h);
EGLContext createContext(EGLDisplay dpy, EGLConfig config, EGLint maj, EGLint min);
void destroyContext(EGLDisplay dpy, EGLContext cxt);
void destroySurface(EGLDisplay dpy, EGLSurface surface);
void destroyDisplay(EGLDisplay dpy);

class GLTest : public ::testing::Test {
protected:
    virtual void SetUp();
    virtual void TearDown();

    const GLESv2Dispatch* gl;
    EGLDisplay m_display;
    EGLConfig m_config;
    EGLSurface m_surface;
    EGLContext m_context;
};

}  // namespace emugl
