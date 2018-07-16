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

namespace emugl {

struct GlTextureUnitState {
    GLuint binding2D;
    GLuint bindingCubeMap;
};

struct GlTextureObjectState {
    GLenum minFilter;
    GLenum magFilter;
    GLenum wrapS;
    GLenum wrapT;
};

class SnapshotGlTextureUnitActiveTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_ACTIVE_TEXTURE, GL_TEXTURE0));
    }

    void changedStateCheck() override {
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_ACTIVE_TEXTURE,
                                       GL_TEXTURE0 + m_active_texture_unit));
    }

    void stateChange() override {
        gl->glActiveTexture(GL_TEXTURE0 + m_active_texture_unit);
    }

    void useTextureUnit(GLuint unit) {
        GLint maxTextureUnits;
        gl->glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
                          &maxTextureUnits);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

        if (unit < maxTextureUnits) {
            m_active_texture_unit = unit;
        } else {
            fprintf(stderr,
                    "Tried to use texture unit %d when max unit was %d."
                    " Defaulting to unit 0.\n",
                    unit, maxTextureUnits);
            m_active_texture_unit = 0;
        }
    }

protected:
    GLuint m_active_texture_unit;
};

TEST_F(SnapshotGlTextureUnitActiveTest, ActiveTextureUnit) {
    useTextureUnit(1);
    doCheckedSnapshot();
}

class SnapshotGlTextureUnitBindingsTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        GLint maxTextureUnits;
        gl->glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
                          &maxTextureUnits);
        for (int i = 0; i < maxTextureUnits; i++) {
            gl->glActiveTexture(GL_TEXTURE0 + i);
            EXPECT_TRUE(compareGlobalGlInt(gl, GL_TEXTURE_BINDING_2D, 0));
            EXPECT_TRUE(compareGlobalGlInt(gl, GL_TEXTURE_BINDING_CUBE_MAP, 0));
        }
    }

    void changedStateCheck() override {
        GLint maxTextureUnits;
        gl->glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
                          &maxTextureUnits);
        EXPECT_EQ(m_unit_states.size(), maxTextureUnits);

        for (int i = 0; i < maxTextureUnits; i++) {
            gl->glActiveTexture(GL_TEXTURE0 + i);
            EXPECT_TRUE(compareGlobalGlInt(gl, GL_TEXTURE_BINDING_2D,
                                           m_unit_states[i].binding2D));
            EXPECT_TRUE(compareGlobalGlInt(gl, GL_TEXTURE_BINDING_CUBE_MAP,
                                           m_unit_states[i].bindingCubeMap));

            // TODO(benzene): check states of bound texture objects?
        }
    }

    void stateChange() override {
        GLint maxTextureUnits;
        gl->glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
                          &maxTextureUnits);
        m_unit_states.resize(maxTextureUnits);

        m_state_changer();
    }

    void setStateChanger(std::function<void()> changer) {
        m_state_changer = changer;
    }

protected:
    // Create a texture object, bind to texture unit |unit| at binding point
    // |bindPoint|, and record that we've done so.
    GLuint createAndBindTexture(GLuint unit, GLenum bindPoint) {
        GLint maxTextureUnits;
        gl->glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
                          &maxTextureUnits);
        if (unit >= maxTextureUnits) {
            fprintf(stderr,
                    "Cannot bind to unit %d: max units is %d. Binding to %d "
                    "instead.\n",
                    unit, maxTextureUnits, maxTextureUnits - 1);
            unit = maxTextureUnits - 1;
        }

        GLuint testTexture;
        gl->glGenTextures(1, &testTexture);
        gl->glActiveTexture(GL_TEXTURE0 + unit);
        gl->glBindTexture(bindPoint, testTexture);
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        switch (bindPoint) {
            case GL_TEXTURE_2D:
                m_unit_states[unit].binding2D = testTexture;
                break;
            case GL_TEXTURE_CUBE_MAP:
                m_unit_states[unit].bindingCubeMap = testTexture;
                break;
            default:
                ADD_FAILURE() << "Unsupported texture unit bind point " +
                                         describeGlEnum(bindPoint);
        }
        return testTexture;
    }

    std::vector<GlTextureUnitState> m_unit_states;
    std::function<void()> m_state_changer = [] {};
};

