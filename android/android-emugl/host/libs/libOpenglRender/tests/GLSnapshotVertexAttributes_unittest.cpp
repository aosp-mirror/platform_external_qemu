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

struct GlVertexAttrib {
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    GLboolean enabled;
    GLvoid* pointer;
};

static const GlVertexAttrib kGLES2DefaultVertexAttrib = {
    .size = 4, .type = GL_FLOAT, .normalized = GL_FALSE, .stride = 0, .enabled = GL_FALSE, .pointer = NULL
};

static const GlVertexAttrib kTestVertexAttrib = {
    .size = 2, .type = GL_SHORT, .normalized = GL_TRUE, .stride = 8, .enabled = GL_TRUE, .pointer = NULL
};

class SnapshotGlVertexAttributesTest : public SnapshotSetValueTest<GlVertexAttrib> {
public:
    void stateCheck(GlVertexAttrib expected) override {
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_SIZE, &expected.size);
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_TYPE, (GLint*)&expected.type);
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, (GLint*)&expected.normalized);
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_STRIDE, &expected.stride);
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_ENABLED, (GLint*)&expected.enabled);

        switch (expected.type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_FIXED:
                //checkIntParameter();
                break;
            case GL_FLOAT:
                //checkFloatParameter();
                break;
            default:
                ADD_FAILURE() << "Unexpected type " << expected.type << " for vertex attribute " << m_index;
        }
        // TODO(benzene) check other params
    }
    void stateChange() override {
        GlVertexAttrib c = *m_changed_value;
        gl->glVertexAttribPointer(m_index, c.size, c.type, c.normalized, c.stride, c.pointer);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        if (c.enabled) {
            gl->glEnableVertexAttribArray(m_index);
        }
        else {
            gl->glDisableVertexAttribArray(m_index);
        }
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        // TODO(benzene) do more setting
    }
protected:
    void selectIndex(GLuint index) {
        GLint maxAttribs;
        gl->glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        ASSERT_LT(m_index, maxAttribs);
        m_index = index;
    }

    void checkFloatParameter(GLenum paramName, GLfloat* expected, GLuint size = 1) {
        std::vector<GLfloat> values;
        values.resize(size);
        gl->glGetVertexAttribfv(m_index, paramName, &(values[0]));
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        for (int i = 0; i < size; ++i) {
            EXPECT_EQ(expected[i], values[i]);
        }
    }

    void checkIntParameter(GLenum paramName, GLint* expected, GLuint size = 1) {
        std::vector<GLint> values;
        values.resize(size);
        gl->glGetVertexAttribiv(m_index, paramName, &(values[0]));
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        for (int i = 0; i < size; ++i) {
            EXPECT_EQ(expected[i], values[i]);
        }
    }

    GLuint m_index = 0;
};

TEST_F(SnapshotGlVertexAttributesTest, PreserveVertexAttributes) {
    setExpectedValues(kGLES2DefaultVertexAttrib, kTestVertexAttrib);
    doCheckedSnapshot();
}

}  // namespace emugl
