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

#include <gtest/gtest.h>
#include <map>
#include <string>

namespace emugl {

static const char kTestVertexShader[] = R"(
attribute vec4 position;
uniform mat4 projection;
uniform mat4 transform;
uniform mat4 screenSpace;
varying float linear;
void main(void) {
    gl_Position = projection * transform * position;
    linear = (screenSpace * position).x;
    gl_PointSize = linear * 0.5;
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

struct GlShaderVariable {
    // TODO(benzene): make sure we aren't writing memory wrongly because of this
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
    std::vector<GlShaderVariable> attributes;

    GLint activeUniforms;
    GLint maxUniformName;
    std::vector<GlShaderVariable> uniforms;
};

class SnapshotGlProgramTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        EXPECT_EQ(GL_FALSE, gl->glIsProgram(m_program_name));
    }

    void changedStateCheck() override {
        EXPECT_EQ(GL_TRUE, gl->glIsProgram(m_program_name));

        GlProgramState currentState = getProgramState();

        EXPECT_STREQ(&m_program_state.infoLog[0], &currentState.infoLog[0]);

        EXPECT_EQ(m_program_state.deleteStatus, currentState.deleteStatus);
        EXPECT_EQ(m_program_state.linkStatus, currentState.linkStatus);
        EXPECT_EQ(m_program_state.validateStatus, currentState.validateStatus);

        // TODO(benzene): accept if they are out of order?
        EXPECT_EQ(m_program_state.shaders, currentState.shaders);

        EXPECT_EQ(m_program_state.activeAttributes,
                  currentState.activeAttributes);
        EXPECT_EQ(m_program_state.maxAttributeName,
                  currentState.maxAttributeName);
        ASSERT_EQ(m_program_state.attributes.size(),
                  currentState.attributes.size());
        for (int i = 0; i < currentState.attributes.size(); i++) {
            EXPECT_EQ(m_program_state.attributes[i].size,
                      currentState.attributes[i].size)
                    << " index " << i;
            EXPECT_EQ(m_program_state.attributes[i].type,
                      currentState.attributes[i].type)
                    << " index " << i;
            EXPECT_STREQ(&m_program_state.attributes[i].name[0],
                         &currentState.attributes[i].name[0])
                    << " index " << i;
            EXPECT_EQ(m_program_state.attributes[i].location,
                      currentState.attributes[i].location)
                    << " index " << i;
        }

        EXPECT_EQ(m_program_state.activeUniforms, currentState.activeUniforms);
        EXPECT_EQ(m_program_state.maxUniformName, currentState.maxUniformName);
        ASSERT_EQ(m_program_state.uniforms.size(),
                  currentState.uniforms.size());
        for (int i = 0; i < currentState.uniforms.size(); i++) {
            EXPECT_EQ(m_program_state.uniforms[i].size,
                      currentState.uniforms[i].size)
                    << " index " << i;
            EXPECT_EQ(m_program_state.uniforms[i].type,
                      currentState.uniforms[i].type)
                    << " index " << i;
            EXPECT_STREQ(&m_program_state.uniforms[i].name[0],
                         &currentState.uniforms[i].name[0])
                    << " index " << i;
            EXPECT_EQ(m_program_state.uniforms[i].location,
                      currentState.uniforms[i].location)
                    << " index " << i;

            // TODO(benzene): check uniform values
        }
    }

