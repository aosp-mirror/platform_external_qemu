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
static const std::vector<GLint> kGLES2TestViewport = {10, 10, 100, 100};

// Depth range settings to attempt
static const std::vector<GLclampf> kGLES2TestDepthRange = {0.2f, 0.8f};

class SnapshotGlViewportTest
    : public SnapshotPreserveTest,
      public ::testing::WithParamInterface<std::vector<GLint>> {
    void defaultStateCheck() override {
        GLint viewport[4] = {};
        gl->glGetIntegerv(GL_VIEWPORT, viewport);
        // initial viewport should match surface size
        EXPECT_EQ(kTestSurfaceSize[0], viewport[2]);
        EXPECT_EQ(kTestSurfaceSize[1], viewport[3]);
    }
    void changedStateCheck() override {
        EXPECT_TRUE(compareGlobalGlIntv(gl, GL_VIEWPORT, GetParam()));
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
    : public SnapshotSetValueTest<std::vector<GLclampf>>,
      public ::testing::WithParamInterface<std::vector<GLclampf>> {
    void stateCheck(std::vector<GLclampf> expected) override {
        EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_DEPTH_RANGE, expected));
    }
    void stateChange() override {
        std::vector<GLclampf> testRange = GetParam();
        gl->glDepthRangef(testRange[0], testRange[1]);
    }
};

TEST_P(SnapshotGlDepthRangeTest, SetDepthRange) {
    std::vector<GLclampf> defaultRange = {0.0f, 1.0f};
    setExpectedValues(defaultRange, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotTransformation,
                        SnapshotGlDepthRangeTest,
                        ::testing::Values(kGLES2TestDepthRange));

}  // namespace emugl
