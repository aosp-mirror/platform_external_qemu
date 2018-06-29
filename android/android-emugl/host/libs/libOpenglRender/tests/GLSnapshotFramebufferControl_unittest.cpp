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

#include <gtest/gtest.h>

namespace emugl {

class SnapshotGlColorMaskTest
    : public SnapshotSetValueTest<std::vector<GLboolean>> {
    void stateCheck(std::vector<GLboolean> expected) override {
        EXPECT_TRUE(compareGlobalGlBooleanv(gl, GL_COLOR_WRITEMASK, expected));
    }
    void stateChange() override {
        std::vector<GLboolean> mask = *m_changed_value;
        gl->glColorMask(mask[0], mask[1], mask[2], mask[3]);
    }
};

TEST_F(SnapshotGlColorMaskTest, SetColorMask) {
    setExpectedValues({true, true, true, true}, {false, false, false, false});
    doCheckedSnapshot();
}

class SnapshotGlDepthMaskTest : public SnapshotSetValueTest<GLboolean> {
    void stateCheck(GLboolean expected) override {
        EXPECT_TRUE(compareGlobalGlBoolean(gl, GL_DEPTH_WRITEMASK, expected));
    }
    void stateChange() override { gl->glDepthMask(*m_changed_value); }
};

TEST_F(SnapshotGlDepthMaskTest, SetDepthMask) {
    setExpectedValues(true, false);
    doCheckedSnapshot();
}

// not to be confused with GL_STENCIL_VALUE_MASK
class SnapshotGlStencilWriteMaskTest : public SnapshotSetValueTest<GLuint> {
    void stateCheck(GLuint expected) override {
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_STENCIL_WRITEMASK, expected));
        EXPECT_TRUE(
                compareGlobalGlInt(gl, GL_STENCIL_BACK_WRITEMASK, expected));
    }
    void stateChange() override { gl->glStencilMask(*m_changed_value); }
};

TEST_F(SnapshotGlStencilWriteMaskTest, SetStencilMask) {
    // different drivers act differently; get the default mask
    GLint defaultWriteMask;
    gl->glGetIntegerv(GL_STENCIL_WRITEMASK, &defaultWriteMask);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

    setExpectedValues(defaultWriteMask, 0);
    doCheckedSnapshot();
}

class SnapshotGlColorClearValueTest
    : public SnapshotSetValueTest<std::vector<GLclampf>> {
    void stateCheck(std::vector<GLclampf> expected) override {
        EXPECT_TRUE(compareGlobalGlFloatv(gl, GL_COLOR_CLEAR_VALUE, expected));
    }
    void stateChange() override {
        std::vector<GLclampf> color = *m_changed_value;
        gl->glClearColor(color[0], color[1], color[2], color[3]);
    }
};

TEST_F(SnapshotGlColorClearValueTest, SetClearColor) {
    setExpectedValues({0, 0, 0, 0}, {1.0f, 0.2f, 0.7f, 0.8f});
    doCheckedSnapshot();
}

class SnapshotGlDepthClearValueTest : public SnapshotSetValueTest<GLclampf> {
    void stateCheck(GLclampf expected) override {
        EXPECT_TRUE(compareGlobalGlFloat(gl, GL_DEPTH_CLEAR_VALUE, expected));
    }
    void stateChange() override { gl->glClearDepthf(*m_changed_value); }
};

TEST_F(SnapshotGlDepthClearValueTest, SetClearDepth) {
    setExpectedValues(1.0f, 0.4f);
    doCheckedSnapshot();
}

class SnapshotGlStencilClearValueTest : public SnapshotSetValueTest<GLint> {
    void stateCheck(GLint expected) override {
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_STENCIL_CLEAR_VALUE, expected));
    }
    void stateChange() override { gl->glClearStencil(*m_changed_value); }
};

TEST_F(SnapshotGlStencilClearValueTest, SetClearStencil) {
    setExpectedValues(0, 3);
    doCheckedSnapshot();
}

}  // namespace emugl
