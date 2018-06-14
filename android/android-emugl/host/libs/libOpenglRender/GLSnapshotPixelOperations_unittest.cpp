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

// Scissor box settings to attempt
static const GLint kGLES2TestScissorBox[] = {2, 3, 10, 20};

// Stencil reference value to attempt
static const GLint kGLES2TestStencilRef = 1;

// Stencil default mask; max GLuint is clamped to a signed GLint by GetInteger?
static const GLuint kGLES2StencilDefaultMask = 0x7FFFFFFF;

// Stencil mask values to attempt
static const GLuint kGLES2TestStencilMasks[] = {0, 1, 0x1000000, 0x7FFFFFFF};

class SnapshotGlScissorBoxTest
    : public SnapshotSetValueTest<GLint*>,
      public ::testing::WithParamInterface<const GLint*> {
    void stateCheck(GLint* expected) override {
        GLint box[4] = {};
        gl->glGetIntegerv(GL_SCISSOR_BOX, box);
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ(expected[i], box[i]);
        }
    }
    void stateChange() override {
        gl->glScissor(GetParam()[0], GetParam()[1], GetParam()[2],
                      GetParam()[3]);
    }
};

TEST_P(SnapshotGlScissorBoxTest, SetScissorBox) {
    GLint defaultViewport[] = {0, 0, gltest::kSurfaceSize[0],
                               gltest::kSurfaceSize[1]};
    GLint testViewport[] = {GetParam()[0], GetParam()[1], GetParam()[2],
                            GetParam()[3]};
    setExpectedValues(defaultViewport, testViewport);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlScissorBoxTest,
                        ::testing::Values(kGLES2TestScissorBox));

// Tests preservation of stencil test conditional state, set by glStencilFunc.
class SnapshotGlStencilConditionsTest
    : public SnapshotSetValueTest<GlStencilFunc> {
    void stateCheck(GlStencilFunc expected) {
        GLint frontFunc, frontRef, frontMask, backFunc, backRef, backMask;
        gl->glGetIntegerv(GL_STENCIL_FUNC, &frontFunc);
        gl->glGetIntegerv(GL_STENCIL_REF, &frontRef);
        gl->glGetIntegerv(GL_STENCIL_VALUE_MASK, &frontMask);
        gl->glGetIntegerv(GL_STENCIL_BACK_FUNC, &backFunc);
        gl->glGetIntegerv(GL_STENCIL_BACK_REF, &backRef);
        gl->glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &backMask);
        EXPECT_EQ(expected.func, frontFunc);
        EXPECT_EQ(expected.ref, frontRef);
        EXPECT_EQ(expected.mask, frontMask);
        EXPECT_EQ(expected.func, backFunc);
        EXPECT_EQ(expected.ref, backRef);
        EXPECT_EQ(expected.mask, backMask);
    }
    void stateChange() override {
        GlStencilFunc sFunc = *m_changed_value;
        gl->glStencilFunc(sFunc.func, sFunc.ref, sFunc.mask);
    }
};

class SnapshotGlStencilFuncTest : public SnapshotGlStencilConditionsTest,
                                  public ::testing::WithParamInterface<GLenum> {
};

class SnapshotGlStencilMaskTest : public SnapshotGlStencilConditionsTest,
                                  public ::testing::WithParamInterface<GLuint> {
};

TEST_P(SnapshotGlStencilFuncTest, SetStencilFunc) {
    GlStencilFunc defaultStencilFunc = {GL_ALWAYS, 0, kGLES2StencilDefaultMask};
    GlStencilFunc testStencilFunc = {GetParam(), kGLES2TestStencilRef, 0};
    setExpectedValues(defaultStencilFunc, testStencilFunc);
    doCheckedSnapshot();
}

TEST_P(SnapshotGlStencilMaskTest, SetStencilMask) {
    GlStencilFunc defaultStencilFunc = {GL_ALWAYS, 0, kGLES2StencilDefaultMask};
    GlStencilFunc testStencilFunc = {GL_ALWAYS, kGLES2TestStencilRef,
                                     GetParam()};
    setExpectedValues(defaultStencilFunc, testStencilFunc);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilFuncTest,
                        ::testing::ValuesIn(kGLES2StencilFuncs));

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilMaskTest,
                        ::testing::ValuesIn(kGLES2TestStencilMasks));

