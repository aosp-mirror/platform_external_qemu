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
#include "GLSnapshotTestStateUtils.h"

#include "ShaderUtils.h"

#include <string>
#include <gtest/gtest.h>

namespace emugl {

static const char kTestVertexShader[] = R"(
attribute vec4 position;
attribute mat2 fooMat;
uniform mat4 projection;
uniform mat4 transform;
uniform mat4 screenSpace;
varying vec3 foobarVec;
varying float linear;
void main(void) {
    foobarVec = fooMat[0].xyx + vec3(1, 1, 0);
    vec4 transformedPosition = projection * transform * position;
    gl_Position = transformedPosition;
    linear = (screenSpace * position).x;
}
)";

static const char kTestInvalidVertexShader[] = R"(
TODO(benzene): make this compile but result in an invalid program
)";

static const char kTestFragmentShader[] = R"(
precision mediump float;
void main() {
    gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
)";

struct GlAttribute {
    //GLsizei length; // length of |name|, excluding null terminator
    // TODO(benzene): make sure we aren't writing memory wrongly because of this
    GLint size;
    GLenum type;
    std::vector<GLchar> name;
    GLint location;
};

struct GlUniform {
    GLint size;
    GLenum type;
    std::vector<GLchar> name;
    GLint location;
};

struct GlProgramState {
    GLboolean deleteStatus;
    GLboolean linkStatus;
    GLboolean validateStatus;

    std::vector<GLchar> infoLog;

    std::vector<GLuint> shaders;

    GLint activeAttributes;
    GLint maxAttributeName;
    std::vector<GlAttribute> attributes;

    GLint activeUniforms;
    GLint maxUniformName;
    std::vector<GlUniform> uniforms;
};

// Vertex Attributes
// Uniforms
//      Samplers
// Varying vars

class SnapshotGlProgramTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        EXPECT_EQ(GL_FALSE, gl->glIsProgram(m_program_name));
    }

    void changedStateCheck() override {
        EXPECT_EQ(GL_TRUE, gl->glIsProgram(m_program_name));
        fprintf(stderr, "Program Changed State Check\n");
        EXPECT_TRUE(checkParameter(GL_DELETE_STATUS, (GLint)m_program_state.deleteStatus));
        EXPECT_TRUE(checkParameter(GL_LINK_STATUS, (GLint)m_program_state.linkStatus));
        EXPECT_TRUE(checkParameter(GL_VALIDATE_STATUS, (GLint)m_program_state.deleteStatus));
        EXPECT_TRUE(checkParameter(GL_ATTACHED_SHADERS, (GLint)m_program_state.shaders.size()));
        EXPECT_TRUE(checkParameter(GL_INFO_LOG_LENGTH, m_program_state.infoLog.size())); // counts null terminator

        // attribute count
        EXPECT_TRUE(checkParameter(GL_ACTIVE_ATTRIBUTES, m_program_state.activeAttributes));
        // attribute max

        // uniform count
        EXPECT_TRUE(checkParameter(GL_ACTIVE_UNIFORMS, m_program_state.activeUniforms));
        // uniform max
    }

    void stateChange() override {
        m_program_name = gl->glCreateProgram();
        m_state_changer();
    }

    void setStateChanger(std::function<void()> changer) {
        m_state_changer = changer;
    }

