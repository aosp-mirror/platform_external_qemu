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

#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

namespace emugl {

GLuint createBuffer(const GLESv2Dispatch* gl, GlBufferData data) {
    // We bind to GL_ARRAY_BUFFER in order to set up buffer data,
    // so let's hold on to what the old binding was so we can restore it
    GLuint currentArrayBuffer;
    gl->glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&currentArrayBuffer);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

    GLuint name;
    gl->glGenBuffers(1, &name);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

    gl->glBindBuffer(GL_ARRAY_BUFFER, name);
    gl->glBufferData(GL_ARRAY_BUFFER, data.size, data.bytes, data.usage);

    // Restore the old binding
    gl->glBindBuffer(GL_ARRAY_BUFFER, currentArrayBuffer);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    return name;
};

GLuint loadAndCompileShader(const GLESv2Dispatch* gl,
                            GLenum shaderType,
                            const char* src) {
    GLuint shader = gl->glCreateShader(shaderType);
    gl->glShaderSource(shader, 1, (const GLchar* const*)&src, nullptr);
    gl->glCompileShader(shader);

    GLint compileStatus;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    EXPECT_EQ(GL_TRUE, compileStatus);

    if (compileStatus != GL_TRUE) {
        GLsizei infoLogLength;
        gl->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog;
        infoLog.resize(infoLogLength);
        gl->glGetShaderInfoLog(shader, infoLogLength, nullptr, &infoLog[0]);
        fprintf(stderr, "%s: fail to compile. infolog %s\n", __func__,
                &infoLog[0]);
    }

    return shader;
}

}  // namespace emugl
