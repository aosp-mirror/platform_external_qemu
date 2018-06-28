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

// Line width settings to attempt
static const GLfloat kGLES2TestLineWidths[] = {2.0f};

// Polygon offset settings to attempt
static const GLfloat kGLES2TestPolygonOffset[] = {0.5f, 0.5f};

class SnapshotGlLineWidthTest : public SnapshotSetValueTest<GLfloat>,
                                public ::testing::WithParamInterface<GLfloat> {
    void stateCheck(GLfloat expected) {
        GLfloat lineWidthRange[2];
        gl->glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);

        GLfloat lineWidth;
        gl->glGetFloatv(GL_LINE_WIDTH, &lineWidth);

        if (expected <= lineWidthRange[1]) {
            EXPECT_EQ(expected, lineWidth);
        }
    }
    void stateChange() override { gl->glLineWidth(GetParam()); }
};

TEST_P(SnapshotGlLineWidthTest, SetLineWidth) {
    setExpectedValues(1.0f, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotRasterization,
                        SnapshotGlLineWidthTest,
                        ::testing::ValuesIn(kGLES2TestLineWidths));

class SnapshotGlCullFaceTest : public SnapshotSetValueTest<GLenum>,
                               public ::testing::WithParamInterface<GLenum> {
    void stateCheck(GLenum expected) {
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_CULL_FACE_MODE, expected));
    }
    void stateChange() override { gl->glCullFace(GetParam()); }
};

TEST_P(SnapshotGlCullFaceTest, SetCullFaceMode) {
    setExpectedValues(GL_BACK, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotRasterization,
                        SnapshotGlCullFaceTest,
                        ::testing::ValuesIn(kGLES2CullFaceModes));

class SnapshotGlFrontFaceTest : public SnapshotSetValueTest<GLenum>,
                                public ::testing::WithParamInterface<GLenum> {
    void stateCheck(GLenum expected) {
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_FRONT_FACE, expected));
    }
    void stateChange() override { gl->glFrontFace(GetParam()); }
};

TEST_P(SnapshotGlFrontFaceTest, SetFrontFaceMode) {
    setExpectedValues(GL_CCW, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotRasterization,
                        SnapshotGlFrontFaceTest,
                        ::testing::ValuesIn(kGLES2FrontFaceModes));

class SnapshotGlPolygonOffsetTest
    : public SnapshotSetValueTest<GLfloat*>,
      public ::testing::WithParamInterface<const GLfloat*> {
    void stateCheck(GLfloat* expected) {
        EXPECT_TRUE(compareGlobalGlFloat(gl, GL_POLYGON_OFFSET_FACTOR,
                                         expected[0]));
        EXPECT_TRUE(
                compareGlobalGlFloat(gl, GL_POLYGON_OFFSET_UNITS, expected[1]));
    }
    void stateChange() override {
        gl->glPolygonOffset(GetParam()[0], GetParam()[1]);
    }
};

TEST_P(SnapshotGlPolygonOffsetTest, SetPolygonOffset) {
    GLfloat defaultOffset[2] = {0.0f, 0.0f};
    GLfloat testOffset[2] = {GetParam()[0], GetParam()[1]};
    setExpectedValues(defaultOffset, testOffset);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotRasterization,
                        SnapshotGlPolygonOffsetTest,
                        ::testing::Values(kGLES2TestPolygonOffset));

}  // namespace emugl
