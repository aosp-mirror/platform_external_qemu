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
    GlValues values;
};

static const GlVertexAttrib kGLES2DefaultVertexAttrib = {
        .size = 4,
        .type = GL_FLOAT,
        .normalized = GL_FALSE,
        .stride = 0,
        .enabled = GL_FALSE,
        .pointer = NULL,
        .values = {.ints = {}, .floats = {0, 0, 0, 1}}};

static const GlVertexAttrib kTestVertexAttrib = {
        .size = 2,
        .type = GL_FLOAT,
        .normalized = GL_TRUE,
        .stride = 8,
        .enabled = GL_TRUE,
        .pointer = NULL,
        .values = {.ints = {}, .floats = {.1, .3, .9, .5}}};

class SnapshotGlVertexAttributesTest
    : public SnapshotSetValueTest<GlVertexAttrib> {
public:
    void stateCheck(GlVertexAttrib expected) override {
        // check parameters
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_SIZE, &expected.size);
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_TYPE, (GLint*)&expected.type);
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,
                          (GLint*)&expected.normalized);
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_STRIDE, &expected.stride);
        checkIntParameter(GL_VERTEX_ATTRIB_ARRAY_ENABLED,
                          (GLint*)&expected.enabled);

        // check element value
        switch (expected.type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_FIXED:
                if (expected.values.ints.size() < expected.size) {
                    FAIL() << "Not enough int values provided.";
                }
                checkIntParameter(GL_CURRENT_VERTEX_ATTRIB,
                                  (GLint*)&expected.values.ints[0],
                                  expected.size);
                break;
            case GL_FLOAT:
                if (expected.values.floats.size() < expected.size) {
                    FAIL() << "Not enough float values provided.";
                }
                checkFloatParameter(GL_CURRENT_VERTEX_ATTRIB,
                                    (GLfloat*)&expected.values.floats[0],
                                    expected.size);
                break;
            default:
                ADD_FAILURE() << "Unexpected type " << expected.type
                              << " for vertex attribute " << m_index;
        }
    }

    void stateChange() override {
        // set parameters
        GlVertexAttrib changed = *m_changed_value;
        gl->glVertexAttribPointer(m_index, changed.size, changed.type,
                                  changed.normalized, changed.stride,
                                  changed.pointer);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        if (changed.enabled) {
            gl->glEnableVertexAttribArray(m_index);
        } else {
            gl->glDisableVertexAttribArray(m_index);
        }
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        // set element value
        switch (changed.type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_FIXED:
                if (changed.values.ints.size() < changed.size) {
                    FAIL() << "Not enough int values provided.";
                }
                // TODO(benzene): support GLES3+
                FAIL() << "GLES2 only supports float vertex attributes "
                          "(VertexAttrib{1234}f).";
            case GL_FLOAT:
                if (changed.values.floats.size() < changed.size) {
                    FAIL() << "Not enough float values provided.";
                }
                switch (changed.size) {
                    case 1:
                        gl->glVertexAttrib1fv(
                                m_index, (GLfloat*)&changed.values.floats[0]);
                        break;
                    case 2:
                        gl->glVertexAttrib2fv(
                                m_index, (GLfloat*)&changed.values.floats[0]);
                        break;
                    case 3:
                        gl->glVertexAttrib3fv(
                                m_index, (GLfloat*)&changed.values.floats[0]);
                        break;
                    case 4:
                        gl->glVertexAttrib4fv(
                                m_index, (GLfloat*)&changed.values.floats[0]);
                        break;
                    default:
                        ADD_FAILURE() << "Unsupported size " << changed.size
                                      << " for vertex attribute " << m_index;
                }
                break;
            default:
                ADD_FAILURE() << "Unsupported type " << changed.type
                              << " for vertex attribute " << m_index;
        }
    }

    void selectIndex(GLuint index) {
        GLint maxAttribs;
        gl->glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        if (index >= maxAttribs) {
            fprintf(stderr,
                    "cannot select index %d: GL_MAX_VERTEX_ATTRIBS is %d.\n",
                    index, maxAttribs);
            return;
        }
        m_index = index;
    }

protected:
    void checkFloatParameter(GLenum paramName,
                             GLfloat* expected,
                             GLuint size = 1) {
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
    selectIndex(31);
    setExpectedValues(kGLES2DefaultVertexAttrib, kTestVertexAttrib);
    doCheckedSnapshot();
}

}  // namespace emugl
