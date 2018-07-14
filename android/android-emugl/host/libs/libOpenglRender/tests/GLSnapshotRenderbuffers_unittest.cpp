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
    GLenum name;
    GLsizei red;
    GLsizei green;
    GLsizei blue;
    GLsizei alpha;
    GLsizei depth;
    GLsizei stencil;
};

struct GlRenderbufferState {
    GLuint width;
    GLuint height;
    GlRenderbufferFormat format;
};

static const GlRenderbufferState kGLES2DefaultRenderbufferState = {
        0,
        0,
        {.name = GL_RGBA4}};

static const GlRenderbufferFormat kGLES2RenderbufferFormatFieldSizes[] = {
        {GL_DEPTH_COMPONENT16, 0, 0, 0, 0, 16, 0},
        {GL_RGBA4, 4, 4, 4, 4, 0, 0},
        {GL_RGB5_A1, 5, 5, 5, 1, 0, 0},
        {GL_RGB565, 5, 6, 5, 0, 0, 0},
        {GL_STENCIL_INDEX8, 0, 0, 0, 0, 0, 8},
};

static const GlRenderbufferState kGLES2TestRenderbufferStates[] = {
        {1, 1, kGLES2RenderbufferFormatFieldSizes[0]},
        {0, 3, kGLES2RenderbufferFormatFieldSizes[1]},
        {2, 2, kGLES2RenderbufferFormatFieldSizes[1]},
        {4, 4, kGLES2RenderbufferFormatFieldSizes[2]},
        {8, 8, kGLES2RenderbufferFormatFieldSizes[3]},
        {16, 16, kGLES2RenderbufferFormatFieldSizes[4]},
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
        return compareValue<GLint>(
                expected, actual,
                "GL Renderbuffer object mismatch for param " +
                        describeGlEnum(name) + ":");
    }

    GLuint m_renderbuffer_name = 0;
    GlRenderbufferState m_state = kGLES2DefaultRenderbufferState;
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
        m_state = GetParam();
        if (maxSize < m_state.width || maxSize < m_state.height) {
            fprintf(stderr,
                    "test dimensions exceed max renderbuffer size %d; "
                    "using max size instead\n",
                    maxSize);
            m_state.width = maxSize;
            m_state.height = maxSize;
        }
        gl->glRenderbufferStorage(GL_RENDERBUFFER, m_state.format.name,
                                  m_state.width, m_state.height);

        // The actual number of bits used for each format doesn't necessarily
        // match how they are defined.
        GLint fieldSize;
        gl->glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                         GL_RENDERBUFFER_RED_SIZE, &fieldSize);
        if (fieldSize != m_state.format.red) {
            fprintf(stderr,
                    "format 0x%x internal RED uses %d bits instead of %d\n",
                    m_state.format.name, fieldSize, m_state.format.red);
            m_state.format.red = fieldSize;
        }
        gl->glGetRenderbufferParameteriv(
                GL_RENDERBUFFER, GL_RENDERBUFFER_GREEN_SIZE, &fieldSize);
        if (fieldSize != m_state.format.green) {
            fprintf(stderr,
                    "format 0x%x internal GREEN uses %d bits instead of %d\n",
                    m_state.format.name, fieldSize, m_state.format.green);
            m_state.format.green = fieldSize;
        }
        gl->glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                         GL_RENDERBUFFER_BLUE_SIZE, &fieldSize);
        if (fieldSize != m_state.format.blue) {
            fprintf(stderr,
                    "format 0x%x internal BLUE uses %d bits instead of %d\n",
                    m_state.format.name, fieldSize, m_state.format.blue);
            m_state.format.blue = fieldSize;
        }
        gl->glGetRenderbufferParameteriv(
                GL_RENDERBUFFER, GL_RENDERBUFFER_ALPHA_SIZE, &fieldSize);
        if (fieldSize != m_state.format.alpha) {
            fprintf(stderr,
                    "format 0x%x internal ALPHA uses %d bits instead of %d\n",
                    m_state.format.name, fieldSize, m_state.format.alpha);
            m_state.format.alpha = fieldSize;
        }
        gl->glGetRenderbufferParameteriv(
                GL_RENDERBUFFER, GL_RENDERBUFFER_DEPTH_SIZE, &fieldSize);
        if (fieldSize != m_state.format.depth) {
            fprintf(stderr,
                    "format 0x%x internal DEPTH uses %d bits instead of %d\n",
                    m_state.format.name, fieldSize, m_state.format.depth);
            m_state.format.depth = fieldSize;
        }
        gl->glGetRenderbufferParameteriv(
                GL_RENDERBUFFER, GL_RENDERBUFFER_STENCIL_SIZE, &fieldSize);
        if (fieldSize != m_state.format.stencil) {
            fprintf(stderr,
                    "format 0x%x internal STENCIL uses %d bits instead of %d\n",
                    m_state.format.name, fieldSize, m_state.format.stencil);
            m_state.format.stencil = fieldSize;
        }
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    });
    doCheckedSnapshot();
}

INSTANTIATE_TEST_CASE_P(GLES2SnapshotRenderbuffers,
                        SnapshotGlRenderbufferFormatTest,
                        ::testing::ValuesIn(kGLES2TestRenderbufferStates));

}  // namespace emugl