TEST_F(SnapshotGlTextureUnitBindingsTest, BindTextures) {
    setStateChanger([this] {
        createAndBindTexture(1, GL_TEXTURE_2D);
        createAndBindTexture(8, GL_TEXTURE_CUBE_MAP);
        createAndBindTexture(16, GL_TEXTURE_2D);
        createAndBindTexture(32, GL_TEXTURE_CUBE_MAP);
    });
    doCheckedSnapshot();
}

class SnapshotGlTextureObjectTest : public SnapshotPreserveTest {
public:
    void defaultStateCheck() override {
        EXPECT_EQ(GL_FALSE, gl->glIsTexture(m_object_name));

        EXPECT_TRUE(compareGlobalGlInt(gl, GL_ACTIVE_TEXTURE, GL_TEXTURE0));
        EXPECT_TRUE(compareGlobalGlInt(gl, GL_TEXTURE_BINDING_2D, 0));

        EXPECT_TRUE(compareParameter(GL_TEXTURE_MIN_FILTER,
                                     GL_NEAREST_MIPMAP_LINEAR));
        EXPECT_TRUE(compareParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        EXPECT_TRUE(compareParameter(GL_TEXTURE_WRAP_S, GL_REPEAT));
        EXPECT_TRUE(compareParameter(GL_TEXTURE_WRAP_T, GL_REPEAT));
    }

    void changedStateCheck() override {
        EXPECT_EQ(GL_TRUE, gl->glIsTexture(m_object_name));

        EXPECT_TRUE(compareGlobalGlInt(gl, GL_ACTIVE_TEXTURE, GL_TEXTURE0));
        EXPECT_TRUE(
                compareGlobalGlInt(gl, GL_TEXTURE_BINDING_2D, m_object_name));

        EXPECT_TRUE(compareParameter(GL_TEXTURE_MIN_FILTER, m_state.minFilter));
        EXPECT_TRUE(compareParameter(GL_TEXTURE_MAG_FILTER, m_state.magFilter));
        EXPECT_TRUE(compareParameter(GL_TEXTURE_WRAP_S, m_state.wrapS));
        EXPECT_TRUE(compareParameter(GL_TEXTURE_WRAP_T, m_state.wrapT));
    }

    void stateChange() override {
        gl->glGenTextures(1, &m_object_name);

        // Bind to texture unit TEXTURE0, GL_TEXTURE_2D for test simplicity
        gl->glActiveTexture(GL_TEXTURE0);
        gl->glBindTexture(GL_TEXTURE_2D, m_object_name);

        m_state_changer();
    }

    void setStateChanger(std::function<void()> changer) {
        m_state_changer = changer;
    }

protected:
    // Compares a symbolic constant value |expected| against the parameter named
    // |paramName| of the texture object which is bound in unit TEXTURE0's
    // binding for TEXTURE_2D.
    testing::AssertionResult compareParameter(GLenum paramName,
                                              GLenum expected) {
        GLint actual;
        gl->glGetTexParameteriv(GL_TEXTURE_2D, paramName, &actual);
        return compareValue<GLint>(
                expected, actual,
                "GL texture object " + std::to_string(m_object_name) +
                        " mismatch for param " + describeGlEnum(paramName));
    }

    GLuint m_object_name;
    GlTextureObjectState m_state;

    std::function<void()> m_state_changer = [] {};
};

TEST_F(SnapshotGlTextureObjectTest, SetObjectParameters) {
    setStateChanger([this] {
        m_state = {GL_LINEAR, GL_NEAREST, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE};
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            m_state.minFilter);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                            m_state.magFilter);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_state.wrapS);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_state.wrapT);
    });
    doCheckedSnapshot();
}

}  // namespace emugl