protected:
    testing::AssertionResult checkParameter(GLenum paramName, GLint expected) {
        GLint value;
        gl->glGetProgramiv(m_program_name, paramName, &value);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        return compareGlValue<GLint>(expected, value, "GL program object "
                + std::to_string(m_program_name) + "; mismatch for param " + std::to_string(paramName));
    }

    GlProgramState getProgramState() {
        GlProgramState ret = {};
        if (GL_FALSE == gl->glIsProgram(m_program_name)) {
            ADD_FAILURE() << "cannot get program state: was not a program";
            return ret;
        }

        // Info log
        GLsizei logLength;
        gl->glGetProgramiv(m_program_name, GL_INFO_LOG_LENGTH, &logLength);
        ret.infoLog.resize(logLength);
        GLsizei actualLength;
        gl->glGetProgramInfoLog(m_program_name, logLength, &actualLength, &ret.infoLog[0]);
        fprintf(stderr, "info log: %s\n", &ret.infoLog[0]);

        // Boolean statuses
        GLint val;
        gl->glGetProgramiv(m_program_name, GL_DELETE_STATUS, &val);
        ret.deleteStatus = val;
        gl->glGetProgramiv(m_program_name, GL_LINK_STATUS, &val);
        ret.linkStatus = val;
        gl->glGetProgramiv(m_program_name, GL_VALIDATE_STATUS, &val);
        ret.validateStatus = val;

        // Attached shaders
        GLint attachedShaders;
        gl->glGetProgramiv(m_program_name, GL_ATTACHED_SHADERS, &attachedShaders);
        ret.shaders.resize(attachedShaders);
        GLsizei shaderCount;
        gl->glGetAttachedShaders(m_program_name, attachedShaders, &shaderCount, &ret.shaders[0]);
        fprintf(stderr, "Shader attachments %d, Shader count %d\n", attachedShaders, shaderCount);

        // Uniforms
        gl->glGetProgramiv(m_program_name, GL_ACTIVE_UNIFORM_MAX_LENGTH, &ret.maxUniformName);
        gl->glGetProgramiv(m_program_name, GL_ACTIVE_UNIFORMS, &ret.activeUniforms);
        for (GLuint i = 0; i < ret.activeUniforms; i++) {
            GlUniform unif = {};
            unif.name.resize(ret.maxUniformName);
            GLsizei unifLen;
            gl->glGetActiveUniform(m_program_name, i, ret.maxUniformName,
                    &unifLen, &unif.size, &unif.type, &unif.name[0]);
            unif.location = gl->glGetUniformLocation(m_program_name, &unif.name[0]);

            fprintf(stderr, "uniform with name %s\n", &unif.name[0]);
            // TODO(benzene): get uniform values

            ret.uniforms.push_back(unif);
        }

        // Attributes
        gl->glGetProgramiv(m_program_name, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &ret.maxAttributeName);
        gl->glGetProgramiv(m_program_name, GL_ACTIVE_ATTRIBUTES, &ret.activeAttributes);
        for (GLuint i = 0; i < ret.activeAttributes; i++) {
            GlAttribute attr = {};
            attr.name.resize(ret.maxAttributeName);
            GLsizei attrLen;
            gl->glGetActiveAttrib(m_program_name, i, ret.maxAttributeName,
                    &attrLen, &attr.size, &attr.type, &attr.name[0]);
            attr.location = gl->glGetAttribLocation(m_program_name, &attr.name[0]);

            fprintf(stderr, "attr with name %s\n", &attr.name[0]);
            // TODO(benzene): get attribute values

            ret.attributes.push_back(attr);
        }



        return ret;
    }

    GLuint m_program_name;
    GlProgramState m_program_state = {};
    GLuint m_current_shader;

    std::function<void()> m_state_changer = [] {};
};

TEST_F(SnapshotGlProgramTest, CreateProgram) {
    doCheckedSnapshot();
}

TEST_F(SnapshotGlProgramTest, AttachDetachShader) {
    setStateChanger([this] {
        GLuint vshader = compileShader(GL_VERTEX_SHADER, kTestVertexShader);
        GLuint fshader = compileShader(GL_FRAGMENT_SHADER, kTestFragmentShader);
        gl->glAttachShader(m_program_name, vshader);
        gl->glAttachShader(m_program_name, fshader);
        gl->glDetachShader(m_program_name, vshader);
        //m_program_state.shaders.push_back(vshader);
        //m_program_state.shaders.push_back(fshader);
        m_program_state = getProgramState();
    });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlProgramTest, ValidateAndLink) {
    setStateChanger([this] {
        GLuint vshader = compileShader(GL_VERTEX_SHADER, kTestVertexShader);
        GLuint fshader = compileShader(GL_FRAGMENT_SHADER, kTestFragmentShader);
        gl->glAttachShader(m_program_name, vshader);
        // m_program_state.shaders.push_back(vshader);
        // m_program_state.activeAttributes = 2;

        gl->glAttachShader(m_program_name, fshader);
        // m_program_state.shaders.push_back(fshader);
        // m_program_state.activeUniforms = 3;

        gl->glValidateProgram(m_program_name);
        // m_program_state.validateStatus = true;

        gl->glLinkProgram(m_program_name);
        // m_program_state.linkStatus = true;

        m_program_state = getProgramState();
    });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlProgramTest, DISABLED_DeleteProgram) {
    setStateChanger([this] {
        gl->glDeleteProgram(m_program_name);
        // TODO(benzene): write a good test, maybe one that doesn't delete the
        // program immediately so we see it in a 'delete when not current' state
    });
    doCheckedSnapshot();
}

// TEST_F() {

// }

}  // namespace emugl
