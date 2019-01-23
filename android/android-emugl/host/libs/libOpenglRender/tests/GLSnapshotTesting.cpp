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

#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"

#include "GLTestUtils.h"
#include "OpenGLTestContext.h"

#include <gtest/gtest.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

namespace emugl {

using android::base::LazyInstance;
using android::base::StdioStream;
using android::snapshot::TextureLoader;
using android::snapshot::TextureSaver;

std::string describeGlEnum(GLenum enumValue) {
    std::ostringstream description;
    description << "0x" << std::hex << enumValue
                << " (" << getEnumString(enumValue) << ")";
    return description.str();
}

template <class T>
testing::AssertionResult compareValue(T expected,
                                      T actual,
                                      const std::string& description) {
    if (expected != actual) {
        return testing::AssertionFailure()
               << description << "\n\tvalue was:\t"
               << testing::PrintToString(actual) << "\n\t expected:\t"
               << testing::PrintToString(expected) << "\t";
    }
    return testing::AssertionSuccess();
}

template testing::AssertionResult compareValue<GLboolean>(GLboolean,
                                                          GLboolean,
                                                          const std::string&);
template testing::AssertionResult compareValue<GLint>(GLint,
                                                      GLint,
                                                      const std::string&);
template testing::AssertionResult compareValue<GLfloat>(GLfloat,
                                                        GLfloat,
                                                        const std::string&);

testing::AssertionResult compareGlobalGlBoolean(const GLESv2Dispatch* gl,
                                                GLenum name,
                                                GLboolean expected) {
    GLboolean current;
    gl->glGetBooleanv(name, &current);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    return compareValue<GLboolean>(expected, current,
                                   "GL global boolean mismatch for parameter " +
                                           describeGlEnum(name) + ":");
}

testing::AssertionResult compareGlobalGlInt(const GLESv2Dispatch* gl,
                                            GLenum name,
                                            GLint expected) {
    GLint current;
    gl->glGetIntegerv(name, &current);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    return compareValue<GLint>(expected, current,
                               "GL global int mismatch for parameter " +
                                       describeGlEnum(name) + ":");
}

testing::AssertionResult compareGlobalGlFloat(const GLESv2Dispatch* gl,
                                              GLenum name,
                                              GLfloat expected) {
    GLfloat current;
    gl->glGetFloatv(name, &current);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    return compareValue<GLfloat>(expected, current,
                                 "GL global float mismatch for parameter " +
                                         describeGlEnum(name) + ":");
}

template <class T>
testing::AssertionResult compareVector(const std::vector<T>& expected,
                                       const std::vector<T>& actual,
                                       const std::string& description) {
    std::stringstream message;
    if (expected.size() != actual.size()) {
        message << "    (!) sizes do not match (actual " << actual.size()
                << ", expected " << expected.size() << ")\n";
    }

    int mismatches = 0;
    for (int i = 0; i < expected.size(); i++) {
        if (i >= actual.size()) {
            if (mismatches < 10) {
                mismatches++;
                message << "    no match for:\t"
                        << testing::PrintToString(expected[i]) << "\n";
            } else {
                mismatches += expected.size() - i;
                message << "\n    nothing can match remaining elements.\n";
                break;
            }
        } else if (expected[i] != actual[i]) {
            mismatches++;
            if (mismatches < 15) {
                message << "    at index " << i << ":\n\tvalue was:\t"
                        << testing::PrintToString(actual[i])
                        << "\n\t expected:\t"
                        << testing::PrintToString(expected[i]) << "\n";
            } else if (mismatches == 15) {
                message << "    ... and indices " << i;
            } else if (mismatches < 50) {
                message << ", " << i;
            } else if (mismatches == 50) {
                message << ", etc...";
            }
        }
    }
    if (mismatches > 0) {
        return testing::AssertionFailure()
               << description << " had " << mismatches << " mismatches.\n"
               << "  expected: " << testing::PrintToString(expected) << "\n"
               << "    actual: " << testing::PrintToString(actual) << "\n"
               << message.str() << "  \n";
    }
    return testing::AssertionSuccess();
}

template testing::AssertionResult compareVector<GLboolean>(
        const std::vector<GLboolean>&,
        const std::vector<GLboolean>&,
        const std::string&);
template testing::AssertionResult compareVector<GLint>(
        const std::vector<GLint>&,
        const std::vector<GLint>&,
        const std::string&);
template testing::AssertionResult compareVector<GLfloat>(
        const std::vector<GLfloat>&,
        const std::vector<GLfloat>&,
        const std::string&);

testing::AssertionResult compareGlobalGlBooleanv(
        const GLESv2Dispatch* gl,
        GLenum name,
        const std::vector<GLboolean>& expected,
        GLuint size) {
    std::vector<GLboolean> current;
    current.resize(std::max(size, static_cast<GLuint>(expected.size())));
    gl->glGetBooleanv(name, &current[0]);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    return compareVector<GLboolean>(
            expected, current,
            "GL global booleanv parameter " + describeGlEnum(name));
}

testing::AssertionResult compareGlobalGlIntv(const GLESv2Dispatch* gl,
                                             GLenum name,
                                             const std::vector<GLint>& expected,
                                             GLuint size) {
    std::vector<GLint> current;
    current.resize(std::max(size, static_cast<GLuint>(expected.size())));
    gl->glGetIntegerv(name, &current[0]);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    return compareVector<GLint>(
            expected, current,
            "GL global intv parameter " + describeGlEnum(name));
}

testing::AssertionResult compareGlobalGlFloatv(
        const GLESv2Dispatch* gl,
        GLenum name,
        const std::vector<GLfloat>& expected,
        GLuint size) {
    std::vector<GLfloat> current;
    current.resize(std::max(size, static_cast<GLuint>(expected.size())));
    gl->glGetFloatv(name, &current[0]);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    return compareVector<GLfloat>(
            expected, current,
            "GL global floatv parameter " + describeGlEnum(name));
}

void SnapshotTest::SetUp() {
    GLTest::SetUp();
    mTestSystem.getTempRoot()->makeSubDir("Snapshots");
    mSnapshotPath = mTestSystem.getTempRoot()->makeSubPath("Snapshots");
}

void SnapshotTest::saveSnapshot(const std::string streamFile,
                                const std::string textureFile) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();

