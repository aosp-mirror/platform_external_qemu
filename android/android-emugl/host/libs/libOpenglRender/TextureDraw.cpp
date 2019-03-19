// Copyright (C) 2015 The Android Open Source Project
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

#include "TextureDraw.h"

#include "DispatchTables.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#define ERR(...)  fprintf(stderr, __VA_ARGS__)

namespace {

// Helper function to create a new shader.
// |shaderType| is the shader type (e.g. GL_VERTEX_SHADER).
// |shaderText| is a 0-terminated C string for the shader source to use.
// On success, return the handle of the new compiled shader, or 0 on failure.
GLuint createShader(GLint shaderType, const char* shaderText) {
    // Create new shader handle and attach source.
    GLuint shader = s_gles2.glCreateShader(shaderType);
    if (!shader) {
        return 0;
    }
    const GLchar* text = static_cast<const GLchar*>(shaderText);
    const GLint textLen = ::strlen(shaderText);
    s_gles2.glShaderSource(shader, 1, &text, &textLen);

    // Compiler the shader.
    GLint success;
    s_gles2.glCompileShader(shader);
    s_gles2.glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        s_gles2.glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// No scaling / projection since we want to fill the whole viewport with
// the texture, hence a trivial vertex shader that only supports translation.
// Note: we used to have a proper free-angle rotation support in this shader,
//  but looks like SwiftShader doesn't support either complicated calculations
//  for gl_Position/varyings or just doesn't like trigonometric functions in
//  shader; anyway the new code has hardcoded texture coordinate mapping for
//  different rotation angles and works in both native OpenGL and SwiftShader.
const char kVertexShaderSource[] =
    "attribute vec4 position;\n"
    "attribute vec2 inCoord;\n"
    "varying vec2 outCoord;\n"
    "uniform vec2 translation;\n"
    "uniform vec2 scale;\n"
    "uniform vec2 coordTranslation;\n"
    "uniform vec2 coordScale;\n"

    "void main(void) {\n"
    "  gl_Position.xy = position.xy * scale.xy - translation.xy;\n"
    "  gl_Position.zw = position.zw;\n"
    "  outCoord = inCoord * coordScale + coordTranslation;\n"
    "}\n";

// Similarly, just interpolate texture coordinates.
const char kFragmentShaderSource[] =
    "#define kComposeModeDevice 2\n"
    "precision mediump float;\n"
    "varying lowp vec2 outCoord;\n"
    "uniform sampler2D tex;\n"
    "uniform float alpha;\n"
    "uniform int composeMode;\n"
    "uniform vec4 color ;\n"

    "void main(void) {\n"
    "  if (composeMode == kComposeModeDevice) {\n"
    "    gl_FragColor = alpha * texture2D(tex, outCoord);\n"
    "  } else {\n"
    "    gl_FragColor = alpha * color;\n"
    "  }\n"
    "}\n";

// Hard-coded arrays of vertex information.
struct Vertex {
    float pos[3];
    float coord[2];
};

const Vertex kVertices[] = {
    // 0 degree
    {{ +1, -1, +0 }, { +1, +0 }},
    {{ +1, +1, +0 }, { +1, +1 }},
    {{ -1, +1, +0 }, { +0, +1 }},
    {{ -1, -1, +0 }, { +0, +0 }},
    // 90 degree clock-wise
    {{ +1, -1, +0 }, { +1, +1 }},
    {{ +1, +1, +0 }, { +0, +1 }},
    {{ -1, +1, +0 }, { +0, +0 }},
    {{ -1, -1, +0 }, { +1, +0 }},
    // 180 degree clock-wise
    {{ +1, -1, +0 }, { +0, +1 }},
    {{ +1, +1, +0 }, { +0, +0 }},
    {{ -1, +1, +0 }, { +1, +0 }},
    {{ -1, -1, +0 }, { +1, +1 }},
    // 270 degree clock-wise
    {{ +1, -1, +0 }, { +0, +0 }},
    {{ +1, +1, +0 }, { +1, +0 }},
    {{ -1, +1, +0 }, { +1, +1 }},
    {{ -1, -1, +0 }, { +0, +1 }},
    // flip horizontally
    {{ +1, -1, +0 }, { +0, +0 }},
    {{ +1, +1, +0 }, { +0, +1 }},
    {{ -1, +1, +0 }, { +1, +1 }},
    {{ -1, -1, +0 }, { +1, +0 }},
    // flip vertically
    {{ +1, -1, +0 }, { +1, +1 }},
    {{ +1, +1, +0 }, { +1, +0 }},
    {{ -1, +1, +0 }, { +0, +0 }},
    {{ -1, -1, +0 }, { +0, +1 }},
    // flip source image horizontally, the rotate 90 degrees clock-wise
    {{ +1, -1, +0 }, { +0, +1 }},
    {{ +1, +1, +0 }, { +1, +1 }},
    {{ -1, +1, +0 }, { +1, +0 }},
    {{ -1, -1, +0 }, { +0, +0 }},
    // flip source image vertically, the rotate 90 degrees clock-wise
    {{ +1, -1, +0 }, { +1, +0 }},
    {{ +1, +1, +0 }, { +0, +0 }},
    {{ -1, +1, +0 }, { +0, +1 }},
    {{ -1, -1, +0 }, { +1, +1 }},
};

// Vertex indices for predefined rotation angles.
const GLubyte kIndices[] = {
    0, 1, 2, 2, 3, 0,      // 0
    4, 5, 6, 6, 7, 4,      // 90
    8, 9, 10, 10, 11, 8,   // 180
    12, 13, 14, 14, 15, 12, // 270
    16, 17, 18 ,18, 19, 16, // flip h
    20, 21, 22, 22, 23, 20, // flip v
    24, 25, 26, 26, 27, 24, // flip h, 90
    28, 29, 30, 30, 31, 28  // flip v, 90
};

const GLint kIndicesLen = sizeof(kIndices) / sizeof(kIndices[0]);
const GLint kIndicesPerDraw = 6;

}  // namespace

TextureDraw::TextureDraw() :
        mVertexShader(0),
        mFragmentShader(0),
        mProgram(0),
        mCoordTranslation(-1),
        mCoordScale(-1),
        mPositionSlot(-1),
        mInCoordSlot(-1),
        mScaleSlot(-1),
        mTextureSlot(-1),
        mTranslationSlot(-1),
        mMaskTexture(0),
        mHaveNewMask(false),
        mMaskIsValid(false) {
    // Create shaders and program.
    mVertexShader = createShader(GL_VERTEX_SHADER, kVertexShaderSource);
    mFragmentShader = createShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);

