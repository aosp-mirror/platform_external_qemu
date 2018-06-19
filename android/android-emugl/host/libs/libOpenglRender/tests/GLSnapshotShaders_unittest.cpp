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

#include <string>

namespace emugl {

static const char kTestVertexShaderSource[] = R"(
attribute vec4 position;
uniform mat4 projection;
uniform mat4 transform;
uniform mat4 screenSpace;
varying float linear;
void main(void) {
    vec4 transformedPosition = projection * transform * position;
    gl_Position = transformedPosition;
    linear = (screenSpace * position).x;
}
)";

static const char kTestFragmentShaderSource[] = R"(
precision mediump float;
void main() {
    gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
)";

struct GLShaderState {
    GLenum type;
    std::string source;
    GLboolean deleteStatus;
    GLboolean compileStatus;
    GLint infoLogLength;
    GLint sourceLength;
};

class SnapshotGlShaderTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        if (m_shader_name == 0) {
            return;
        }
        GLint type;
        gl->glGetShaderiv(m_shader_name, GL_SHADER_TYPE, &type);
        EXPECT_EQ(GL_INVALID_VALUE, gl->glGetError());
    }

    void changedStateCheck() override {
        checkParameter(GL_SHADER_TYPE, m_shader_state.type);
        checkParameter(GL_DELETE_STATUS, m_shader_state.deleteStatus);
        checkParameter(GL_COMPILE_STATUS, m_shader_state.compileStatus);
        checkParameter(GL_INFO_LOG_LENGTH, m_shader_state.infoLogLength);
        checkParameter(GL_SHADER_SOURCE_LENGTH, m_shader_state.sourceLength);
    }

    virtual void stateChange() override {
        m_shader_name = gl->glCreateShader(m_shader_state.type);
    }

    void loadSource(std::string sourceString) {
        GLboolean compiler;
        gl->glGetBooleanv(GL_SHADER_COMPILER, &compiler);
        if (compiler == GL_FALSE) {
            fprintf(stderr, "Shader compiler is not supported.\n");
            return;
        }

        if (m_shader_name == 0) {
            FAIL() << "Cannot set source without a shader name";
        }
        m_shader_state.source = sourceString;
        GLint len = sourceString.length();
        if (len > 0) {
            m_shader_state.sourceLength =
                    len + 1;  // Counts the null terminator
        }
        const char* source = sourceString.c_str();
        const char** sources = &source;
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        gl->glShaderSource(m_shader_name, 1, sources, &len);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    }

protected:
    void checkParameter(GLenum paramName, GLenum expected) {
        GLint value;
        gl->glGetShaderiv(m_shader_name, paramName, &value);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        EXPECT_EQ(expected, value);
    }

    GLShaderState m_shader_state = {};
    GLuint m_shader_name;
};

class SnapshotGlVertexShaderTest : public SnapshotGlShaderTest {
public:
    SnapshotGlVertexShaderTest() { m_shader_state = {GL_VERTEX_SHADER}; }
};

TEST_F(SnapshotGlVertexShaderTest, CreateVertexShader) {
    doCheckedSnapshot();
}

class SnapshotGlLoadVertexShaderTest : public SnapshotGlVertexShaderTest {
public:
    void stateChange() override {
        SnapshotGlVertexShaderTest::stateChange();
        loadSource(std::string(kTestVertexShaderSource));
    }
};

TEST_F(SnapshotGlLoadVertexShaderTest, SetVertexShaderSource) {
    doCheckedSnapshot();
}

class SnapshotGlFragmentShaderTest : public SnapshotGlShaderTest {
public:
    SnapshotGlFragmentShaderTest() { m_shader_state = {GL_FRAGMENT_SHADER}; }
};

TEST_F(SnapshotGlFragmentShaderTest, CreateFragmentShader) {
    doCheckedSnapshot();
}

class SnapshotGlLoadFragmentShaderTest : public SnapshotGlFragmentShaderTest {
public:
    void stateChange() override {
        SnapshotGlFragmentShaderTest::stateChange();
        loadSource(std::string(kTestFragmentShaderSource));
    }
};

TEST_F(SnapshotGlLoadFragmentShaderTest, SetFragmentShaderSource) {
    doCheckedSnapshot();
}

}  // namespace emugl
