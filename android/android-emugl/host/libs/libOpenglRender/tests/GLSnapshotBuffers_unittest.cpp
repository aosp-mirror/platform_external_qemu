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

#include <map>

namespace emugl {

static const GLenum kGLES2GlobalBufferBindings[] = {
        GL_ARRAY_BUFFER_BINDING, GL_ELEMENT_ARRAY_BUFFER_BINDING};
// Buffers could also be bound with vertex attribute arrays.

class SnapshotGlBufferObjectsTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        for (GLenum bindTarget : kGLES2GlobalBufferBindings) {
            EXPECT_TRUE(compareGlobalGlInt(gl, bindTarget, 0))
                    << "no global buffer object bindings are bound by default";
        }
        for (auto& it : m_buffers_data) {
            EXPECT_EQ(GL_FALSE, gl->glIsBuffer(it.first))
                    << "test-added buffer objects should not exist by default";
        }
    }

    void changedStateCheck() override {
        // Check that right buffers are bound
        for (auto& it : m_bindings) {
            GLenum boundTarget;
            switch (it.first) {
                case GL_ARRAY_BUFFER:
                    boundTarget = GL_ARRAY_BUFFER_BINDING;
                    break;
                case GL_ELEMENT_ARRAY_BUFFER:
                    boundTarget = GL_ELEMENT_ARRAY_BUFFER_BINDING;
                    break;
                default:
                    FAIL() << "Unknown binding location " << it.first;
            }

            EXPECT_TRUE(compareGlobalGlInt(gl, boundTarget, it.second))
                    << "buffer binding " << describeGlEnum(boundTarget)
                    << " should be bound with " << it.second;
        }

        // Check that all buffers have the correct attributes
        for (auto& it : m_buffers_data) {
            checkBufferData(it.first, it.second);
        }
    }

    void stateChange() override { m_buffer_state_change(); }

    // Creates a buffer with the properties in |data| and records its state so
    // it can be checked for preservation after a snapshot.
    GLuint addBuffer(GlBufferData data) {
        GLuint name = createBuffer(gl, data);
        m_buffers_data[name] = data;
        return name;
    }

    // Binds a buffer and records the binding to be checked for preservation
    // after a snapshot.
    void bindBuffer(GLenum binding, GLuint buffer) {
        gl->glBindBuffer(binding, buffer);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        m_bindings[binding] = buffer;
    }

    void setObjectStateChange(std::function<void()> func) {
        m_buffer_state_change = func;
    }

protected:
    void checkBufferData(GLuint name, GlBufferData data) {
        SCOPED_TRACE("checking data for buffer " + std::to_string(name));
        EXPECT_EQ(GL_TRUE, gl->glIsBuffer(name));

        // We bind to GL_ARRAY_BUFFER in order to read buffer data,
        // so let's hold on to what the old binding was so we can restore it
        GLuint currentArrayBuffer;
        gl->glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&currentArrayBuffer);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        gl->glBindBuffer(GL_ARRAY_BUFFER, name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        EXPECT_TRUE(compareBufferParameter(GL_ARRAY_BUFFER, GL_BUFFER_SIZE,
                                           (GLint)data.size));
        EXPECT_TRUE(compareBufferParameter(GL_ARRAY_BUFFER, GL_BUFFER_USAGE,
                                           (GLint)data.usage));
        // TODO(benzene): compare actual buffer contents?
        // in GLES2 there doesn't seem to be a way to directly read the buffer
        // contents

        // Restore the old binding
        gl->glBindBuffer(GL_ARRAY_BUFFER, currentArrayBuffer);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    }

    testing::AssertionResult compareBufferParameter(GLenum target,
                                                    GLenum paramName,
                                                    GLint expected) {
        GLint value;
        gl->glGetBufferParameteriv(target, paramName, &value);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        return compareValue<GLint>(
                expected, value,
                "mismatch on parameter " + describeGlEnum(paramName) +
                        " for buffer bound to " + describeGlEnum(target));
    }

    std::map<GLenum, GLuint> m_bindings;
    std::map<GLuint, GlBufferData> m_buffers_data;
    std::function<void()> m_buffer_state_change = [] {};
};

TEST_F(SnapshotGlBufferObjectsTest, BindArrayAndElementBuffers) {
    setObjectStateChange([this] {
        GlBufferData arrayData = {128, nullptr, GL_STATIC_DRAW};
        GlBufferData elementArrayData = {256, nullptr, GL_DYNAMIC_DRAW};
        GLuint arrayBuff = addBuffer(arrayData);
        GLuint elementArrayBuff = addBuffer(elementArrayData);
        bindBuffer(GL_ARRAY_BUFFER, arrayBuff);
        bindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuff);
    });
    doCheckedSnapshot();
}

}  // namespace emugl
