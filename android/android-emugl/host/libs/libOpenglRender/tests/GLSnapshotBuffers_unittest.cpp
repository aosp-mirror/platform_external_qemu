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

#include <map>

//#include <GLcommon/ScopedGLState.h>

namespace emugl {

struct GlBufferData {
    GLsizeiptr size;
    GLvoid* bytes;
    GLenum usage;
};

static const GLenum kGLES2GlobalBufferBindings[] = {
    GL_ARRAY_BUFFER_BINDING, GL_ELEMENT_ARRAY_BUFFER_BINDING
};
// Buffers could also be bound with vertex attribute arrays.

class SnapshotGlBufferObjectsTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        for (GLenum bindTarget : kGLES2GlobalBufferBindings) {
            GLint arrayBinding;
            gl->glGetIntegerv(bindTarget, &arrayBinding);
            EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
            // by default, no buffers are bound.
            EXPECT_EQ(0, arrayBinding);
        }
        // None of the buffers should exist
        for (auto& it : m_buffers_data) {
            EXPECT_EQ(GL_FALSE, gl->glIsBuffer(it.first));
        }
    }

    void changedStateCheck() override {
        // Check that all buffers exist
        for (auto& it : m_buffers_data) {
            EXPECT_EQ(GL_TRUE, gl->glIsBuffer(it.first));
        }
        // Check that bound buffers have the properties we expect
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

            GLint boundBuffer;
            gl->glGetIntegerv(boundTarget, &boundBuffer);
            EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
            EXPECT_EQ(it.second, boundBuffer);

            checkBufferParameter(it.first, GL_BUFFER_SIZE, (GLint)m_buffers_data[it.second].size);
            checkBufferParameter(it.first, GL_BUFFER_USAGE, (GLint)m_buffers_data[it.second].usage);
            // in GLES2 there doesn't seem to be a way to directly read the buffer contents
        }
        // TODO(benzene): bind other buffers so we can check them
    }

    void stateChange() override {
        m_buffer_state_change();
    }

    GLuint addBuffer(GlBufferData data) {
        fprintf(stderr, "starting addBuffer\n");
        // TODO(benzene): fix this or save and restore state another way
        //ScopedGLState state;
        //state.push(GL_ARRAY_BUFFER_BINDING);
        fprintf(stderr, "set up scoped state\n");

        GLuint name;
        gl->glGenBuffers(1, &name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        gl->glBindBuffer(GL_ARRAY_BUFFER, name);
        gl->glBufferData(GL_ARRAY_BUFFER, data.size, data.bytes, data.usage);

        m_buffers_data[name] = data;

        fprintf(stderr, "finished addBuffer, trying to restore state\n");
        return name;
    }

    void bindBuffer(GLenum binding, GLuint buffer) {
        gl->glBindBuffer(binding, buffer);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        m_bindings[binding] = buffer;
    }

    void setObjectStateChange(std::function<void()> func) {
        m_buffer_state_change = func;
    }

protected:
    void checkBufferParameter(GLenum target, GLenum paramName, GLint expected) {
        GLint value;
        gl->glGetBufferParameteriv(target, paramName, &value);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        EXPECT_EQ(expected, value);
    }

    std::map<GLenum, GLuint> m_bindings;
    std::map<GLuint, GlBufferData> m_buffers_data;
    std::function<void()> m_buffer_state_change = [] {};
};

TEST_F(SnapshotGlBufferObjectsTest, BuffersTest) {
    setObjectStateChange([this] {
        GlBufferData data = { 128, NULL, GL_STATIC_DRAW };
        GLuint buff = addBuffer(data);
        bindBuffer(GL_ELEMENT_ARRAY_BUFFER, buff);
    });
    doCheckedSnapshot();
}

}  // namespace emugl