    mProgram = s_gles2.glCreateProgram();
    s_gles2.glAttachShader(mProgram, mVertexShader);
    s_gles2.glAttachShader(mProgram, mFragmentShader);

    GLint success;
    s_gles2.glLinkProgram(mProgram);
    s_gles2.glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar messages[256];
        s_gles2.glGetProgramInfoLog(
                mProgram, sizeof(messages), 0, &messages[0]);
        ERR("%s: Could not create/link program: %s\n", __FUNCTION__, messages);
        s_gles2.glDeleteProgram(mProgram);
        mProgram = 0;
        return;
    }

    s_gles2.glUseProgram(mProgram);

    // Retrieve attribute/uniform locations.
    mPositionSlot = s_gles2.glGetAttribLocation(mProgram, "position");
    s_gles2.glEnableVertexAttribArray(mPositionSlot);

    mInCoordSlot = s_gles2.glGetAttribLocation(mProgram, "inCoord");
    s_gles2.glEnableVertexAttribArray(mInCoordSlot);

    mAlpha = s_gles2.glGetUniformLocation(mProgram, "alpha");
    mComposeMode = s_gles2.glGetUniformLocation(mProgram, "composeMode");
    mColor = s_gles2.glGetUniformLocation(mProgram, "color");
    mCoordTranslation = s_gles2.glGetUniformLocation(mProgram, "coordTranslation");
    mCoordScale = s_gles2.glGetUniformLocation(mProgram, "coordScale");
    mScaleSlot = s_gles2.glGetUniformLocation(mProgram, "scale");
    mTranslationSlot = s_gles2.glGetUniformLocation(mProgram, "translation");
    mTextureSlot = s_gles2.glGetUniformLocation(mProgram, "tex");

    // set default uniform values
    s_gles2.glUniform1f(mAlpha, 1.0);
    s_gles2.glUniform1i(mComposeMode, 2);
    s_gles2.glUniform2f(mTranslationSlot, 0.0, 0.0);
    s_gles2.glUniform2f(mScaleSlot, 1.0, 1.0);
    s_gles2.glUniform2f(mCoordTranslation, 0.0, 0.0);
    s_gles2.glUniform2f(mCoordScale, 1.0, 1.0);