class SnapshotGlStencilConsequenceTest
    : public SnapshotSetValueTest<GlStencilOp> {
    void stateCheck(GlStencilOp expected) override {
        GLint frontFail, frontDepthFail, frontDepthPass, backFail,
                backDepthFail, backDepthPass;
        gl->glGetIntegerv(GL_STENCIL_FAIL, &frontFail);
        gl->glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &frontDepthFail);
        gl->glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &frontDepthPass);
        gl->glGetIntegerv(GL_STENCIL_BACK_FAIL, &backFail);
        gl->glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &backDepthFail);
        gl->glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &backDepthPass);
        EXPECT_EQ(expected.sfail, frontFail);
        EXPECT_EQ(expected.dpfail, frontDepthFail);
        EXPECT_EQ(expected.dppass, frontDepthPass);
        EXPECT_EQ(expected.sfail, backFail);
        EXPECT_EQ(expected.dpfail, backDepthFail);
        EXPECT_EQ(expected.dppass, backDepthPass);
    }
    void stateChange() override {
        GlStencilOp sOp = *m_changed_value;
        gl->glStencilOp(sOp.sfail, sOp.dpfail, sOp.dppass);
    }
};

class SnapshotGlStencilFailTest : public SnapshotGlStencilConsequenceTest,
                                  public ::testing::WithParamInterface<GLenum> {
};

class SnapshotGlStencilDepthFailTest
    : public SnapshotGlStencilConsequenceTest,
      public ::testing::WithParamInterface<GLenum> {};

class SnapshotGlStencilDepthPassTest
    : public SnapshotGlStencilConsequenceTest,
      public ::testing::WithParamInterface<GLenum> {};

TEST_P(SnapshotGlStencilFailTest, SetStencilOps) {
    GlStencilOp defaultStencilOp = {GL_KEEP, GL_KEEP, GL_KEEP};
    GlStencilOp testStencilOp = {GetParam(), GL_KEEP, GL_KEEP};
    setExpectedValues(defaultStencilOp, testStencilOp);
    doCheckedSnapshot();
}

TEST_P(SnapshotGlStencilDepthFailTest, SetStencilOps) {
    GlStencilOp defaultStencilOp = {GL_KEEP, GL_KEEP, GL_KEEP};
    GlStencilOp testStencilOp = {GL_KEEP, GetParam(), GL_KEEP};
    setExpectedValues(defaultStencilOp, testStencilOp);
    doCheckedSnapshot();
}

TEST_P(SnapshotGlStencilDepthPassTest, SetStencilOps) {
    GlStencilOp defaultStencilOp = {GL_KEEP, GL_KEEP, GL_KEEP};
    GlStencilOp testStencilOp = {GL_KEEP, GL_KEEP, GetParam()};
    setExpectedValues(defaultStencilOp, testStencilOp);
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilFailTest,
                        ::testing::ValuesIn(kGLES2StencilOps));

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilDepthFailTest,
                        ::testing::ValuesIn(kGLES2StencilOps));

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlStencilDepthPassTest,
                        ::testing::ValuesIn(kGLES2StencilOps));

class SnapshotGlDepthFuncTest : public SnapshotSetValueTest<GLenum>,
                                public ::testing::WithParamInterface<GLenum> {
    void stateCheck(GLenum expected) override {
        GLint depthFunc;
        gl->glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
        EXPECT_EQ(expected, depthFunc);
    }
    void stateChange() { gl->glDepthFunc(*m_changed_value); }
};

TEST_P(SnapshotGlDepthFuncTest, SetDepthFunc) {
    setExpectedValues(GL_LESS, GetParam());
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotPixelOps,
                        SnapshotGlDepthFuncTest,
                        ::testing::ValuesIn(kGLES2StencilFuncs));

}  // namespace emugl
