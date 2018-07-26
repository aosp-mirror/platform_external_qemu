// Copyright (C) 2018 The Android Open Source Project
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

#include "OpenGLTestContext.h"

#include <gtest/gtest.h>

namespace emugl {

TEST_F(GLTest, InitDestroy) {}

TEST_F(GLTest, SetUpMulticontext) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
    EXPECT_TRUE(egl != nullptr);
    EXPECT_TRUE(gl != nullptr);

    egl->eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                        EGL_NO_CONTEXT);
    EGLSurface duoSurface = pbufferSurface(
            m_display, m_config, kTestSurfaceSize[0], kTestSurfaceSize[1]);
    EGLContext duoContext = createContext(m_display, m_config, 3, 0);

    egl->eglMakeCurrent(m_display, duoSurface, duoSurface, duoContext);
    EXPECT_EQ(EGL_SUCCESS, egl->eglGetError());

    destroyContext(m_display, duoContext);
    destroySurface(m_display, duoSurface);

    egl->eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    EXPECT_EQ(EGL_SUCCESS, egl->eglGetError());
}

}  // namespace emugl
