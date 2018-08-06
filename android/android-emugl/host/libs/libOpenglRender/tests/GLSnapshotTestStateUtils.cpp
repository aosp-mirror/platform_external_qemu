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

std::vector<GLubyte> getTextureImageData(const GLESv2Dispatch* gl,
                                         GLuint texture,
                                         GLenum target,
                                         GLint level,
                                         GLsizei width,
                                         GLsizei height,
                                         GLenum format,
                                         GLenum type) {
    std::vector<GLubyte> out = {};
    out.resize(width * height *
               glUtilsPixelBitSize(GL_RGBA /* format */,
                                   GL_UNSIGNED_BYTE /* type */) / 8);

    // switch to auxiliary framebuffer
    GLint oldFramebuffer;
    gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);

    GLuint auxFramebuffer;
    gl->glGenFramebuffers(1, &auxFramebuffer);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, auxFramebuffer);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target,
                               texture, level);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());
    gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                     out.data());  // TODO(benzene): flexible format/type?
                                   // seems like RGBA/UNSIGNED_BYTE is the only
                                   // guaranteed supported format+type
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

    // restore old framebuffer
    gl->glBindFramebuffer(GL_FRAMEBUFFER, oldFramebuffer);
    gl->glDeleteFramebuffers(1, &auxFramebuffer);
    EXPECT_EQ(GL_NO_ERROR, gl->glGetError());

    return out;
}

}  // namespace emugl
