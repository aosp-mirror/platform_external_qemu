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

#include "GLSnapshotTestStateUtils.h"
#include "GLSnapshotTesting.h"
#include "OpenGLTestContext.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace emugl {

enum class GlVertexAttribMode { SingleValue = 0, Array = 1, Buffer = 2 };

struct GlVertexAttrib {
    GlVertexAttribMode mode;
    GlValues values;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    GLboolean enabled;
    GLvoid* pointer;
    GLuint bufferBinding;
};

static const GlVertexAttrib kGLES2DefaultVertexAttrib = {
        .mode = GlVertexAttribMode::SingleValue,
        .values = {.ints = {}, .floats = {0, 0, 0, 1}},
        .size = 4,
        .type = GL_FLOAT,
        .normalized = GL_FALSE,
        .stride = 0,
        .enabled = GL_FALSE,
        .pointer = nullptr,
        .bufferBinding = 0};

static const GlBufferData kTestAttachedBuffer = {.size = 16,
                                                 .bytes = nullptr,
                                                 .usage = GL_STATIC_DRAW};

class SnapshotGlVertexAttributesTest
    : public SnapshotSetValueTest<GlVertexAttrib> {
public:
    virtual void stateCheck(GlVertexAttrib expected) override {
        EXPECT_TRUE(compareIntParameter(GL_VERTEX_ATTRIB_ARRAY_ENABLED,
                                        expected.enabled));
    }

    virtual void stateChange() override {
        GlVertexAttrib changed = *m_changed_value;
        if (changed.enabled) {
            gl->glEnableVertexAttribArray(m_index);
        } else {
            gl->glDisableVertexAttribArray(m_index);
        }
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
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
    testing::AssertionResult compareFloatParameter(GLenum paramName,
                                                   GLfloat expected) {
        std::vector<GLfloat> v = {expected};
        return compareFloatParameter(paramName, v);
    }

    testing::AssertionResult compareFloatParameter(
            GLenum paramName,
            const std::vector<GLfloat>& expected) {
        std::vector<GLfloat> values;
        values.resize(std::max((GLuint)4, (GLuint)expected.size()));
        gl->glGetVertexAttribfv(m_index, paramName, &(values[0]));
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        return compareVector<GLfloat>(
                expected, values,
                "float(s) for parameter " + describeGlEnum(paramName) +
                        " of vertex attribute " + std::to_string(m_index));
    }

    testing::AssertionResult compareIntParameter(GLenum paramName,
                                                 GLint expected) {
        std::vector<GLint> v = {expected};
        return compareIntParameter(paramName, v);
    }

    testing::AssertionResult compareIntParameter(
            GLenum paramName,
            const std::vector<GLint>& expected) {
        std::vector<GLint> values;
        values.resize(std::max((GLuint)4, (GLuint)expected.size()));
        gl->glGetVertexAttribiv(m_index, paramName, &(values[0]));
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        return compareVector<GLint>(
                expected, values,
                "int(s) for parameter " + describeGlEnum(paramName) +
                        " of vertex attribute " + std::to_string(m_index));
    }

    GLuint m_index = 0;
};

class SnapshotGlVertexAttribSingleValueTest
    : public SnapshotGlVertexAttributesTest {
public:
    void stateCheck(GlVertexAttrib expected) override {
        SnapshotGlVertexAttributesTest::stateCheck(expected);

        // check current element value
        switch (expected.type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_FIXED:
                EXPECT_TRUE(compareIntParameter(GL_CURRENT_VERTEX_ATTRIB,
                                                expected.values.ints));
                break;
            case GL_FLOAT:
                EXPECT_TRUE(compareFloatParameter(GL_CURRENT_VERTEX_ATTRIB,
                                                  expected.values.floats));
                break;
            default:
                ADD_FAILURE() << "Unexpected type " << expected.type
                              << " for vertex attribute " << m_index;
        }
    }

    void stateChange() override {
        SnapshotGlVertexAttributesTest::stateChange();
        GlVertexAttrib changed = *m_changed_value;
        switch (changed.type) {
            case GL_BYTE:
            case GL_UNSIGNED_BYTE:
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
            case GL_FIXED:
                // TODO(benzene): support GLES3+
                FAIL() << "GLES2 only supports float vertex attributes "
                          "(VertexAttrib{1234}f).";
            case GL_FLOAT:
                switch (changed.values.floats.size()) {
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
};

class SnapshotGlVertexAttribArrayTest : public SnapshotGlVertexAttributesTest {
public:
    virtual void stateCheck(GlVertexAttrib expected) override {
        SnapshotGlVertexAttributesTest::stateCheck(expected);
        // check parameters
        EXPECT_TRUE(compareIntParameter(GL_VERTEX_ATTRIB_ARRAY_SIZE,
                                        expected.size));
        EXPECT_TRUE(compareIntParameter(GL_VERTEX_ATTRIB_ARRAY_TYPE,
                                        expected.type));
        EXPECT_TRUE(compareIntParameter(GL_VERTEX_ATTRIB_ARRAY_STRIDE,
                                        expected.stride));
        EXPECT_TRUE(compareIntParameter(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,
                                        expected.normalized));
        EXPECT_TRUE(compareIntParameter(GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
                                        expected.bufferBinding));

        GLvoid* pointer;
        gl->glGetVertexAttribPointerv(m_index, GL_VERTEX_ATTRIB_ARRAY_POINTER,
                                      &pointer);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        EXPECT_EQ(expected.pointer, pointer);
    }

    virtual void stateChange() override {
        SnapshotGlVertexAttributesTest::stateChange();
        GlVertexAttrib changed = *m_changed_value;
        gl->glVertexAttribPointer(m_index, changed.size, changed.type,
                                  changed.normalized, changed.stride,
                                  changed.pointer);
    }
};

class SnapshotGlVertexAttribBufferTest
    : public SnapshotGlVertexAttribArrayTest {
public:
    void stateCheck(GlVertexAttrib expected) override {
        SnapshotGlVertexAttribArrayTest::stateCheck(expected);
    }

    void stateChange() override {
        GlVertexAttrib changed = *m_changed_value;

        // Set up buffer to be bound before glVertexAttribPointer,
        // which will copy ARRAY_BUFFER_BINDING into the attrib's binding
        if (gl->glIsBuffer(changed.bufferBinding) == GL_TRUE) {
            gl->glBindBuffer(GL_ARRAY_BUFFER, changed.bufferBinding);
            EXPECT_EQ(GL_NO_ERROR, gl->glGetError())
                    << "Failed to bind buffer " << changed.bufferBinding;
            GLint bindresult;
            gl->glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bindresult);
            EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        } else {
            ADD_FAILURE() << "Tried to bind buffer with vertex attributes but "
                          << changed.bufferBinding << " is not a valid buffer.";
        }

        SnapshotGlVertexAttribArrayTest::stateChange();

        if (changed.bufferBinding != 0) {
            // Clear the array buffer binding
            gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
            EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

            GLint bindresult;
            gl->glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bindresult);
            EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        }
    }
};

TEST_F(SnapshotGlVertexAttribSingleValueTest, PreserveCurrentFloatAttrib) {
    selectIndex(31);
    GlVertexAttrib testAttrib = kGLES2DefaultVertexAttrib;
    testAttrib.values = {.ints = {}, .floats = {.1, .3}},
    setExpectedValues(kGLES2DefaultVertexAttrib, testAttrib);
    doCheckedSnapshot();
}

TEST_F(SnapshotGlVertexAttribArrayTest, DISABLED_PreserveArrayProperties) {
    selectIndex(5);
    GLfloat testArrayContents[] = {2.1f, 2.2f, 2.3f, 2.4f, 2.5f, 2.6f};
    GlVertexAttrib arrayAttrib = kGLES2DefaultVertexAttrib;
    arrayAttrib.mode = GlVertexAttribMode::Array;
    arrayAttrib.size = 3;
    arrayAttrib.stride = sizeof(GLfloat) * 3;
    arrayAttrib.normalized = GL_TRUE;
    arrayAttrib.enabled = GL_TRUE;
    arrayAttrib.pointer = testArrayContents;
    setExpectedValues(kGLES2DefaultVertexAttrib, arrayAttrib);
    doCheckedSnapshot();
}

TEST_F(SnapshotGlVertexAttribBufferTest, AttachArrayBuffer) {
    selectIndex(15);
    GLfloat testBuffContents[] = {
            0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f,
            0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f,
    };
    GlBufferData data = kTestAttachedBuffer;
    data.bytes = testBuffContents;
    GLuint buffer = createBuffer(gl, data);
    GlVertexAttrib withBuffer = kGLES2DefaultVertexAttrib;
    withBuffer.mode = GlVertexAttribMode::Buffer;
    withBuffer.enabled = GL_TRUE;
    withBuffer.pointer = reinterpret_cast<GLvoid*>(2);  // offset
    withBuffer.bufferBinding = buffer;
    setExpectedValues(kGLES2DefaultVertexAttrib, withBuffer);
    doCheckedSnapshot();
}

}  // namespace emugl
