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

#include "android/base/Log.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/threads/Thread.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"

#include <gtest/gtest.h>

#include "GLESv2Decoder.h"
#include "GLSnapshot.h"
#include "OpenGLTestContext.h"
#include "TextureDraw.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>
#include <memory>

namespace emugl {

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::PathUtils;
using android::base::StdioStream;
using android::base::System;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

using namespace gltest;

class TextureDrawTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
        EXPECT_TRUE(egl != nullptr);
        EXPECT_TRUE(gl != nullptr);

        m_display = getDisplay();
        m_config = createConfig(m_display, 8, 8, 8, 8, 24, 8, 0);
        m_surface = pbufferSurface(m_display, m_config, 32, 32);
        egl->eglSetMaxGLESVersion(3);
        m_context = createContext(m_display, m_config, 3, 0);
        egl->eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    }

    virtual void TearDown() {
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        egl->eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                            EGL_NO_CONTEXT);
        destroyContext(m_display, m_context);
        destroySurface(m_display, m_surface);
        destroyDisplay(m_display);
    }

    EGLDisplay m_display;
    EGLConfig m_config;
    EGLSurface m_surface;
    EGLContext m_context;
};

TEST_F(TextureDrawTest, Basic) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();

    GLuint textureToDraw;

    gl->glGenTextures(1, &textureToDraw);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, textureToDraw);

    gl->glPixelStorei(GL_PACK_ALIGNMENT, 1);
    gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->glViewport(0, 0, 32, 32);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::vector<unsigned char> pixels(32 * 32 * 4);
    std::vector<unsigned char> pixelsOut(32 * 32 * 4, 0xff);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            pixels[i * 32 * 4 + j * 4 + 0] = 0xaa;
            pixels[i * 32 * 4 + j * 4 + 1] = 0x00;
            pixels[i * 32 * 4 + j * 4 + 2] = 0x11;
            pixels[i * 32 * 4 + j * 4 + 3] = 0xff;
        }
    }

    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLint fbStatus = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "%s: incomplete fb 0x%x\n", __func__, fbStatus);
    }

    TextureDraw textureDraw;

    textureDraw.draw(textureToDraw, 0, 0, 0);

    gl->glReadPixels(0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, pixelsOut.data());

    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 1; j++) {
            fprintf(stderr, "%s: 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n",
                    __func__,
            pixelsOut[i * 32 * 4 + j * 4 + 0],
            pixelsOut[i * 32 * 4 + j * 4 + 1],
            pixelsOut[i * 32 * 4 + j * 4 + 2],
            pixelsOut[i * 32 * 4 + j * 4 + 3]);

            EXPECT_EQ(pixels[i * 32 * 4 + j * 4 + 0], pixelsOut[i * 32 * 4 + j * 4 + 0]);
            EXPECT_EQ(pixels[i * 32 * 4 + j * 4 + 1], pixelsOut[i * 32 * 4 + j * 4 + 1]);
            EXPECT_EQ(pixels[i * 32 * 4 + j * 4 + 2], pixelsOut[i * 32 * 4 + j * 4 + 2]);
            EXPECT_EQ(pixels[i * 32 * 4 + j * 4 + 3], pixelsOut[i * 32 * 4 + j * 4 + 3]);
        }
    }
}


}  // namespace emugl