#if 0
    printf("SLOTS position=%d inCoord=%d texture=%d translation=%d\n",
          mPositionSlot, mInCoordSlot, mTextureSlot, mTranslationSlot);
#endif

    // Create vertex and index buffers.
    s_gles2.glGenBuffers(1, &mVertexBuffer);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    s_gles2.glBufferData(
            GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

    s_gles2.glGenBuffers(1, &mIndexBuffer);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    s_gles2.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         sizeof(kIndices),
                         kIndices,
                         GL_STATIC_DRAW);

    // Reset state.
    s_gles2.glUseProgram(0);
    s_gles2.glDisableVertexAttribArray(mPositionSlot);
    s_gles2.glDisableVertexAttribArray(mInCoordSlot);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, 0);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Create a texture handle for use with an overlay mask
    s_gles2.glGenTextures(1, &mMaskTexture);
}

bool TextureDraw::drawImpl(GLuint texture, float rotation,
                           float dx, float dy, bool wantOverlay) {
    if (!mProgram) {
        ERR("%s: no program\n", __FUNCTION__);
        return false;
    }

    // TODO(digit): Save previous program state.

    s_gles2.glUseProgram(mProgram);

#ifndef NDEBUG
    GLenum err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not use program error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    // Setup the |position| attribute values.
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not bind GL_ARRAY_BUFFER error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    s_gles2.glEnableVertexAttribArray(mPositionSlot);
    s_gles2.glVertexAttribPointer(mPositionSlot,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(Vertex),
                                  0);

#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could glVertexAttribPointer with mPositionSlot error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    // Setup the |inCoord| attribute values.
    s_gles2.glEnableVertexAttribArray(mInCoordSlot);
    s_gles2.glVertexAttribPointer(mInCoordSlot,
                                  2,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(Vertex),
                                  reinterpret_cast<GLvoid*>(
                                        static_cast<uintptr_t>(
                                                sizeof(float) * 3)));

    // setup the |texture| uniform value.
    s_gles2.glActiveTexture(GL_TEXTURE0);
    s_gles2.glBindTexture(GL_TEXTURE_2D, texture);
    s_gles2.glUniform1i(mTextureSlot, 0);

    // setup the |translation| uniform value.
    s_gles2.glUniform2f(mTranslationSlot, dx, dy);

#ifndef NDEBUG
    // Validate program, just to be sure.
    s_gles2.glValidateProgram(mProgram);
    GLint validState = 0;
    s_gles2.glGetProgramiv(mProgram, GL_VALIDATE_STATUS, &validState);
    if (validState == GL_FALSE) {
        GLchar messages[256] = {};
        s_gles2.glGetProgramInfoLog(
                mProgram, sizeof(messages), 0, &messages[0]);
        ERR("%s: Could not run program: '%s'\n", __FUNCTION__, messages);
        return false;
    }
#endif

    // Do the rendering.
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not glBindBuffer(GL_ELEMENT_ARRAY_BUFFER) error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    // We may only get 0, 90, 180, 270 in |rotation| so far.
    const int intRotation = ((int)rotation)/90;
    assert(intRotation >= 0 && intRotation <= 3);
    intptr_t indexShift = 0;
    switch (intRotation) {
    case 0:
        indexShift = 5 * kIndicesPerDraw;
        break;
    case 1:
        indexShift = 7 * kIndicesPerDraw;
        break;
    case 2:
        indexShift = 4 * kIndicesPerDraw;
        break;
    case 3:
        indexShift = 6 * kIndicesPerDraw;
        break;
    }

    s_gles2.glDrawElements(GL_TRIANGLES, kIndicesPerDraw, GL_UNSIGNED_BYTE,
                           (const GLvoid*)indexShift);

    if (wantOverlay && mHaveNewMask) {
        // Create a texture from the mask image and make it
        // available to be blended
        GLint prevUnpackAlignment;
        s_gles2.glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpackAlignment);
        s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        s_gles2.glBindTexture(GL_TEXTURE_2D, mMaskTexture);

        s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mMaskWidth, mMaskHeight, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, mMaskPixels);

        s_gles2.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        s_gles2.glEnable(GL_BLEND);

        s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpackAlignment);

        mHaveNewMask = false;
        mMaskIsValid = true;
    }

    if (wantOverlay && mMaskIsValid) {
        if (mBlendResetNeeded) {
            s_gles2.glEnable(GL_BLEND);
            mBlendResetNeeded = false;
        }
        s_gles2.glBindTexture(GL_TEXTURE_2D, mMaskTexture);
        s_gles2.glDrawElements(GL_TRIANGLES, kIndicesPerDraw, GL_UNSIGNED_BYTE,
                               (const GLvoid*)indexShift);
        // Reset to the "normal" texture
        s_gles2.glBindTexture(GL_TEXTURE_2D, texture);
    }

