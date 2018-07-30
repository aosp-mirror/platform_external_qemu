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

struct GlShaderState {
    GLenum type;
    GLboolean deleteStatus;
    GLboolean compileStatus;
    GLint infoLogLength;
    std::vector<GLchar> infoLog;
    GLint sourceLength;
    std::string source;
};

// SnapshotGlShaderTest - A helper class for testing snapshot's preservation of
// a GL shader object's states.
//
// It operates like SnapshotPreserveTest, and also holds information about a
// particular shader object which is manipulated in tests using
// SnapshotGlShaderTest as a fixture.
// Helper functions like loadSource first need a created shader identified by
// |m_shader_name|. This creation happens by default in stateChange. Use them in
// a lambda, set through setShaderStateChanger, to set up state without
// overriding doCheckedSnapshot.
//
class SnapshotGlShaderTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        EXPECT_EQ(GL_FALSE, gl->glIsShader(m_shader_name));
    }

    void changedStateCheck() override {
        SCOPED_TRACE("for shader " + std::to_string(m_shader_name));

        EXPECT_TRUE(compareParameter(GL_SHADER_TYPE, m_shader_state.type));
        EXPECT_TRUE(compareParameter(GL_DELETE_STATUS,
                                     m_shader_state.deleteStatus));
        EXPECT_TRUE(compareParameter(GL_COMPILE_STATUS,
                                     m_shader_state.compileStatus));
        EXPECT_TRUE(compareParameter(GL_INFO_LOG_LENGTH,
                                     m_shader_state.infoLogLength));
        EXPECT_TRUE(compareParameter(GL_SHADER_SOURCE_LENGTH,
                                     m_shader_state.sourceLength));

        std::vector<GLchar> srcData = {};
        srcData.resize(m_shader_state.sourceLength);
        gl->glGetShaderSource(m_shader_name, m_shader_state.sourceLength,
                              nullptr, srcData.data());
        if (srcData.data() == NULL) {
            EXPECT_EQ(0, m_shader_state.source.length()) << "source is empty";
        } else {
            EXPECT_STREQ(m_shader_state.source.c_str(), srcData.data());
        }

        std::vector<GLchar> infoLogData = {};
        infoLogData.resize(m_shader_state.infoLogLength);
        gl->glGetShaderInfoLog(m_shader_name, m_shader_state.infoLogLength,
                               nullptr, infoLogData.data());
        if (infoLogData.data() == NULL) {
            EXPECT_EQ(0, m_shader_state.infoLogLength) << "info log is empty";
        } else {
            EXPECT_STREQ(m_shader_state.infoLog.data(), infoLogData.data());
        }
    }

    void stateChange() override {
        m_shader_name = gl->glCreateShader(m_shader_state.type);
        m_shader_state_changer();

        // Store state of info log
        gl->glGetShaderiv(m_shader_name, GL_INFO_LOG_LENGTH,
                          &m_shader_state.infoLogLength);
        m_shader_state.infoLog.resize(m_shader_state.infoLogLength);
        gl->glGetShaderInfoLog(m_shader_name, m_shader_state.infoLogLength,
                               nullptr, m_shader_state.infoLog.data());
    }

    void loadSource(const std::string& sourceString) {
        GLboolean compiler;
        gl->glGetBooleanv(GL_SHADER_COMPILER, &compiler);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
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
        gl->glShaderSource(m_shader_name, 1, sources, &len);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    }

    void compile(GLboolean expectCompileStatus = GL_TRUE) {
        GLboolean compiler;
        gl->glGetBooleanv(GL_SHADER_COMPILER, &compiler);
        if (compiler == GL_FALSE) {
            fprintf(stderr, "Shader compiler is not supported.\n");
            return;
        }

        if (m_shader_name == 0) {
            ADD_FAILURE() << "Cannot compile shader without a shader name";
        }
        if (m_shader_state.source.length() == 0) {
            ADD_FAILURE() << "Shader needs source to compile";
        }
        gl->glCompileShader(m_shader_name);
        m_shader_state.compileStatus = expectCompileStatus;
    }

    // Supply a lambda as |changer| to perform additional state setup after the
    // shader has been created but before the snapshot is performed.
    void setShaderStateChanger(std::function<void()> changer) {
        m_shader_state_changer = changer;
    }

protected:
    testing::AssertionResult compareParameter(GLenum paramName,
                                              GLenum expected) {
        GLint value;
        gl->glGetShaderiv(m_shader_name, paramName, &value);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        return compareValue<GLint>(
                expected, value,
                "mismatch on parameter " + describeGlEnum(paramName) +
                        " for shader " + std::to_string(m_shader_name));
    }

    GlShaderState m_shader_state = {};
    GLuint m_shader_name;
    std::function<void()> m_shader_state_changer = [] {};
};

class SnapshotGlVertexShaderTest : public SnapshotGlShaderTest {
public:
    SnapshotGlVertexShaderTest() { m_shader_state = {GL_VERTEX_SHADER}; }
};

TEST_F(SnapshotGlVertexShaderTest, Create) {
    doCheckedSnapshot();
}

TEST_F(SnapshotGlVertexShaderTest, SetSource) {
    setShaderStateChanger(
            [this] { loadSource(std::string(kTestVertexShaderSource)); });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlVertexShaderTest, CompileSuccess) {
    setShaderStateChanger([this] {
        loadSource(std::string(kTestVertexShaderSource));
        compile();
    });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlVertexShaderTest, CompileFail) {
    std::string failedShader = "vec3 hi my name is compile failed";
    setShaderStateChanger([this, &failedShader] {
        loadSource(failedShader);
        compile(GL_FALSE);
    });
    doCheckedSnapshot();
}

class SnapshotGlFragmentShaderTest : public SnapshotGlShaderTest {
public:
    SnapshotGlFragmentShaderTest() { m_shader_state = {GL_FRAGMENT_SHADER}; }
};

TEST_F(SnapshotGlFragmentShaderTest, Create) {
    doCheckedSnapshot();
}

TEST_F(SnapshotGlFragmentShaderTest, SetSource) {
    setShaderStateChanger(
            [this] { loadSource(std::string(kTestFragmentShaderSource)); });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlFragmentShaderTest, CompileSuccess) {
    setShaderStateChanger([this] {
        loadSource(std::string(kTestFragmentShaderSource));
        compile();
    });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlFragmentShaderTest, CompileFail) {
    std::string failedShader = "vec3 nice to meet you compile failed";
    setShaderStateChanger([this, &failedShader] {
        loadSource(failedShader);
        compile(GL_FALSE);
    });
    doCheckedSnapshot();
}

}  // namespace emugl
