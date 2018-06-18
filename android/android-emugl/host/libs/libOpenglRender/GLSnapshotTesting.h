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

#pragma once

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

#include "OpenGLTestContext.h"

#include <map>
#include <memory>
#include <vector>

namespace emugl {
namespace snapshottest {

struct GlSampleCoverage {
    GLclampf value;
    GLboolean invert;
};

struct GlStencilFunc {
    GLenum func;
    GLint ref;
    GLuint mask;
};

struct GlStencilOp {
    GLenum sfail;
    GLenum dpfail;
    GLenum dppass;
};

// Capabilities which, according to the GLES2 spec, start disabled.
static const GLenum kGLES2CanBeEnabled[] = {GL_BLEND,
                                            GL_CULL_FACE,
                                            GL_DEPTH_TEST,
                                            GL_POLYGON_OFFSET_FILL,
                                            GL_SAMPLE_ALPHA_TO_COVERAGE,
                                            GL_SAMPLE_COVERAGE,
                                            GL_SCISSOR_TEST,
                                            GL_STENCIL_TEST};

// Capabilities which, according to the GLES2 spec, start enabled.
static const GLenum kGLES2CanBeDisabled[] = {GL_DITHER};

// Modes for CullFace
static const GLenum kGLES2CullFaceModes[] = {GL_BACK, GL_FRONT,
                                             GL_FRONT_AND_BACK};

// Modes for FrontFace
static const GLenum kGLES2FrontFaceModes[] = {GL_CCW, GL_CW};

// Valid Stencil test functions
static const GLenum kGLES2StencilFuncs[] = {GL_NEVER,   GL_ALWAYS,  GL_LESS,
                                            GL_LEQUAL,  GL_EQUAL,   GL_GEQUAL,
                                            GL_GREATER, GL_NOTEQUAL};
// Valid Stencil test result operations
static const GLenum kGLES2StencilOps[] = {GL_KEEP,      GL_ZERO,     GL_REPLACE,
                                          GL_INCR,      GL_DECR,     GL_INVERT,
                                          GL_INCR_WRAP, GL_DECR_WRAP};

// Valid Blend equation modes
static const GLenum kGLES2BlendEquationModes[] = {GL_FUNC_ADD, GL_FUNC_SUBTRACT,
                                                  GL_FUNC_REVERSE_SUBTRACT};

struct GlValues {
    std::vector<GLboolean> bools;
    std::vector<GLint> ints;
    std::vector<GLuint> uints;
    std::vector<GLenum> enums;
    std::vector<GLfloat> floats;
    std::vector<GLclampf> clampfs;
};

enum class GlGetterType { Booleanv, Integerv, Floatv, Enabled };

struct GlGlobalCheckInfo {
    GlGetterType getterType;
    GLuint size;
};

static const GlGlobalCheckInfo kEnableGlobalMeta = {GlGetterType::Enabled, 1};

static const std::map<GLenum, GlGlobalCheckInfo> kGLES2GlobalMeta = {
        {GL_BLEND, kEnableGlobalMeta},
        {GL_CULL_FACE, kEnableGlobalMeta},
        {GL_DEPTH_TEST, kEnableGlobalMeta},
        {GL_POLYGON_OFFSET_FILL, kEnableGlobalMeta},
        {GL_SAMPLE_ALPHA_TO_COVERAGE, kEnableGlobalMeta},
        {GL_SAMPLE_COVERAGE, kEnableGlobalMeta},
        {GL_SCISSOR_TEST, kEnableGlobalMeta},
        {GL_STENCIL_TEST, kEnableGlobalMeta},
        {GL_DITHER, kEnableGlobalMeta},
};

using GlobalStateMap = std::map<GLenum, GlValues>;

static const GlValues kDisabledCapabilityValues = {.bools = {false}};

static const GlobalStateMap kGLES2DefaultGlobalState = {
        {GL_BLEND, kDisabledCapabilityValues},
        {GL_CULL_FACE, kDisabledCapabilityValues},
        {GL_DEPTH_TEST, kDisabledCapabilityValues},
        {GL_POLYGON_OFFSET_FILL, kDisabledCapabilityValues},
        {GL_SAMPLE_ALPHA_TO_COVERAGE, kDisabledCapabilityValues},
        {GL_SAMPLE_COVERAGE, kDisabledCapabilityValues},
        {GL_SCISSOR_TEST, kDisabledCapabilityValues},
        {GL_STENCIL_TEST, kDisabledCapabilityValues},
        {GL_DITHER, {.bools = {true}}},
};

template <class T>
static std::vector<T> retreiveGlobal(const GLESv2Dispatch* gl,
                                     GlGetterType type,
                                     GLenum name,
                                     GLuint size) {
    std::vector<T> values;
    values.resize(size);
    switch (type) {
        case GlGetterType::Enabled:
            values.push_back(gl->glIsEnabled(name));
            break;
        case GlGetterType::Booleanv:
            gl->glGetBooleanv(name, (GLboolean*)&values);
            break;
        case GlGetterType::Integerv:
            gl->glGetIntegerv(name, (GLint*)&values);
            break;
        case GlGetterType::Floatv:
            gl->glGetFloatv(name, (GLfloat*)&values);
            break;
    }
    return values;
};

template <class T>
static std::function<void()> createChecker(const GLESv2Dispatch* gl,
                                           GLenum name,
                                           GlGetterType type,
                                           std::vector<T> expected) {
    fprintf(stderr, "creating checker for %d\n", name);
    return [gl, name, &type, &expected]() {
        fprintf(stderr, "retreiving globals %d\n", name);
        auto values = retreiveGlobal<T>(gl, type, name, expected.size());
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        for (int i = 0; i < values.size(); ++i) {
            fprintf(stderr, "checking value %d vs %d\n", expected[i],
                    values[i]);
            EXPECT_EQ(expected[i], values[i]);
        }
    };
};

using GlobalCheckerList = std::vector<std::function<void()>>;

class SnapshotTest : public gltest::GLTest {
public:
    SnapshotTest()
        : mTestSystem(PATH_SEP "progdir",
                      android::base::System::kProgramBitness,
                      PATH_SEP "homedir",
                      PATH_SEP "appdir") {}