    void stateChange() override {
        m_program_name = gl->glCreateProgram();
        m_program_state = getProgramState();
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
        return compareValue<GLint>(
                expected, value,
                "GL program object " + std::to_string(m_program_name) +
                        "; mismatch for param " + std::to_string(paramName));
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
        gl->glGetProgramInfoLog(m_program_name, logLength, &actualLength,
                                &ret.infoLog[0]);
        // fprintf(stderr, "%d length info log: %s\n", actualLength,
        // &ret.infoLog[0]);

        // Boolean statuses
        GLint val;
        gl->glGetProgramiv(m_program_name, GL_DELETE_STATUS, &val);
        ret.deleteStatus = val;
        gl->glGetProgramiv(m_program_name, GL_LINK_STATUS, &val);
        ret.linkStatus = val;
        gl->glGetProgramiv(m_program_name, GL_VALIDATE_STATUS, &val);
        ret.validateStatus = val;
        // fprintf(stderr, "Validate status is %s\n", val ? "true" : "false");

        // Attached shaders
        GLint attachedShaders;
        gl->glGetProgramiv(m_program_name, GL_ATTACHED_SHADERS,
                           &attachedShaders);
        ret.shaders.resize(attachedShaders);
        GLsizei shaderCount;
        gl->glGetAttachedShaders(m_program_name, attachedShaders, &shaderCount,
                                 &ret.shaders[0]);
        // fprintf(stderr, "Shader attachments %d, Shader count %d\n",
        //         attachedShaders, shaderCount);

        // Uniforms
        gl->glGetProgramiv(m_program_name, GL_ACTIVE_UNIFORM_MAX_LENGTH,
                           &ret.maxUniformName);
        gl->glGetProgramiv(m_program_name, GL_ACTIVE_UNIFORMS,
                           &ret.activeUniforms);
        for (GLuint i = 0; i < ret.activeUniforms; i++) {
            GlShaderVariable unif = {};
            unif.name.resize(ret.maxUniformName);
            GLsizei unifLen;
            gl->glGetActiveUniform(m_program_name, i, ret.maxUniformName,
                                   &unifLen, &unif.size, &unif.type,
                                   &unif.name[0]);
            unif.location =
                    gl->glGetUniformLocation(m_program_name, &unif.name[0]);

            // fprintf(stderr, "uniform with name %s\n", &unif.name[0]);
            // TODO(benzene): get uniform values

            ret.uniforms.push_back(unif);
        }

        // Attributes
        gl->glGetProgramiv(m_program_name, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                           &ret.maxAttributeName);
        gl->glGetProgramiv(m_program_name, GL_ACTIVE_ATTRIBUTES,
                           &ret.activeAttributes);
        for (GLuint i = 0; i < ret.activeAttributes; i++) {
            GlShaderVariable attr = {};
            attr.name.resize(ret.maxAttributeName);
            GLsizei attrLen;
            gl->glGetActiveAttrib(m_program_name, i, ret.maxAttributeName,
                                  &attrLen, &attr.size, &attr.type,
                                  &attr.name[0]);
            attr.location =
                    gl->glGetAttribLocation(m_program_name, &attr.name[0]);

            // fprintf(stderr, "attr with name %s\n", &attr.name[0]);
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
        GLuint vshader =
                loadAndCompileShader(gl, GL_VERTEX_SHADER, kTestVertexShader);
        GLuint fshader = loadAndCompileShader(gl, GL_FRAGMENT_SHADER,
                                              kTestFragmentShader);
        gl->glAttachShader(m_program_name, vshader);
        gl->glAttachShader(m_program_name, fshader);
        gl->glDetachShader(m_program_name, vshader);
        m_program_state.shaders.push_back(fshader);
    });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlProgramTest, LinkAndValidate) {
    setStateChanger([this] {
        GLuint vshader =
                loadAndCompileShader(gl, GL_VERTEX_SHADER, kTestVertexShader);
        GLuint fshader = loadAndCompileShader(gl, GL_FRAGMENT_SHADER,
                                              kTestFragmentShader);

        gl->glAttachShader(m_program_name, vshader);
        gl->glAttachShader(m_program_name, fshader);

        gl->glValidateProgram(m_program_name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        gl->glLinkProgram(m_program_name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        gl->glValidateProgram(m_program_name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        m_program_state = getProgramState();

        EXPECT_EQ(1, m_program_state.activeAttributes);
        EXPECT_EQ(3, m_program_state.activeUniforms);
        EXPECT_EQ(GL_TRUE, m_program_state.linkStatus);
        EXPECT_EQ(GL_TRUE, m_program_state.validateStatus);
    });
    doCheckedSnapshot();
}

}  // namespace emugl
