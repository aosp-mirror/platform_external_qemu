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
#include "OpenglCodecCommon/glUtils.h"

#include <gtest/gtest.h>
#include <map>
#include <string>

namespace emugl {

static const char kTestVertexShader[] = R"(
attribute vec4 position;
uniform mat4 testFloatMat;
uniform mat4 transform;
uniform mat4 screenSpace;
uniform ivec3 testInts[2];
varying float linear;
void main(void) {
    gl_Position = testFloatMat * transform * position;
    linear = (screenSpace * position).x;
    gl_PointSize = linear * 0.5 + float(testInts[1].x);
}
)";

static const char kTestFragmentShader[] = R"(
precision mediump float;
void main() {
    gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
)";

struct GlShaderVariable {
    GLint size;
    GLenum type;
    std::vector<GLchar> name;
    GLint location;

    std::vector<GlValues> values;
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

// SnapshotGlProgramTest - A helper class for testing the snapshot preservation
// of program objects' state.
//
// This holds state information of a particular single program object whose
// state is mutated in order to set up tests.
// Provide a lambda via setStateChanger to set up the state which will be
// checked for preservation.
// A test can also verify that the snapshot keeps the correct program in use by
// calling useProgram during state setup.
//
class SnapshotGlProgramTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        EXPECT_EQ(GL_FALSE, gl->glIsProgram(m_program_name));
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_CURRENT_PROGRAM, 0));
    }

    void changedStateCheck() override {
        SCOPED_TRACE("test program name = " + std::to_string(m_program_name));
        EXPECT_EQ(GL_TRUE, gl->glIsProgram(m_program_name));
        EXPECT_TRUE(
                compareGlobalGlInt(gl, GL_CURRENT_PROGRAM, m_current_program));

        GlProgramState currentState = getProgramState();

        EXPECT_STREQ(m_program_state.infoLog.data(),
                     currentState.infoLog.data());

        EXPECT_EQ(m_program_state.deleteStatus, currentState.deleteStatus);
        EXPECT_EQ(m_program_state.linkStatus, currentState.linkStatus);
        EXPECT_EQ(m_program_state.validateStatus, currentState.validateStatus);

        // TODO(benzene): allow test to pass even if these are out of order
        EXPECT_EQ(m_program_state.shaders, currentState.shaders);

        EXPECT_EQ(m_program_state.activeAttributes,
                  currentState.activeAttributes);
        EXPECT_EQ(m_program_state.maxAttributeName,
                  currentState.maxAttributeName);
        ASSERT_EQ(m_program_state.attributes.size(),
                  currentState.attributes.size());
        for (int i = 0; i < currentState.attributes.size(); i++) {
            SCOPED_TRACE("active attribute i = " + std::to_string(i));
            EXPECT_EQ(m_program_state.attributes[i].size,
                      currentState.attributes[i].size);
            EXPECT_EQ(m_program_state.attributes[i].type,
                      currentState.attributes[i].type);
            EXPECT_STREQ(m_program_state.attributes[i].name.data(),
                         currentState.attributes[i].name.data());
            EXPECT_EQ(m_program_state.attributes[i].location,
                      currentState.attributes[i].location);

            // TODO(benzene): check attribute values?
        }

        EXPECT_EQ(m_program_state.activeUniforms, currentState.activeUniforms);
        EXPECT_EQ(m_program_state.maxUniformName, currentState.maxUniformName);
        ASSERT_EQ(m_program_state.uniforms.size(),
                  currentState.uniforms.size());
        for (int i = 0; i < currentState.uniforms.size(); i++) {
            SCOPED_TRACE("active uniform i = " + std::to_string(i));
            EXPECT_EQ(m_program_state.uniforms[i].size,
                      currentState.uniforms[i].size);
            EXPECT_EQ(m_program_state.uniforms[i].type,
                      currentState.uniforms[i].type);
            EXPECT_STREQ(m_program_state.uniforms[i].name.data(),
                         currentState.uniforms[i].name.data());
            EXPECT_EQ(m_program_state.uniforms[i].location,
                      currentState.uniforms[i].location);

            for (int j = 0; j < currentState.uniforms[i].size; j++) {
                SCOPED_TRACE("value j = " + std::to_string(j));
                GlValues& expectedVal = m_program_state.uniforms[i].values[j];
                GlValues& currentVal = currentState.uniforms[i].values[j];
                if (currentVal.floats.size() > 0 &&
                    currentVal.ints.size() > 0) {
                    ADD_FAILURE() << "Uniform "
                                  << currentState.uniforms[i].name.data()
                                  << " had both ints and floats at index " << j;
                }
                if (currentVal.floats.size() > 0) {
                    EXPECT_EQ(currentVal.floats, expectedVal.floats)
                            << currentState.uniforms[i].name.data();
                } else {
                    EXPECT_EQ(currentVal.ints, expectedVal.ints)
                            << currentState.uniforms[i].name.data();
                }
            }
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
    // As part of state change, have the GL use the test program.
    void useProgram() {
        gl->glUseProgram(m_program_name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        m_current_program = m_program_name;
    }

    // Collects information about the test program object's current state.
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
        gl->glGetProgramiv(m_program_name, GL_ATTACHED_SHADERS,
                           &attachedShaders);
        ret.shaders.resize(attachedShaders);
        GLsizei shaderCount;
        gl->glGetAttachedShaders(m_program_name, attachedShaders, &shaderCount,
                                 &ret.shaders[0]);

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
                    gl->glGetUniformLocation(m_program_name, unif.name.data());

            if (unif.size > 1) {
                // uniform array; get values from each index
                std::string baseName =
                        getUniformBaseName(std::string(unif.name.data()));
                for (int uniformValueIndex = 0; uniformValueIndex < unif.size;
                     uniformValueIndex++) {
                    std::string indexedName =
                            baseName + '[' + std::to_string(uniformValueIndex) +
                            ']';
                    GLuint indexedLocation = gl->glGetUniformLocation(
                            m_program_name, indexedName.c_str());
                    getUniformValues(indexedLocation, unif.type, unif.values);
                }
            } else {
                getUniformValues(unif.location, unif.type, unif.values);
            }
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

            // TODO(benzene): get attribute values?

            ret.attributes.push_back(attr);
        }

        return ret;
    }

    // Retrieves the values of the uniform at |location| for the test program.
    // Returns them into |values|.
    void getUniformValues(GLuint location,
                          GLenum type,
                          std::vector<GlValues>& values) {
        GlValues val = {};
        switch (type) {
            case GL_FLOAT:
            case GL_FLOAT_VEC2:
            case GL_FLOAT_VEC3:
            case GL_FLOAT_VEC4:
            case GL_FLOAT_MAT2:
            case GL_FLOAT_MAT3:
            case GL_FLOAT_MAT4:
                val.floats.resize(glSizeof(type) / sizeof(GLfloat));
                gl->glGetUniformfv(m_program_name, location, val.floats.data());
                values.push_back(std::move(val));
                return;
            case GL_INT:
            case GL_INT_VEC2:
            case GL_INT_VEC3:
            case GL_INT_VEC4:
            case GL_BOOL:
            case GL_BOOL_VEC2:
            case GL_BOOL_VEC3:
            case GL_BOOL_VEC4:
            case GL_SAMPLER_2D:
            case GL_SAMPLER_CUBE:
                val.ints.resize(glSizeof(type) / sizeof(GLint));
                gl->glGetUniformiv(m_program_name, location, val.ints.data());
                values.push_back(std::move(val));
                break;
            default:
                ADD_FAILURE() << "unsupported uniform type " << type;
                return;
        }
    }

    // If string |name| ends with a subscript ([]) operator, return a substring
    // with the subscript removed.
    std::string getUniformBaseName(const std::string& name) {
        std::string baseName;
        int length = name.length();
        if (length < 3)
            return name;
        size_t lastBracket = name.find_last_of('[');
        if (lastBracket != std::string::npos) {
            baseName = name.substr(0, lastBracket);
        } else {
            baseName = name;
        }
        return baseName;
    }

    GLuint m_program_name = 0;
    GlProgramState m_program_state = {};
    GLuint m_current_program = 0;

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

        gl->glLinkProgram(m_program_name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        gl->glValidateProgram(m_program_name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        m_program_state = getProgramState();

        EXPECT_EQ(1, m_program_state.activeAttributes);
        EXPECT_EQ(4, m_program_state.activeUniforms);
        EXPECT_EQ(GL_TRUE, m_program_state.linkStatus);
        EXPECT_EQ(GL_TRUE, m_program_state.validateStatus);
    });
    doCheckedSnapshot();
}