    std::unique_ptr<StdioStream> m_stream(new StdioStream(
            android_fopen(streamFile.c_str(), "wb"), StdioStream::kOwner));
    auto egl_stream = static_cast<EGLStream>(m_stream.get());
    std::unique_ptr<TextureSaver> m_texture_saver(new TextureSaver(StdioStream(
            android_fopen(textureFile.c_str(), "wb"), StdioStream::kOwner)));

    egl->eglPreSaveContext(m_display, m_context, egl_stream);
    egl->eglSaveAllImages(m_display, egl_stream, &m_texture_saver);

    egl->eglSaveContext(m_display, m_context, egl_stream);

    // Skip saving a bunch of FrameBuffer's fields
    // Skip saving colorbuffers
    // Skip saving window surfaces

    egl->eglSaveConfig(m_display, m_config, egl_stream);

    // Skip saving a bunch of process-owned objects

    egl->eglPostSaveContext(m_display, m_context, egl_stream);

    m_stream->close();
    m_texture_saver->done();
}

void SnapshotTest::loadSnapshot(const std::string streamFile,
                                const std::string textureFile) {
    const EGLDispatch* egl = LazyLoadedEGLDispatch::get();

    std::unique_ptr<StdioStream> m_stream(new StdioStream(
            android_fopen(streamFile.c_str(), "rb"), StdioStream::kOwner));
    auto egl_stream = static_cast<EGLStream>(m_stream.get());
    std::shared_ptr<TextureLoader> m_texture_loader(
            new TextureLoader(StdioStream(android_fopen(textureFile.c_str(), "rb"),
                                          StdioStream::kOwner)));

    egl->eglLoadAllImages(m_display, egl_stream, &m_texture_loader);

    EGLint contextAttribs[5] = {EGL_CONTEXT_CLIENT_VERSION, 3,
                                EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE};

    m_context = egl->eglLoadContext(m_display, &contextAttribs[0], egl_stream);
    m_config = egl->eglLoadConfig(m_display, egl_stream);
    m_surface = pbufferSurface(m_display, m_config, kTestSurfaceSize[0],
                               kTestSurfaceSize[0]);
    egl->eglPostLoadAllImages(m_display, egl_stream);

    m_stream->close();
    m_texture_loader->join();
    egl->eglMakeCurrent(m_display, m_surface, m_surface, m_context);
}

void SnapshotTest::preloadReset() {
    GLTest::TearDown();
    GLTest::SetUp();
}

void SnapshotTest::doSnapshot(std::function<void()> preloadCheck = [] {}) {
    std::string timeStamp =
            std::to_string(android::base::System::get()->getUnixTime());
    std::string snapshotFile =
            mSnapshotPath + PATH_SEP "snapshot_" + timeStamp + ".snap";
    std::string textureFile =
            mSnapshotPath + PATH_SEP "textures_" + timeStamp + ".stex";

    saveSnapshot(snapshotFile, textureFile);

    preloadReset();
    preloadCheck();

    loadSnapshot(snapshotFile, textureFile);

    EXPECT_NE(m_context, EGL_NO_CONTEXT);
    EXPECT_NE(m_surface, EGL_NO_SURFACE);
}

void SnapshotPreserveTest::doCheckedSnapshot() {
    {
        SCOPED_TRACE("during pre-snapshot default state check");
        defaultStateCheck();
        ASSERT_EQ(GL_NO_ERROR, gl->glGetError());
    }
    {
        SCOPED_TRACE("during pre-snapshot state change");
        stateChange();
        ASSERT_EQ(GL_NO_ERROR, gl->glGetError());
    }
    {
        SCOPED_TRACE("during pre-snapshot changed state check");
        changedStateCheck();
    }
    SnapshotTest::doSnapshot([this] {
        SCOPED_TRACE("during post-reset default state check");
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
        defaultStateCheck();
    });
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    {
        SCOPED_TRACE("during post-snapshot changed state check");
        changedStateCheck();
        EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    }
}

}  // namespace emugl
