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

#include "GLSnapshotTesting.h"
#include "OpenGLTestContext.h"

#include <gtest/gtest.h>

namespace emugl {

// Viewport settings to attempt
static const GLint kGLES2TestViewport[] = {10, 10, 100, 100};

// Depth range settings to attempt
static const GLclampf kGLES2TestDepthRange[] = {0.2f, 0.8f};

class SnapshotGlViewportTest
    : public SnapshotPreserveTest,
      public ::testing::WithParamInterface<const GLint*> {
    void defaultStateCheck() override {
        GLint viewport[4] = {};
        gl->glGetIntegerv(GL_VIEWPORT, viewport);
        // initial viewport should match surface size
        EXPECT_EQ(gltest::kSurfaceSize[0], viewport[2]);
        EXPECT_EQ(gltest::kSurfaceSize[1], viewport[3]);
    }
    void changedStateCheck() override {
        GLint viewport[4] = {};
        gl->glGetIntegerv(GL_VIEWPORT, viewport);
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ(GetParam()[i], viewport[i]);
        }
    }
    void stateChange() override {
        gl->glViewport(GetParam()[0], GetParam()[1], GetParam()[2],
                       GetParam()[3]);
    }
};

TEST_P(SnapshotGlViewportTest, SetViewport) {
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotTransformation,
                        SnapshotGlViewportTest,
                        ::testing::Values(kGLES2TestViewport));

class SnapshotGlDepthRangeTest
    : public SnapshotSetValueTest<GLclampf*>,
      public ::testing::WithParamInterface<const GLclampf*> {
    void stateCheck(GLclampf* expected) override {
        GLfloat depthRange[2] = {};
        gl->glGetFloatv(GL_DEPTH_RANGE, depthRange);
        EXPECT_EQ(expected[0], depthRange[0]);
        EXPECT_EQ(expected[1], depthRange[1]);
    }
    void stateChange() override {
        gl->glDepthRangef(GetParam()[0], GetParam()[1]);
    }
};

TEST_P(SnapshotGlDepthRangeTest, SetDepthRange) {
    GLclampf defaultRange[2] = {0.0f, 1.0f};
    GLclampf testRange[2] = {GetParam()[0], GetParam()[1]};
    setExpectedValues(defaultRange, testRange);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotTransformation,
                        SnapshotGlDepthRangeTest,
                        ::testing::Values(kGLES2TestDepthRange));

}  // namespace emugl