TEST_F(SnapshotGlProgramTest, UseProgramAndUniforms) {
    setStateChanger([this] {
        GLuint vshader =
                loadAndCompileShader(gl, GL_VERTEX_SHADER, kTestVertexShader);
        GLuint fshader = loadAndCompileShader(gl, GL_FRAGMENT_SHADER,
                                              kTestFragmentShader);

        gl->glAttachShader(m_program_name, vshader);
        gl->glAttachShader(m_program_name, fshader);

        gl->glLinkProgram(m_program_name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        gl->glValidateProgram(m_program_name);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        useProgram();

        GLuint floatMatUnifLocation =
                gl->glGetUniformLocation(m_program_name, "testFloatMat");
        const GLfloat testFloatMatrix[16] = {
                1.0, 0.9, 0.8, 0.7,  0.6,  0.5,  0.4,  0.3,
                0.2, 0.1, 0.0, -0.1, -0.1, -0.3, -0.4, -0.5,
        };
        gl->glUniformMatrix4fv(floatMatUnifLocation, 1, GL_FALSE,
                               testFloatMatrix);

        GLuint intVecUnifLocation =
                gl->glGetUniformLocation(m_program_name, "testInts");
        const GLint testIntVec[6] = {
                10, 11, 12, 20, 21, 22,
        };
        gl->glUniform3iv(intVecUnifLocation, 2, testIntVec);

        m_program_state = getProgramState();
    });
    doCheckedSnapshot();
}

}  // namespace emugl