#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not glDrawElements() error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    // TODO(digit): Restore previous program state.
    // For now, reset back to zero and assume other users will
    // follow the same protocol.
    s_gles2.glUseProgram(0);
    s_gles2.glDisableVertexAttribArray(mPositionSlot);
    s_gles2.glDisableVertexAttribArray(mInCoordSlot);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, 0);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return true;
}

TextureDraw::~TextureDraw() {
    s_gles2.glDeleteBuffers(1, &mIndexBuffer);
    s_gles2.glDeleteBuffers(1, &mVertexBuffer);

    if (mFragmentShader) {
        s_gles2.glDeleteShader(mFragmentShader);
    }
    if (mVertexShader) {
        s_gles2.glDeleteShader(mVertexShader);
    }
    if (mMaskTexture) {
        s_gles2.glDeleteTextures(1, &mMaskTexture);
    }
}

void TextureDraw::setScreenMask(int width, int height, const unsigned char* rgbaData) {
    if (width <= 0 || height <= 0 || rgbaData == nullptr) {
        mMaskIsValid = false;
        return;
    }
    // Save the data for use in the right context
    mMaskPixels = rgbaData;
    mMaskWidth = width;
    mMaskHeight = height;

    mHaveNewMask = true;
}

void TextureDraw::prepareForDrawLayer() {
    if (!mProgram) {
        ERR("%s: no program\n", __FUNCTION__);
        return;
    }
    s_gles2.glUseProgram(mProgram);
#ifndef NDEBUG
    GLenum err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not use program error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not bind GL_ARRAY_BUFFER error=0x%x\n",
            __FUNCTION__, err);
    }
#endif
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not glBindBuffer(GL_ELEMENT_ARRAY_BUFFER) error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    s_gles2.glEnableVertexAttribArray(mPositionSlot);
    s_gles2.glVertexAttribPointer(mPositionSlot,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(Vertex),
                                  0);

    s_gles2.glEnableVertexAttribArray(mInCoordSlot);
    s_gles2.glVertexAttribPointer(mInCoordSlot,
                                  2,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(Vertex),
                                  reinterpret_cast<GLvoid*>(
                                        static_cast<uintptr_t>(
                                                sizeof(float) * 3)));