    void SetUp() override;

    // Mimics FrameBuffer.onSave, with fewer objects to manage.
    // |streamFile| is a filename into which the snapshot will be saved.
    // |textureFile| is a filename into which the textures will be saved.
    void saveSnapshot(const std::string streamFile,
                      const std::string textureFile);

    // Mimics FrameBuffer.onLoad, with fewer objects to manage.
    // Assumes that a valid display is present.
    // |streamFile| is a filename from which the snapshot will be loaded.
    // |textureFile| is a filename from which the textures will be loaded.
    void loadSnapshot(const std::string streamFile,
                      const std::string textureFile);

    // Performs a teardown and reset of graphics objects in preparation for
    // a snapshot load.
    void preloadReset();

    // Mimics saving and then loading a graphics snapshot.
    // To verify that state has been reset to some default before the load,
    // assertions can be performed in |preloadCheck|.
    void doSnapshot(std::function<void()> preloadCheck);

protected:
    android::base::TestSystem mTestSystem;
    std::string mSnapshotPath = {};
};

// Abstract, for granular testing of snapshot's ability to preserve parts of GL
// state.
class SnapshotPreserveTest : public SnapshotTest {
public:
    // Asserts that we are working from a clean starting state.
    virtual void defaultStateCheck() {
        ADD_FAILURE() << "Snapshot test needs a default state check function.";
    }

    // Asserts that any expected changes to state have occurred.
    virtual void changedStateCheck() {
        ADD_FAILURE()
                << "Snapshot test needs a post-change state check function.";
    }

    // Modifies the state.
    virtual void stateChange() {
        ADD_FAILURE() << "Snapshot test needs a state-changer function.";
    }

    // Sets up a non-default state and asserts that a snapshot preserves it.
    virtual void doCheckedSnapshot();
};

// For testing preservation of pieces of GL state that can be checked via
// comparison(s) which are the same for both a known default and after
// predictable changes.
template <class T>
class SnapshotSetValueTest : public SnapshotPreserveTest {
public:
    // Configures the test to assert against values which it should consider
    // default and values which it should expect after changes.
    void setExpectedValues(T defaultValue, T changedValue) {
        m_default_value = std::unique_ptr<T>(new T(defaultValue));
        m_changed_value = std::unique_ptr<T>(new T(changedValue));
    }

    // Checks part of state against an expected value.
    virtual void stateCheck(T expected) {
        ADD_FAILURE() << "Snapshot test needs a state-check function.";
    }

    void defaultStateCheck() override { stateCheck(*m_default_value); }
    void changedStateCheck() override { stateCheck(*m_changed_value); }

    void doCheckedSnapshot() override {
        if (m_default_value == nullptr || m_changed_value == nullptr) {
            ADD_FAILURE() << "Snapshot test not provided expected values.";
        }
        SnapshotPreserveTest::doCheckedSnapshot();
    }

protected:
    std::unique_ptr<T> m_default_value;
    std::unique_ptr<T> m_changed_value;
};

class SnapshotChangeGlobalsTest : public SnapshotPreserveTest {
public:
    template <class T>
    void setExpectedGlobal(GLenum paramName,
                           std::vector<T> defaults,
                           std::vector<T> changed) {
        fprintf(stderr, "Setting expected global %d\n", paramName);
        m_default_checkers.push_back(createChecker<T>(
                gl, paramName, kGLES2GlobalMeta.at(paramName).getterType,
                defaults));
        m_changed_checkers.push_back(createChecker<T>(
                gl, paramName, kGLES2GlobalMeta.at(paramName).getterType,
                changed));
        fprintf(stderr, "Done setting expected global %s\n", "yay");
    }

    void defaultStateCheck() override {
        fprintf(stderr, "default state check %s\n", "yay");
        for (auto checker : m_default_checkers) {
            checker();
        }
        fprintf(stderr, "done with default state check %s\n", "yay");
    }
    void changedStateCheck() override {
        fprintf(stderr, "changed state check %s\n", "yay");
        for (auto checker : m_changed_checkers) {
            checker();
        }
        fprintf(stderr, "done with changed state check %s\n", "yay");
    }

    void doCheckedSnapshot() override {
        fprintf(stderr, "Starting snapshot %s\n", "yay");
        if (m_default_checkers.size() == 0 || m_changed_checkers.size() == 0) {
            ADD_FAILURE() << "Snapshot test was not given any expected values "
                             "to assert.";
        }
        SnapshotPreserveTest::doCheckedSnapshot();
    }

protected:
    GlobalCheckerList m_default_checkers;
    GlobalCheckerList m_changed_checkers;
};

/* TEST_HOPEFUL(SnapshotChangeGlobalsTest<>) {
    // specify getvalues
    // getFunctionv, GL_ENUM, size, defaultexpect, changedexpect
    // getFunctionv, GL_ENUM, size, defaultexpect, changedexpect

    // ... but size and function come free with GL_ENUM after a lookup.
    // at some point we will have to specify the defaults for everything too
    // but let's delay that for now

    // specify statechange
    // lambda( glfunction with params )
}
*/

}  // namespace snapshottest
}  // namespace emugl
