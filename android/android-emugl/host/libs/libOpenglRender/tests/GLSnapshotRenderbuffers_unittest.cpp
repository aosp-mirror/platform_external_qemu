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

struct GlRenderbufferFormat {
    GLenum name = GL_RGBA4;
    GLsizei red;
    GLsizei blue;
    GLsizei green;
    GLsizei alpha;
    GLsizei depth;
    GLsizei stencil;
};

struct GlRenderbufferState {
    GLuint width;
    GLuint height;
    GlRenderbufferFormat format = {};
};

static const GlRenderbufferFormat kGLES2RenderbufferFormatFieldSizes[] = {
        {GL_DEPTH_COMPONENT16, 0, 0, 0, 16, 0}, {GL_RGBA4, 4, 4, 4, 4, 0, 0},
        {GL_RGB5_A1, 5, 5, 5, 1, 0, 0},         {GL_RGB565, 5, 6, 5, 0, 0, 0},
        {GL_STENCIL_INDEX8, 0, 0, 0, 0, 0, 8},
};

class SnapshotGlRenderbufferTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        EXPECT_EQ(GL_FALSE, gl->glIsRenderbuffer(m_renderbuffer_name));
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_RENDERBUFFER_BINDING, 0));
    }

    void changedStateCheck() override {
        EXPECT_EQ(GL_TRUE, gl->glIsRenderbuffer(m_renderbuffer_name));
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_RENDERBUFFER_BINDING,
                                       m_renderbuffer_name));

        EXPECT_TRUE(compareParameter(GL_RENDERBUFFER_WIDTH, m_state.width));
        EXPECT_TRUE(compareParameter(GL_RENDERBUFFER_HEIGHT, m_state.height));
        EXPECT_TRUE(compareParameter(GL_RENDERBUFFER_INTERNAL_FORMAT,
                                     m_state.format.name));
        EXPECT_TRUE(
                compareParameter(GL_RENDERBUFFER_RED_SIZE, m_state.format.red));
        EXPECT_TRUE(compareParameter(GL_RENDERBUFFER_GREEN_SIZE,
                                     m_state.format.green));
        EXPECT_TRUE(compareParameter(GL_RENDERBUFFER_BLUE_SIZE,
                                     m_state.format.blue));
        EXPECT_TRUE(compareParameter(GL_RENDERBUFFER_ALPHA_SIZE,
                                     m_state.format.alpha));
        EXPECT_TRUE(compareParameter(GL_RENDERBUFFER_DEPTH_SIZE,
                                     m_state.format.depth));
        EXPECT_TRUE(compareParameter(GL_RENDERBUFFER_STENCIL_SIZE,
                                     m_state.format.stencil));
    }

    void stateChange() override {
        gl->glGenRenderbuffers(1, &m_renderbuffer_name);
        gl->glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer_name);

        m_state_changer();
    }

    void setStateChanger(std::function<void()> changer) {
        m_state_changer = changer;
    }

protected:
    testing::AssertionResult compareParameter(GLenum name, GLint expected) {
        GLint actual;
        gl->glGetRenderbufferParameteriv(GL_RENDERBUFFER, name, &actual);
        if (expected != actual) {
            return testing::AssertionFailure()
                   << "GL Renderbuffer object parameter mismatch for " << name
                   << ":\n\tvalue was:\t" << actual << "\n\t expected:\t"
                   << expected << "\t";
        }
        return testing::AssertionSuccess();
    }

    GLuint m_renderbuffer_name = 0;
    GlRenderbufferState m_state = {};
    std::function<void()> m_state_changer = [] {};
};

TEST_F(SnapshotGlRenderbufferTest, CreateAndBind) {
    doCheckedSnapshot();
}

class SnapshotGlRenderbufferFormatTest
    : public SnapshotGlRenderbufferTest,
      public ::testing::WithParamInterface<GlRenderbufferState> {};

TEST_P(SnapshotGlRenderbufferFormatTest, SetFormat) {
    setStateChanger([this] {
        GLint maxSize;
        gl->glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxSize);
        if (maxSize < GetParam().width || maxSize < GetParam().height) {
            fprintf(stderr,
                    "max renderbuffer size too small for test values\n");
        }
        gl->glRenderbufferStorage(GL_RENDERBUFFER, GetParam().format.name,
                                  GetParam().width, GetParam().height);
    });
    doCheckedSnapshot();
}

}  // namespace emugl