#ifndef NDEBUG
    err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could glVertexAttribPointer with mPositionSlot error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

   // set composition default
    s_gles2.glUniform1i(mComposeMode, 2);
    s_gles2.glActiveTexture(GL_TEXTURE0);
    s_gles2.glUniform1i(mTextureSlot, 0);
    s_gles2.glEnable(GL_BLEND);
    s_gles2.glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // clear color
    s_gles2.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void TextureDraw::drawLayer(ComposeLayer* l, int frameWidth, int frameHeight,
                            int cbWidth, int cbHeight, GLuint texture) {
    switch(l->composeMode) {
        case HWC2_COMPOSITION_DEVICE:
            s_gles2.glBindTexture(GL_TEXTURE_2D, texture);
            break;
        case HWC2_COMPOSITION_SOLID_COLOR: {
            s_gles2.glUniform1i(mComposeMode, l->composeMode);
            s_gles2.glUniform4f(mColor,
                                l->color.r/255.0, l->color.g/255.0,
                                l->color.b/255.0, l->color.a/255.0);
            break;
        }
        case HWC2_COMPOSITION_CLIENT:
        case HWC2_COMPOSITION_CURSOR:
        case HWC2_COMPOSITION_SIDEBAND:
        case HWC2_COMPOSITION_INVALID:
        default:
            ERR("%s: invalid composition mode %d", __FUNCTION__, l->composeMode);
            return;
    }

    switch(l->blendMode) {
        case HWC2_BLEND_MODE_NONE:
            s_gles2.glDisable(GL_BLEND);
            mBlendResetNeeded = true;
            break;
        case HWC2_BLEND_MODE_PREMULTIPLIED:
            break;
        case HWC2_BLEND_MODE_INVALID:
        case HWC2_BLEND_MODE_COVERAGE:
        default:
            ERR("%s: invalid blendMode %d", __FUNCTION__, l->blendMode);
            return;
    }

    s_gles2.glUniform1f(mAlpha, l->alpha);

    float edges[4];
    edges[0] = 1 - 2.0 * (frameWidth - l->displayFrame.left)/frameWidth;
    edges[1] = 1 - 2.0 * (frameHeight - l->displayFrame.top)/frameHeight;
    edges[2] = 1 - 2.0 * (frameWidth - l->displayFrame.right)/frameWidth;
    edges[3] = 1- 2.0 * (frameHeight - l->displayFrame.bottom)/frameHeight;

    float crop[4];
    crop[0] = l->crop.left/cbWidth;
    crop[1] = l->crop.top/cbHeight;
    crop[2] = l->crop.right/cbWidth;
    crop[3] = l->crop.bottom/cbHeight;

    // setup the |translation| uniform value.
    s_gles2.glUniform2f(mTranslationSlot, (-edges[2] - edges[0])/2,
                        (-edges[3] - edges[1])/2);
    s_gles2.glUniform2f(mScaleSlot, (edges[2] - edges[0])/2,
                        (edges[1] - edges[3])/2);
    s_gles2.glUniform2f(mCoordTranslation, crop[0], crop[3]);
    s_gles2.glUniform2f(mCoordScale, crop[2] - crop[0], crop[1] - crop[3]);

    intptr_t indexShift;
    switch(l->transform) {
    case HWC_TRANSFORM_ROT_90:
        indexShift = 1 * kIndicesPerDraw;
        break;
    case HWC_TRANSFORM_ROT_180:
        indexShift = 2 * kIndicesPerDraw;
        break;
    case HWC_TRANSFORM_ROT_270:
        indexShift = 3 * kIndicesPerDraw;
        break;
    case HWC_TRANSFORM_FLIP_H:
        indexShift = 4 * kIndicesPerDraw;
        break;
    case HWC_TRANSFORM_FLIP_V:
        indexShift = 5 * kIndicesPerDraw;
        break;
    case HWC_TRANSFORM_FLIP_H_ROT_90:
        indexShift = 6 * kIndicesPerDraw;
        break;
    case HWC_TRANSFORM_FLIP_V_ROT_90:
        indexShift = 7 * kIndicesPerDraw;
        break;
    default:
        indexShift = 0;
    }
    s_gles2.glDrawElements(GL_TRIANGLES, kIndicesPerDraw, GL_UNSIGNED_BYTE,
                           (const GLvoid*)indexShift);
#ifndef NDEBUG
    GLenum err = s_gles2.glGetError();
    if (err != GL_NO_ERROR) {
        ERR("%s: Could not glDrawElements() error=0x%x\n",
            __FUNCTION__, err);
    }
#endif

    // restore the default value for the next draw layer
    if (l->composeMode != HWC2_COMPOSITION_DEVICE) {
        s_gles2.glUniform1i(mComposeMode, HWC2_COMPOSITION_DEVICE);
    }
    if (l->blendMode != HWC2_BLEND_MODE_PREMULTIPLIED) {
        s_gles2.glEnable(GL_BLEND);
        mBlendResetNeeded = false;
        s_gles2.glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
}

// Do Post right after drawing each layer, so keep using this program
void TextureDraw::cleanupForDrawLayer() {
    s_gles2.glUniform1f(mAlpha, 1.0);
    s_gles2.glUniform1i(mComposeMode, HWC2_COMPOSITION_DEVICE);
    s_gles2.glUniform2f(mTranslationSlot, 0.0, 0.0);
    s_gles2.glUniform2f(mScaleSlot, 1.0, 1.0);
    s_gles2.glUniform2f(mCoordTranslation, 0.0, 0.0);
    s_gles2.glUniform2f(mCoordScale, 1.0, 1.0);
}
