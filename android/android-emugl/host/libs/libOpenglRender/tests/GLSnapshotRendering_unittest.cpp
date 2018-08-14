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

#include "GLSnapshotTestDispatch.h"
#include "GLSnapshotTesting.h"
#include "Standalone.h"
#include "samples/HelloTriangle.h"

#include <gtest/gtest.h>

namespace emugl {

TEST(SnapshotGlRenderingSampleTest, OverrideDispatch) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();
    const GLESv2Dispatch* testGl = getSnapshotTestDispatch();
    EXPECT_NE(nullptr, gl);
    EXPECT_NE(nullptr, testGl);
    EXPECT_NE(gl->glDrawArrays, testGl->glDrawArrays);
    EXPECT_NE(gl->glDrawElements, testGl->glDrawElements);
}

class SnapshotTestTriangle : public HelloTriangle {
public:
    virtual ~SnapshotTestTriangle() {}

    void drawLoop() {
        this->initialize();
        while (mFrameCount < 5) {
            this->draw();
            mFrameCount++;
            mFb->flushWindowSurfaceColorBuffer(mSurface);
            if (mUseSubWindow) {
                mFb->post(mColorBuffer);
                mWindow->messageLoop();
            }
        }
    }

protected:
    const GLESv2Dispatch* getGlDispatch() { return getSnapshotTestDispatch(); }

    int mFrameCount = 0;
};

template <typename T>
class SnapshotGlRenderingSampleTest : public ::testing::Test {
protected:
    virtual void SetUp() override { mApp.reset(new T()); }

    virtual void TearDown() override {
        mApp.reset();
        EXPECT_EQ(EGL_SUCCESS, LazyLoadedEGLDispatch::get()->eglGetError())
                << "SnapshotGlRenderingSampleTest TearDown found an EGL error";
    }

    std::unique_ptr<T> mApp;
};

// To test with additional SampleApplications, extend them to override drawLoop
// and getGlDispatch, then add the type to TestSampleApps.
using TestSampleApps = ::testing::Types<SnapshotTestTriangle>;
TYPED_TEST_CASE(SnapshotGlRenderingSampleTest, TestSampleApps);

TYPED_TEST(SnapshotGlRenderingSampleTest, SnapshotDrawOnce) {
    this->mApp->drawOnce();
}

TYPED_TEST(SnapshotGlRenderingSampleTest, SnapshotDrawLoop) {
    this->mApp->drawLoop();
}

}  // namespace emugl
