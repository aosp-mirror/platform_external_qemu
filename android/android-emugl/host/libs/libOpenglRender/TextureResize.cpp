/*
* Copyright (C) 2015 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "TextureResize.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "android/utils/debug.h"

#include "emugl/common/misc.h"
#include "emugl/common/mutex.h"

#include "GLES2/gl2ext.h"

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <string>
#include <utility>

#define ERR(...)  fprintf(stderr, __VA_ARGS__)
#define V(...)  VERBOSE_PRINT(gles,__VA_ARGS__)
#define MAX_FACTOR_POWER 4

static const char kCommonShaderSource[] =
    "precision mediump float;\n"
    "varying vec2 vUV00, vUV01;\n"
    "#if FACTOR > 2\n"
    "varying vec2 vUV02, vUV03;\n"
    "#if FACTOR > 4\n"
    "varying vec2 vUV04, vUV05, vUV06, vUV07;\n"
    "#if FACTOR > 8\n"
    "varying vec2 vUV08, vUV09, vUV10, vUV11, vUV12, vUV13, vUV14, vUV15;\n"
    "#endif\n"
    "#endif\n"
    "#endif\n";

static const char kVertexShaderSource[] =
    "attribute vec2 aPosition;\n"

    "void main() {\n"
    "  gl_Position = vec4(aPosition, 0, 1);\n"
    "  vec2 uv = ((aPosition + 1.0) / 2.0) + 0.5 / kDimension;\n"
    "  vUV00 = uv;\n"
    "  #ifdef HORIZONTAL\n"
    "  vUV01 = uv + vec2( 1.0 / kDimension.x, 0);\n"
    "  #if FACTOR > 2\n"
    "  vUV02 = uv + vec2( 2.0 / kDimension.x, 0);\n"
    "  vUV03 = uv + vec2( 3.0 / kDimension.x, 0);\n"
    "  #if FACTOR > 4\n"
    "  vUV04 = uv + vec2( 4.0 / kDimension.x, 0);\n"
    "  vUV05 = uv + vec2( 5.0 / kDimension.x, 0);\n"
    "  vUV06 = uv + vec2( 6.0 / kDimension.x, 0);\n"
    "  vUV07 = uv + vec2( 7.0 / kDimension.x, 0);\n"
    "  #if FACTOR > 8\n"
    "  vUV08 = uv + vec2( 8.0 / kDimension.x, 0);\n"
    "  vUV09 = uv + vec2( 9.0 / kDimension.x, 0);\n"
    "  vUV10 = uv + vec2(10.0 / kDimension.x, 0);\n"
    "  vUV11 = uv + vec2(11.0 / kDimension.x, 0);\n"
    "  vUV12 = uv + vec2(12.0 / kDimension.x, 0);\n"
    "  vUV13 = uv + vec2(13.0 / kDimension.x, 0);\n"
    "  vUV14 = uv + vec2(14.0 / kDimension.x, 0);\n"
    "  vUV15 = uv + vec2(15.0 / kDimension.x, 0);\n"
    "  #endif\n"  // FACTOR > 8
    "  #endif\n"  // FACTOR > 4
    "  #endif\n"  // FACTOR > 2

    "  #else\n"
    "  vUV01 = uv + vec2(0,  1.0 / kDimension.y);\n"
    "  #if FACTOR > 2\n"
    "  vUV02 = uv + vec2(0,  2.0 / kDimension.y);\n"
    "  vUV03 = uv + vec2(0,  3.0 / kDimension.y);\n"
    "  #if FACTOR > 4\n"
    "  vUV04 = uv + vec2(0,  4.0 / kDimension.y);\n"
    "  vUV05 = uv + vec2(0,  5.0 / kDimension.y);\n"
    "  vUV06 = uv + vec2(0,  6.0 / kDimension.y);\n"
    "  vUV07 = uv + vec2(0,  7.0 / kDimension.y);\n"
    "  #if FACTOR > 8\n"
    "  vUV08 = uv + vec2(0,  8.0 / kDimension.y);\n"
    "  vUV09 = uv + vec2(0,  9.0 / kDimension.y);\n"
    "  vUV10 = uv + vec2(0, 10.0 / kDimension.y);\n"
    "  vUV11 = uv + vec2(0, 11.0 / kDimension.y);\n"
    "  vUV12 = uv + vec2(0, 12.0 / kDimension.y);\n"
    "  vUV13 = uv + vec2(0, 13.0 / kDimension.y);\n"
    "  vUV14 = uv + vec2(0, 14.0 / kDimension.y);\n"
    "  vUV15 = uv + vec2(0, 15.0 / kDimension.y);\n"
    "  #endif\n"  // FACTOR > 8
    "  #endif\n"  // FACTOR > 4
    "  #endif\n"  // FACTOR > 2
    "  #endif\n"  // HORIZONTAL/VERTICAL
    "}\n";

const char kFragmentShaderSource[] =
    "uniform sampler2D uTexture;\n"

    "vec3 read(vec2 uv) {\n"
    "  vec3 r = texture2D(uTexture, uv).rgb;\n"
    "  #ifdef HORIZONTAL\n"
    "  r.rgb = pow(r.rgb, vec3(2.2));\n"
    "  #endif\n"
    "  return r;\n"
    "}\n"

    "void main() {\n"
    "  vec3 sum = read(vUV00) + read(vUV01);\n"
    "  #if FACTOR > 2\n"
    "  sum += read(vUV02) + read(vUV03);\n"
    "  #if FACTOR > 4\n"
    "  sum += read(vUV04) + read(vUV05) + read(vUV06) + read(vUV07);\n"
    "  #if FACTOR > 8\n"
    "  sum += read(vUV08) + read(vUV09) + read(vUV10) + read(vUV11) +"
    "      read(vUV12) + read(vUV13) + read(vUV14) + read(vUV15);\n"
    "  #endif\n"
    "  #endif\n"
    "  #endif\n"
    "  sum /= float(FACTOR);\n"
    "  #ifdef VERTICAL\n"
    "  sum.rgb = pow(sum.rgb, vec3(1.0 / 2.2));\n"
    "  #endif\n"
    "  gl_FragColor = vec4(sum.rgb, 1.0);\n"
    "}\n";

// Vertex shader for anti-aliasing - doesn't do anything special.
const char kGenericVertexShaderSource[] = R"(
    attribute vec2 position;
    attribute vec2 inCoord;
    varying vec2 outCoord;
    void main(void) {
        gl_Position = vec4(position.x, position.y, 0.0, 1.0);
        outCoord = inCoord;
    })";

// Fragment shader
const char kGenericFragmentShaderSource[] = R"(
    precision mediump float;
    uniform sampler2D texSampler;
    varying vec2 outCoord;
    void main(void) {
        gl_FragColor = texture2D(texSampler, outCoord);
    }
)";

static const float kVertexData[] = {-1, -1, 3, -1, -1, 3};
static emugl::Mutex s_programLock;
static std::vector<GLuint> s_programsToRelease;

static GLuint createShader(GLenum type, std::initializer_list<const char*> source) {
    GLint success, infoLength;

    GLuint shader = s_gles2.glCreateShader(type);
    if (shader) {
        s_gles2.glShaderSource(shader, source.size(), source.begin(), nullptr);
        s_gles2.glCompileShader(shader);
        s_gles2.glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE) {
            s_gles2.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
            std::string infoLog(infoLength + 1, '\0');
            s_gles2.glGetShaderInfoLog(shader, infoLength, nullptr, &infoLog[0]);
            ERR("%s shader compile failed:\n%s\n",
                    (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment",
                    infoLog.c_str());
            s_gles2.glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

static void attachShaders(TextureResize::Framebuffer* fb, const char* factorDefine,
        const char* dimensionDefine, GLuint width, GLuint height) {

    std::ostringstream dimensionConst;
    dimensionConst << "const vec2 kDimension = vec2(" << width << ", " << height << ");\n";

    GLuint vShader = createShader(GL_VERTEX_SHADER, {
            factorDefine, dimensionDefine,
            kCommonShaderSource, dimensionConst.str().c_str(), kVertexShaderSource
    });
    GLuint fShader = createShader(GL_FRAGMENT_SHADER, {
        factorDefine, dimensionDefine, kCommonShaderSource, kFragmentShaderSource
    });

    if (!vShader || !fShader) {
        return;
    }
    if (!fb->program) {
        fb->program = s_gles2.glCreateProgram();
    }
    s_gles2.glAttachShader(fb->program, vShader);
    s_gles2.glAttachShader(fb->program, fShader);
    s_gles2.glLinkProgram(fb->program);
    s_gles2.glDeleteShader(vShader);
    s_gles2.glDeleteShader(fShader);

    fb->aPosition = s_gles2.glGetAttribLocation(fb->program, "aPosition");
    fb->uTexture = s_gles2.glGetUniformLocation(fb->program, "uTexture");
}

TextureResize::TextureResize(GLuint width, GLuint height) :
        mWidth(width),
        mHeight(height),
        mFactor(1),
        mFBWidth({0,}),
        mFBHeight({0,}),
        // Use unsigned byte as the default since it has the most support
        // and is the input/output format in the end
        // (TODO) until HDR is common on both guest and host, and we'll
        // cross that bridge when we get there.
        mTextureDataType(GL_UNSIGNED_BYTE) {

    // Fix color banding by trying to use a texture type with a high precision.
    const char* exts = (const char*)s_gles2.glGetString(GL_EXTENSIONS);

    bool hasColorBufferFloat =
        emugl::getRenderer() == SELECTED_RENDERER_HOST ||
        emugl::hasExtension(exts, "GL_EXT_color_buffer_float");
    bool hasColorBufferHalfFloat =
        emugl::hasExtension(exts, "GL_EXT_color_buffer_half_float");
    bool hasTextureFloat =
        emugl::hasExtension(exts, "GL_OES_texture_float");
    bool hasTextureHalfFloat =
        emugl::hasExtension(exts, "GL_OES_texture_half_float");
    bool hasTextureFloatLinear =
        emugl::hasExtension(exts, "GL_OES_texture_float_linear");

    if (hasColorBufferFloat && hasTextureFloat) {
        mTextureDataType = GL_FLOAT;
    } else if (hasColorBufferHalfFloat && hasTextureHalfFloat) {
        mTextureDataType = GL_HALF_FLOAT_OES;
    }

    if (hasTextureFloat || hasTextureHalfFloat) {
        mTextureFilteringMode =
            hasTextureFloatLinear ? GL_LINEAR : GL_NEAREST;
    }

    s_gles2.glGenTextures(1, &mFBWidth.texture);
    s_gles2.glBindTexture(GL_TEXTURE_2D, mFBWidth.texture);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    s_gles2.glGenTextures(1, &mFBHeight.texture);
    s_gles2.glBindTexture(GL_TEXTURE_2D, mFBHeight.texture);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mTextureFilteringMode);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mTextureFilteringMode);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    s_gles2.glGenFramebuffers(1, &mFBWidth.framebuffer);
    s_gles2.glGenFramebuffers(1, &mFBHeight.framebuffer);

    s_gles2.glGenBuffers(1, &mVertexBuffer);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    s_gles2.glBufferData(GL_ARRAY_BUFFER, sizeof(kVertexData), kVertexData, GL_STATIC_DRAW);

    // Clear bindings.
    s_gles2.glBindTexture(GL_TEXTURE_2D, 0);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, 0);
}



TextureResize::~TextureResize() {
    GLuint fb[2] = {mFBWidth.framebuffer, mFBHeight.framebuffer};
    s_gles2.glDeleteFramebuffers(2, fb);

    GLuint tex[2] = {mFBWidth.texture, mFBHeight.texture};
    s_gles2.glDeleteTextures(2, tex);

    s_gles2.glDeleteBuffers(1, &mVertexBuffer);
    // b/242245912
    // There seems to be a mesa bug that we have to delete the
    // program in the post thread.
    android::base::AutoLock lock(s_programLock);
    s_programsToRelease.push_back(mFBWidth.program);
    s_programsToRelease.push_back(mFBHeight.program);
}

GLuint TextureResize::update(GLuint texture) {
    // Store the viewport. The viewport is clobbered due to the framebuffers.
    GLint vport[4] = { 0, };
    s_gles2.glGetIntegerv(GL_VIEWPORT, vport);
    GLint prevFbo = 0;
    s_gles2.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevFbo);
    // Correctly deal with rotated screens.
    GLint tWidth = vport[2], tHeight = vport[3];
    if ((mWidth < mHeight) != (tWidth < tHeight)) {
        std::swap(tWidth, tHeight);
    }

    // Compute the scaling factor needed to get an image just larger than the target viewport.
    unsigned int factor = 1;
    for (int i = 0, w = mWidth / 2, h = mHeight / 2;
        i < MAX_FACTOR_POWER && w >= tWidth && h >= tHeight;
        i++, w /= 2, h /= 2, factor *= 2) {
    }

    // No resizing needed if factor == 1
    if (factor == 1) {
        return texture;
    }

    s_gles2.glGetError(); // Clear any GL errors.
    setupFramebuffers(factor);
    resize(texture);
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
    s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]); // Restore the viewport.
    // If there was an error while resizing, just use the unscaled texture.
    GLenum error = s_gles2.glGetError();
    if (error != GL_NO_ERROR) {
        V("GL error while resizing: 0x%x (ignored)\n", error);
        return texture;
    }

    return mFBHeight.texture;
}

GLuint TextureResize::update(GLuint texture, int width, int height, SkinRotation rotation) {
    if (mGenericResizer.get() == nullptr) {
        mGenericResizer.reset(new TextureResize::GenericResizer());
    }
    return mGenericResizer->draw(texture, width, height, rotation);
}

void TextureResize::setupFramebuffers(unsigned int factor) {
    if (factor == mFactor) {
        // The factor hasn't changed, no need to update the framebuffers.
        return;
    }

    // Update the framebuffer sizes to match the new factor.
    s_gles2.glBindTexture(GL_TEXTURE_2D, mFBWidth.texture);
    s_gles2.glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB, mWidth / factor, mHeight, 0, GL_RGB,
                mTextureDataType, nullptr);
    s_gles2.glBindTexture(GL_TEXTURE_2D, 0);

    s_gles2.glBindTexture(GL_TEXTURE_2D, mFBHeight.texture);
    s_gles2.glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB, mWidth / factor, mHeight / factor, 0, GL_RGB,
                mTextureDataType, nullptr);
    s_gles2.glBindTexture(GL_TEXTURE_2D, 0);

    // Update the shaders to the new factor.
    std::ostringstream factorDefine;
    factorDefine << "#define FACTOR " << factor << '\n';
    const std::string factorDefineStr = factorDefine.str();
    attachShaders(&mFBWidth, factorDefineStr.c_str(), "#define HORIZONTAL\n", mWidth, mHeight);
    attachShaders(&mFBHeight, factorDefineStr.c_str(), "#define VERTICAL\n", mWidth, mHeight);

    mFactor = factor;

    s_gles2.glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureResize::resize(GLuint texture) {
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    s_gles2.glActiveTexture(GL_TEXTURE0);

    // First scale the horizontal dimension by rendering the input texture to a scaled framebuffer.
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, mFBWidth.framebuffer);
    s_gles2.glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFBWidth.texture, 0);
    s_gles2.glClear(GL_COLOR_BUFFER_BIT);
    s_gles2.glViewport(0, 0, mWidth / mFactor, mHeight);
    s_gles2.glUseProgram(mFBWidth.program);
    s_gles2.glEnableVertexAttribArray(mFBWidth.aPosition);
    s_gles2.glVertexAttribPointer(mFBWidth.aPosition, 2, GL_FLOAT, GL_FALSE, 0, 0);
    s_gles2.glBindTexture(GL_TEXTURE_2D, texture);

    // Store the current texture filters and set to nearest for scaling.
    GLint mag_filter, min_filter;
    s_gles2.glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mag_filter);
    s_gles2.glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    s_gles2.glUniform1i(mFBWidth.uTexture, 0);
    s_gles2.glDrawArrays(GL_TRIANGLES, 0, sizeof(kVertexData) / (2 * sizeof(float)));

    // Restore the previous texture filters.
    s_gles2.glDisableVertexAttribArray(mFBWidth.aPosition);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    s_gles2.glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    // Secondly, scale the vertical dimension using the second framebuffer.
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, mFBHeight.framebuffer);
    s_gles2.glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFBHeight.texture, 0);
    s_gles2.glClear(GL_COLOR_BUFFER_BIT);
    s_gles2.glViewport(0, 0, mWidth / mFactor, mHeight / mFactor);
    s_gles2.glUseProgram(mFBHeight.program);
    s_gles2.glEnableVertexAttribArray(mFBHeight.aPosition);
    s_gles2.glVertexAttribPointer(mFBHeight.aPosition, 2, GL_FLOAT, GL_FALSE, 0, 0);
    s_gles2.glBindTexture(GL_TEXTURE_2D, mFBWidth.texture);
    s_gles2.glUniform1i(mFBHeight.uTexture, 0);
    s_gles2.glDrawArrays(GL_TRIANGLES, 0, sizeof(kVertexData) / (2 * sizeof(float)));
    s_gles2.glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    // Clear the bindings. (Viewport restored outside)
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, 0);
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_gles2.glBindTexture(GL_TEXTURE_2D, 0);
    s_gles2.glDisableVertexAttribArray(mFBHeight.aPosition);
    s_gles2.glUseProgram(0);
    android::base::AutoLock lock(s_programLock);
    while (s_programsToRelease.size()) {
        s_gles2.glDeleteProgram(s_programsToRelease.back());
        s_programsToRelease.pop_back();
    }
}

struct Vertex {
    float pos[2];
    float coord[2];
};

TextureResize::GenericResizer::GenericResizer() :
        mProgram(0),
        mVertexBuffer(0),
        mIndexBuffer(0),
        mWidth(0),
        mHeight(0) {
    GLuint vertex_shader =
            createShader(GL_VERTEX_SHADER, {kGenericVertexShaderSource});
    GLuint fragment_shader =
            createShader(GL_FRAGMENT_SHADER, {kGenericFragmentShaderSource});

    mProgram = s_gles2.glCreateProgram();
    s_gles2.glAttachShader(mProgram, vertex_shader);
    s_gles2.glAttachShader(mProgram, fragment_shader);
    s_gles2.glLinkProgram(mProgram);

    // Shader objects no longer needed.
    s_gles2.glDeleteShader(vertex_shader);
    s_gles2.glDeleteShader(fragment_shader);

    // Check for errors.
    GLint success;
    s_gles2.glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infolog[256];
        s_gles2.glGetProgramInfoLog(mProgram, sizeof(infolog), 0, infolog);
        fprintf(stderr, "Could not create/link program: %s\n", infolog);
        return;
    }

    // Get all the attributes and uniforms.
    mPositionAttribLocation =
            s_gles2.glGetAttribLocation(mProgram, "position");
    mInCoordAttribLocation =
            s_gles2.glGetAttribLocation(mProgram, "inCoord");
    mInputUniformLocation =
            s_gles2.glGetUniformLocation(mProgram, "texSampler");

    // Create vertex buffers.
    static const Vertex kVertices[] = {
        // 0 degree
        {{ +1, -1 }, { +1, +0 }},
        {{ +1, +1 }, { +1, +1 }},
        {{ -1, +1 }, { +0, +1 }},
        {{ -1, -1 }, { +0, +0 }},
        // 90 degree clock-wise
        {{ +1, -1 }, { +0, +0 }},
        {{ +1, +1 }, { +1, +0 }},
        {{ -1, +1 }, { +1, +1 }},
        {{ -1, -1 }, { +0, +1 }},
        // 180 degree clock-wise
        {{ +1, -1 }, { +0, +1 }},
        {{ +1, +1 }, { +0, +0 }},
        {{ -1, +1 }, { +1, +0 }},
        {{ -1, -1 }, { +1, +1 }},
        // 270 degree clock-wise
        {{ +1, -1 }, { +1, +1 }},
        {{ +1, +1 }, { +0, +1 }},
        {{ -1, +1 }, { +0, +0 }},
        {{ -1, -1 }, { +1, +0 }},
    };
    s_gles2.glGenBuffers(1, &mVertexBuffer);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    s_gles2.glBufferData(GL_ARRAY_BUFFER,
                         sizeof(kVertices),
                         kVertices,
                         GL_STATIC_DRAW);

    // indices for predefined rotation angles.
    static const GLubyte kIndices[] = {
        0, 1, 2, 2, 3, 0,      // 0
        4, 5, 6, 6, 7, 4,      // 90
        8, 9, 10, 10, 11, 8,   // 180
        12, 13, 14, 14, 15, 12, // 270
    };
    s_gles2.glGenBuffers(1, &mIndexBuffer);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    s_gles2.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         sizeof(kIndices),
                         kIndices,
                         GL_STATIC_DRAW);

    s_gles2.glGenTextures(1, &mFrameBuffer.texture);
    s_gles2.glBindTexture(GL_TEXTURE_2D, mFrameBuffer.texture);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    s_gles2.glGenFramebuffers(1, &mFrameBuffer.framebuffer);

    // Clear bindings.
    s_gles2.glBindTexture(GL_TEXTURE_2D, 0);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, 0);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLuint TextureResize::GenericResizer::draw(GLuint texture, int width, int height,
                                           SkinRotation rotation) {
    if (mWidth != width || mHeight != height) {
        // update the framebuffer to match the new resolution
        mWidth = width;
        mHeight = height;
        s_gles2.glBindTexture(GL_TEXTURE_2D, mFrameBuffer.texture);
        s_gles2.glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB,
            GL_UNSIGNED_BYTE, nullptr);
        s_gles2.glBindTexture(GL_TEXTURE_2D, 0);

    }

    // Store the viewport.
    GLint vport[4] = { 0, };
    s_gles2.glGetIntegerv(GL_VIEWPORT, vport);

    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer.framebuffer);
    s_gles2.glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFrameBuffer.texture, 0);
    s_gles2.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    s_gles2.glViewport(0, 0, mWidth, mHeight);
    s_gles2.glUseProgram(mProgram);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    s_gles2.glEnableVertexAttribArray(mPositionAttribLocation);
    s_gles2.glVertexAttribPointer(mPositionAttribLocation,
                                  2, // components per attrib
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(Vertex), // stride
                                  0); // offset
    s_gles2.glEnableVertexAttribArray(mInCoordAttribLocation);
    s_gles2.glVertexAttribPointer(mInCoordAttribLocation,
                                  2,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(Vertex),
                                  reinterpret_cast<GLvoid*>(sizeof(float) * 2));
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    s_gles2.glActiveTexture(GL_TEXTURE0);
    s_gles2.glBindTexture(GL_TEXTURE_2D, texture);
    s_gles2.glUniform1i(mInputUniformLocation, 0);
    intptr_t indexShift;
    switch(rotation) {
    case SKIN_ROTATION_0:
        indexShift = 0;
        break;
    case SKIN_ROTATION_90:
        indexShift = 6;
        break;
    case SKIN_ROTATION_180:
        indexShift = 12;
        break;
    case SKIN_ROTATION_270:
        indexShift = 18;
        break;
    }
    s_gles2.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, (const GLvoid*)indexShift);

    // Clear the bindings.
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, 0);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    s_gles2.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_gles2.glBindTexture(GL_TEXTURE_2D, 0);
    s_gles2.glDisableVertexAttribArray(mPositionAttribLocation);
    s_gles2.glDisableVertexAttribArray(mInCoordAttribLocation);

    // Restore the viewport.
    s_gles2.glViewport(vport[0], vport[1], vport[2], vport[3]);

    return mFrameBuffer.texture;
}

TextureResize::GenericResizer::~GenericResizer() {
    s_gles2.glDeleteFramebuffers(1, &mFrameBuffer.framebuffer);
    s_gles2.glDeleteTextures(1, &mFrameBuffer.texture);
    s_gles2.glUseProgram(0);
    s_gles2.glDeleteProgram(mProgram);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, 0);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    s_gles2.glDeleteBuffers(1, &mVertexBuffer);
    s_gles2.glDeleteBuffers(1, &mIndexBuffer);
}


