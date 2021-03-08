/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "YUVConverter.h"

#include "DispatchTables.h"
#include "emugl/common/feature_control.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define FATAL(fmt,...) do { \
    fprintf(stderr, "%s: FATAL: " fmt "\n", __func__, ##__VA_ARGS__); \
    assert(false); \
} while(0)

#define YUV_CONVERTER_DEBUG 0

#if YUV_CONVERTER_DEBUG
#define DDD(fmt, ...)                                                     \
    fprintf(stderr, "yuv-converter: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define DDD(fmt, ...)
#endif

enum YUVInterleaveDirection {
    YUVInterleaveDirectionVU = 0,
    YUVInterleaveDirectionUV = 1,
};

// getYUVOffsets(), given a YUV-formatted buffer that is arranged
// according to the spec
// https://developer.android.com/reference/android/graphics/ImageFormat.html#YUV
// In particular, Android YUV widths are aligned to 16 pixels.
// Inputs:
// |yv12|: the YUV-formatted buffer
// Outputs:
// |yoff|: offset into |yv12| of the start of the Y component
// |uoff|: offset into |yv12| of the start of the U component
// |voff|: offset into |yv12| of the start of the V component
static void getYUVOffsets(int width, int height, FrameworkFormat format,
                          uint32_t* yoff, uint32_t* uoff, uint32_t* voff,
                          uint32_t* alignwidth, uint32_t* alignwidthc) {
    uint32_t totalSize, yStride, cStride, cHeight, cSize, align;
    switch (format) {
    case FRAMEWORK_FORMAT_YV12:
        align = 16;
        yStride = (width + (align - 1)) & ~(align - 1);
        cStride = (yStride / 2 + (align - 1)) & ~(align - 1);
        cHeight = height / 2;
        cSize = cStride * cHeight;
        *yoff = 0;
        *voff = yStride * height;
        *uoff = (*voff) + cSize;
        *alignwidth = yStride;
        *alignwidthc = cStride;
        break;
    case FRAMEWORK_FORMAT_YUV_420_888:
        if (emugl::emugl_feature_is_enabled(
            android::featurecontrol::YUV420888toNV21)) {
            align = 1;
            yStride = (width + (align - 1)) & ~(align - 1);
            cStride = yStride;
            cHeight = height / 2;
            *yoff = 0;
            *voff = yStride * height;
            *uoff = (*voff) + 1;
            *alignwidth = yStride;
            *alignwidthc = cStride / 2;
        } else {
            align = 1;
            yStride = (width + (align - 1)) & ~(align - 1);
            cStride = (yStride / 2 + (align - 1)) & ~(align - 1);
            cHeight = height / 2;
            cSize = cStride * cHeight;
            *yoff = 0;
            *uoff = yStride * height;
            *voff = (*uoff) + cSize;
            *alignwidth = yStride;
            *alignwidthc = cStride;
        }
        break;
    case FRAMEWORK_FORMAT_NV12:
        align = 1;
        yStride = width;
        cStride = yStride;
        cHeight = height / 2;
        cSize = cStride * cHeight;
        *yoff = 0;
        *uoff = yStride * height;
        *voff = (*uoff) + 1;
        *alignwidth = yStride;
        *alignwidthc = cStride / 2;
        break;
    case FRAMEWORK_FORMAT_GL_COMPATIBLE:
        FATAL("Input not a YUV format! (FRAMEWORK_FORMAT_GL_COMPATIBLE)");
    default:
        FATAL("Unknown format: 0x%x", format);
    }
}

// createYUVGLTex() allocates GPU memory that is enough
// to hold the raw data of the YV12 buffer.
// The memory is in the form of an OpenGL texture
// with one component (GL_LUMINANCE) and
// of type GL_UNSIGNED_BYTE.
// In order to process all Y, U, V components
// simultaneously in conversion, the simple thing to do
// is to use multiple texture units, hence
// the |texture_unit| argument.
// Returns a new OpenGL texture object in |texName_out|
// that is to be cleaned up by the caller.
void YUVConverter::createYUVGLTex(GLenum texture_unit,
                                  GLsizei width,
                                  GLsizei height,
                                  GLuint* texName_out,
                                  bool uvInterleaved) {
    assert(texName_out);

    s_gles2.glActiveTexture(texture_unit);
    s_gles2.glGenTextures(1, texName_out);
    s_gles2.glBindTexture(GL_TEXTURE_2D, *texName_out);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_gles2.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GLint unprevAlignment = 0;
    s_gles2.glGetIntegerv(GL_UNPACK_ALIGNMENT, &unprevAlignment);
    s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (uvInterleaved) {
        s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                             width, height, 0,
                             GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                             NULL);
    } else {
        s_gles2.glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                             width, height, 0,
                             GL_LUMINANCE, GL_UNSIGNED_BYTE,
                             NULL);
    }
    s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, unprevAlignment);
    s_gles2.glActiveTexture(GL_TEXTURE0);
}

static void readYUVTex(GLuint tex, void* pixels, bool uvInterleaved) {
    GLuint prevTexture = 0;
    s_gles2.glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&prevTexture);
    s_gles2.glBindTexture(GL_TEXTURE_2D, tex);
    GLint prevAlignment = 0;
    s_gles2.glGetIntegerv(GL_PACK_ALIGNMENT, &prevAlignment);
    s_gles2.glPixelStorei(GL_PACK_ALIGNMENT, 1);
    if (uvInterleaved) {
        if (s_gles2.glGetTexImage) {
            s_gles2.glGetTexImage(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                                  GL_UNSIGNED_BYTE, pixels);
        } else {
            DDD("empty glGetTexImage");
        }
    } else {
        if (s_gles2.glGetTexImage) {
            s_gles2.glGetTexImage(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                                  GL_UNSIGNED_BYTE, pixels);
        } else {
            DDD("empty glGetTexImage");
        }
    }
    s_gles2.glPixelStorei(GL_PACK_ALIGNMENT, prevAlignment);
    s_gles2.glBindTexture(GL_TEXTURE_2D, prevTexture);
}

// subUpdateYUVGLTex() updates a given YUV texture
// at the coordinates (x, y, width, height),
// with the raw YUV data in |pixels|.
// We cannot view the result properly until
// after conversion; this is to be used only
// as input to the conversion shader.
static void subUpdateYUVGLTex(GLenum texture_unit,
                              GLuint tex,
                              int x, int y, int width, int height,
                              void* pixels, bool uvInterleaved) {
    s_gles2.glActiveTexture(texture_unit);
    s_gles2.glBindTexture(GL_TEXTURE_2D, tex);
    GLint unprevAlignment = 0;
    s_gles2.glGetIntegerv(GL_UNPACK_ALIGNMENT, &unprevAlignment);
    s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (uvInterleaved) {
        s_gles2.glTexSubImage2D(GL_TEXTURE_2D, 0,
                                x, y, width, height,
                                GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                                pixels);
    } else {
        s_gles2.glTexSubImage2D(GL_TEXTURE_2D, 0,
                                x, y, width, height,
                                GL_LUMINANCE, GL_UNSIGNED_BYTE,
                                pixels);
    }
    s_gles2.glPixelStorei(GL_UNPACK_ALIGNMENT, unprevAlignment);

    s_gles2.glActiveTexture(GL_TEXTURE0);
}

// createYUVGLShader() defines the vertex/fragment
// shader that does the actual work of converting
// YUV to RGB. The resulting program is stored in |program_out|.
static void createYUVGLShader(GLuint* program_out,
                              GLint* ywidthcutoffloc_out,
                              GLint* cwidthcutoffloc_out,
                              GLint* ysamplerloc_out,
                              GLint* usamplerloc_out,
                              GLint* vsamplerloc_out,
                              GLint* incoordloc_out,
                              GLint* posloc_out) {
    assert(program_out);

    static const char kVShader[] = R"(
precision highp float;
attribute mediump vec4 position;
attribute highp vec2 inCoord;
varying highp vec2 outCoord;
void main(void) {
  gl_Position = position;
  outCoord = inCoord;
}
    )";
    const GLchar* const kVShaders =
        static_cast<const GLchar*>(kVShader);

    // Based on:
    // http://stackoverflow.com/questions/11093061/yv12-to-rgb-using-glsl-in-ios-result-image-attached
    // + account for 16-pixel alignment using |yWidthCutoff| / |cWidthCutoff|
    // + use conversion matrix in
    // frameworks/av/media/libstagefright/colorconversion/ColorConverter.cpp (YUV420p)
    // + more precision from
    // https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.601_conversion
    static const char kFShader[] = R"(
precision highp float;
varying highp vec2 outCoord;
uniform highp float yWidthCutoff;
uniform highp float cWidthCutoff;
uniform sampler2D ysampler;
uniform sampler2D usampler;
uniform sampler2D vsampler;
void main(void) {
    highp vec2 cutoffCoordsY;
    highp vec2 cutoffCoordsC;
    highp vec3 yuv;
    highp vec3 rgb;
    cutoffCoordsY.x = outCoord.x * yWidthCutoff;
    cutoffCoordsY.y = outCoord.y;
    cutoffCoordsC.x = outCoord.x * cWidthCutoff;
    cutoffCoordsC.y = outCoord.y;
    yuv[0] = texture2D(ysampler, cutoffCoordsY).r - 0.0625;
    yuv[1] = 0.96*(texture2D(usampler, cutoffCoordsC).r - 0.5);
    yuv[2] = texture2D(vsampler, cutoffCoordsC).r - 0.5;
    highp float yscale = 1.1643835616438356;
    rgb = mat3(yscale,                           yscale,            yscale,
               0,                  -0.39176229009491365, 2.017232142857143,
               1.5960267857142856, -0.8129676472377708,                  0) * yuv;
    gl_FragColor = vec4(rgb, 1);
}
    )";

    const GLchar* const kFShaders =
        static_cast<const GLchar*>(kFShader);

    GLuint vshader = s_gles2.glCreateShader(GL_VERTEX_SHADER);
    GLuint fshader = s_gles2.glCreateShader(GL_FRAGMENT_SHADER);

    const GLint vtextLen = strlen(kVShader);
    const GLint ftextLen = strlen(kFShaders);
    s_gles2.glShaderSource(vshader, 1, &kVShaders, &vtextLen);
    s_gles2.glShaderSource(fshader, 1, &kFShaders, &ftextLen);
    s_gles2.glCompileShader(vshader);
    s_gles2.glCompileShader(fshader);

    *program_out = s_gles2.glCreateProgram();
    s_gles2.glAttachShader(*program_out, vshader);
    s_gles2.glAttachShader(*program_out, fshader);
    s_gles2.glLinkProgram(*program_out);

    *ywidthcutoffloc_out = s_gles2.glGetUniformLocation(*program_out, "yWidthCutoff");
    *cwidthcutoffloc_out = s_gles2.glGetUniformLocation(*program_out, "cWidthCutoff");
    *ysamplerloc_out = s_gles2.glGetUniformLocation(*program_out, "ysampler");
    *usamplerloc_out = s_gles2.glGetUniformLocation(*program_out, "usampler");
    *vsamplerloc_out = s_gles2.glGetUniformLocation(*program_out, "vsampler");
    *posloc_out = s_gles2.glGetAttribLocation(*program_out, "position");
    *incoordloc_out = s_gles2.glGetAttribLocation(*program_out, "inCoord");

    s_gles2.glDeleteShader(vshader);
    s_gles2.glDeleteShader(fshader);
}

static void createYUVInterleavedGLShader(GLuint* program_out,
                                         GLint* ywidthcutoffloc_out,
                                         GLint* cwidthcutoffloc_out,
                                         GLint* ysamplerloc_out,
                                         GLint* vusamplerloc_out,
                                         GLint* incoordloc_out,
                                         GLint* posloc_out,
                                         YUVInterleaveDirection interleaveDir) {
    assert(program_out);

    static const char kVShader[] = R"(
precision highp float;
attribute mediump vec4 position;
attribute highp vec2 inCoord;
varying highp vec2 outCoord;
void main(void) {
  gl_Position = position;
  outCoord = inCoord;
}
    )";
    const GLchar* const kVShaders =
        static_cast<const GLchar*>(kVShader);

    // Based on:
    // https://stackoverflow.com/questions/22456884/how-to-render-androids-yuv-nv21-camera-image-on-the-background-in-libgdx-with-o
    // + account for 16-pixel alignment using |yWidthCutoff| / |cWidthCutoff|
    // + use conversion matrix in
    // frameworks/av/media/libstagefright/colorconversion/ColorConverter.cpp (YUV420p)
    // + more precision from
    // https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.601_conversion
    // UV texture is (width/2*height/2) in size (downsampled by 2 in
    // both dimensions, each pixel corresponds to 4 pixels of the Y channel)
    // and each pixel is two bytes. By setting GL_LUMINANCE_ALPHA, OpenGL
    // puts first byte (V) into R,G and B components and of the texture
    // and the second byte (U) into the A component of the texture. That's
    // why we find U and V at A and R respectively in the fragment shader code.
    // Note that we could have also found V at G or B as well.
    static const char kFShaderVu[] = R"(
precision highp float;
varying highp vec2 outCoord;
uniform highp float yWidthCutoff;
uniform highp float cWidthCutoff;
uniform sampler2D ysampler;
uniform sampler2D vusampler;
void main(void) {
    highp vec2 cutoffCoordsY;
    highp vec2 cutoffCoordsC;
    highp vec3 yuv;
    highp vec3 rgb;
    cutoffCoordsY.x = outCoord.x * yWidthCutoff;
    cutoffCoordsY.y = outCoord.y;
    cutoffCoordsC.x = outCoord.x * cWidthCutoff;
    cutoffCoordsC.y = outCoord.y;
    yuv[0] = texture2D(ysampler, cutoffCoordsY).r - 0.0625;
    yuv[1] = 0.96 * (texture2D(vusampler, cutoffCoordsC).a - 0.5);
    yuv[2] = texture2D(vusampler, cutoffCoordsC).r - 0.5;
    highp float yscale = 1.1643835616438356;
    rgb = mat3(yscale,                           yscale,            yscale,
               0,                  -0.39176229009491365, 2.017232142857143,
               1.5960267857142856, -0.8129676472377708,                  0) * yuv;
    gl_FragColor = vec4(rgb, 1);
}
    )";

    static const char kFShaderUv[] = R"(
precision highp float;
varying highp vec2 outCoord;
uniform highp float yWidthCutoff;
uniform highp float cWidthCutoff;
uniform sampler2D ysampler;
uniform sampler2D uvsampler;
void main(void) {
    highp vec2 cutoffCoordsY;
    highp vec2 cutoffCoordsC;
    highp vec3 yuv;
    highp vec3 rgb;
    cutoffCoordsY.x = outCoord.x * yWidthCutoff;
    cutoffCoordsY.y = outCoord.y;
    cutoffCoordsC.x = outCoord.x * cWidthCutoff;
    cutoffCoordsC.y = outCoord.y;
    yuv[0] = texture2D(ysampler, cutoffCoordsY).r - 0.0625;
    yuv[1] = 0.96 * (texture2D(uvsampler, cutoffCoordsC).r - 0.5);
    yuv[2] = (texture2D(uvsampler, cutoffCoordsC).a - 0.5);
    highp float yscale = 1.1643835616438356;
    rgb = mat3(yscale,                           yscale,            yscale,
               0,                  -0.39176229009491365, 2.017232142857143,
               1.5960267857142856, -0.8129676472377708,                  0) * yuv;
    gl_FragColor = vec4(rgb, 1);
}
    )";

    const GLchar* const kFShaders =
        interleaveDir == YUVInterleaveDirectionVU ? kFShaderVu : kFShaderUv;

    GLuint vshader = s_gles2.glCreateShader(GL_VERTEX_SHADER);
    GLuint fshader = s_gles2.glCreateShader(GL_FRAGMENT_SHADER);

    const GLint vtextLen = strlen(kVShader);
    const GLint ftextLen = strlen(kFShaders);
    s_gles2.glShaderSource(vshader, 1, &kVShaders, &vtextLen);
    s_gles2.glShaderSource(fshader, 1, &kFShaders, &ftextLen);
    s_gles2.glCompileShader(vshader);
    s_gles2.glCompileShader(fshader);

    *program_out = s_gles2.glCreateProgram();
    s_gles2.glAttachShader(*program_out, vshader);
    s_gles2.glAttachShader(*program_out, fshader);
    s_gles2.glLinkProgram(*program_out);

    *ywidthcutoffloc_out = s_gles2.glGetUniformLocation(*program_out, "yWidthCutoff");
    *cwidthcutoffloc_out = s_gles2.glGetUniformLocation(*program_out, "cWidthCutoff");
    *ysamplerloc_out = s_gles2.glGetUniformLocation(*program_out, "ysampler");
    *vusamplerloc_out = s_gles2.glGetUniformLocation(
            *program_out, YUVInterleaveDirectionVU ? "vusampler" : "uvsampler");
    *posloc_out = s_gles2.glGetAttribLocation(*program_out, "position");
    *incoordloc_out = s_gles2.glGetAttribLocation(*program_out, "inCoord");

    s_gles2.glDeleteShader(vshader);
    s_gles2.glDeleteShader(fshader);
}
// When converting YUV to RGB with shaders,
// we are using the OpenGL graphics pipeline to do compute,
// so we need to express the place to store the result
// with triangles and draw calls.
// createYUVGLFullscreenQuad() defines a fullscreen quad
// with position and texture coordinates.
// The quad will be textured with the resulting RGB colors,
// and we will read back the pixels from the framebuffer
// to retrieve our RGB result.
static void createYUVGLFullscreenQuad(GLuint* vbuf_out,
                                      GLuint* ibuf_out,
                                      int picture_width,
                                      int aligned_width) {
    assert(vbuf_out);
    assert(ibuf_out);

    s_gles2.glGenBuffers(1, vbuf_out);
    s_gles2.glGenBuffers(1, ibuf_out);

    static const float kVertices[] = {
        +1, -1, +0, +1, +0,
        +1, +1, +0, +1, +1,
        -1, +1, +0, +0, +1,
        -1, -1, +0, +0, +0,
    };

    static const GLubyte kIndices[] = { 0, 1, 2, 2, 3, 0 };

    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, *vbuf_out);
    s_gles2.glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices,
                         GL_STATIC_DRAW);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibuf_out);
    s_gles2.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices,
                         GL_STATIC_DRAW);
}

// doYUVConversionDraw() does the actual work of setting up
// and submitting draw commands to the GPU.
// It uses the textures, shaders, and fullscreen quad defined above
// and executes the pipeline on them.
// Note, however, that it is up to the caller to dig out
// the result of the draw.
static void doYUVConversionDraw(GLuint program,
                                GLint yWidthCutoffLoc,
                                GLint cWidthCutoffLoc,
                                GLint ySamplerLoc,
                                GLint uSamplerLoc,
                                GLint vSamplerLoc,
                                GLint vuSamplerLoc,
                                GLint inCoordLoc,
                                GLint posLoc,
                                GLuint vbuf, GLuint ibuf,
                                int width, int ywidth,
                                int halfwidth, int cwidth,
                                float yWidthCutoff,
                                float cWidthCutoff,
                                bool uvInterleaved) {

    const GLsizei kVertexAttribStride = 5 * sizeof(GL_FLOAT);
    const GLvoid* kVertexAttribPosOffset = (GLvoid*)0;
    const GLvoid* kVertexAttribCoordOffset = (GLvoid*)(3 * sizeof(GL_FLOAT));

    s_gles2.glUseProgram(program);

    s_gles2.glUniform1f(yWidthCutoffLoc, yWidthCutoff);
    s_gles2.glUniform1f(cWidthCutoffLoc, cWidthCutoff);

    s_gles2.glUniform1i(ySamplerLoc, 0);
    if (uvInterleaved) {
        s_gles2.glUniform1i(vuSamplerLoc, 1);
    } else {
        s_gles2.glUniform1i(uSamplerLoc, 1);
        s_gles2.glUniform1i(vSamplerLoc, 2);
    }

    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    s_gles2.glEnableVertexAttribArray(posLoc);
    s_gles2.glEnableVertexAttribArray(inCoordLoc);

    s_gles2.glVertexAttribPointer(posLoc, 3, GL_FLOAT, false,
                                  kVertexAttribStride,
                                  kVertexAttribPosOffset);
    s_gles2.glVertexAttribPointer(inCoordLoc, 2, GL_FLOAT, false,
                                  kVertexAttribStride,
                                  kVertexAttribCoordOffset);

    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
    s_gles2.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    s_gles2.glDisableVertexAttribArray(posLoc);
    s_gles2.glDisableVertexAttribArray(inCoordLoc);
}

// initialize(): allocate GPU memory for YUV components,
// and create shaders and vertex data.
YUVConverter::YUVConverter(int width, int height, FrameworkFormat format)
    : mWidth(width),
      mHeight(height),
      mFormat(format),
      mCbWidth(width),
      mCbHeight(height),
      mCbFormat(format) {}

void YUVConverter::init(int width, int height, FrameworkFormat format) {
    uint32_t yoff, uoff, voff, ywidth, cwidth, cheight;
    getYUVOffsets(width, height, mFormat,
                  &yoff, &uoff, &voff,
                  &ywidth, &cwidth);
    cheight = height / 2;

    mWidth = width;
    mHeight = height;
    if (!mYtex)
        createYUVGLTex(GL_TEXTURE0, ywidth, height, &mYtex, false);
    switch (mFormat) {
        case FRAMEWORK_FORMAT_YV12:
            if (!mUtex)
                createYUVGLTex(GL_TEXTURE1, cwidth, cheight, &mUtex, false);
            if (!mVtex)
                createYUVGLTex(GL_TEXTURE2, cwidth, cheight, &mVtex, false);
            createYUVGLShader(&mProgram,
                              &mYWidthCutoffLoc,
                              &mCWidthCutoffLoc,
                              &mYSamplerLoc,
                              &mUSamplerLoc,
                              &mVSamplerLoc,
                              &mInCoordLoc,
                              &mPosLoc);
            break;
        case FRAMEWORK_FORMAT_YUV_420_888:
            if (emugl::emugl_feature_is_enabled(
                    android::featurecontrol::YUV420888toNV21)) {
                if (!mVUtex)
                    createYUVGLTex(GL_TEXTURE1, cwidth, cheight, &mVUtex, true);
                createYUVInterleavedGLShader(&mProgram,
                                             &mYWidthCutoffLoc,
                                             &mCWidthCutoffLoc,
                                             &mYSamplerLoc,
                                             &mVUSamplerLoc,
                                             &mInCoordLoc,
                                             &mPosLoc,
                                             YUVInterleaveDirectionVU);
            } else {
                if (!mUtex)
                    createYUVGLTex(GL_TEXTURE1, cwidth, cheight, &mUtex, false);
                if (!mVtex)
                    createYUVGLTex(GL_TEXTURE2, cwidth, cheight, &mVtex, false);
                createYUVGLShader(&mProgram,
                                  &mYWidthCutoffLoc,
                                  &mCWidthCutoffLoc,
                                  &mYSamplerLoc,
                                  &mUSamplerLoc,
                                  &mVSamplerLoc,
                                  &mInCoordLoc,
                                  &mPosLoc);
            }
            break;
        case FRAMEWORK_FORMAT_NV12:
            if (!mUVtex)
                createYUVGLTex(GL_TEXTURE1, cwidth, cheight, &mUVtex, true);
            createYUVInterleavedGLShader(&mProgram,
                                         &mYWidthCutoffLoc,
                                         &mCWidthCutoffLoc,
                                         &mYSamplerLoc,
                                         &mVUSamplerLoc,
                                         &mInCoordLoc,
                                         &mPosLoc,
                                         YUVInterleaveDirectionUV);
            break;
        default:
            FATAL("Unknown format: 0x%x", mFormat);
    }

    createYUVGLFullscreenQuad(&mVbuf, &mIbuf, width, ywidth);
}

void YUVConverter::saveGLState() {
    s_gles2.glGetFloatv(GL_VIEWPORT, mCurrViewport);
    s_gles2.glGetIntegerv(GL_ACTIVE_TEXTURE, &mCurrTexUnit);
    s_gles2.glGetIntegerv(GL_TEXTURE_BINDING_2D, &mCurrTexBind);
    s_gles2.glGetIntegerv(GL_CURRENT_PROGRAM, &mCurrProgram);
    s_gles2.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &mCurrVbo);
    s_gles2.glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &mCurrIbo);
}

void YUVConverter::restoreGLState() {
    s_gles2.glViewport(mCurrViewport[0], mCurrViewport[1],
                       mCurrViewport[2], mCurrViewport[3]);
    s_gles2.glActiveTexture(mCurrTexUnit);
    s_gles2.glUseProgram(mCurrProgram);
    s_gles2.glBindBuffer(GL_ARRAY_BUFFER, mCurrVbo);
    s_gles2.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mCurrIbo);
}

uint32_t YUVConverter::getDataSize() {
    uint32_t align = (mFormat == FRAMEWORK_FORMAT_YV12) ? 16 : 1;
    uint32_t yStride = (mWidth + (align - 1)) & ~(align - 1);
    uint32_t uvStride = (yStride / 2 + (align - 1)) & ~(align - 1);
    uint32_t uvHeight = mHeight / 2;
    uint32_t dataSize = yStride * mHeight + 2 * (uvHeight * uvStride);
    return dataSize;
}

void YUVConverter::readPixels(uint8_t* pixels, uint32_t pixels_size) {
    int width = mWidth;
    int height = mHeight;

    DDD("calling %s %d\n texture: width %d height %d\n", __func__, __LINE__,
        width, height);
    {
        uint32_t align = (mFormat == FRAMEWORK_FORMAT_YV12) ? 16 : 1;
        uint32_t yStride = (width + (align - 1)) & ~(align - 1);
        uint32_t uvStride = (yStride / 2 + (align - 1)) & ~(align - 1);
        uint32_t uvHeight = height / 2;
        uint32_t dataSize = yStride * height + 2 * (uvHeight * uvStride);
        if (pixels_size != dataSize) {
            DDD("failed %s %d\n", __func__, __LINE__);
            return;
        }
        DDD("%s %d reading %d bytes\n", __func__, __LINE__, (int)dataSize);
    }

    uint32_t yoff, uoff, voff, ywidth, cwidth;
    getYUVOffsets(width, height, mFormat, &yoff, &uoff, &voff, &ywidth,
                  &cwidth);

    if (mFormat == FRAMEWORK_FORMAT_YUV_420_888) {
        if (emugl::emugl_feature_is_enabled(
                    android::featurecontrol::YUV420888toNV21)) {
            readYUVTex(mVUtex, pixels + voff, true);
            DDD("done");
        } else {
            readYUVTex(mUtex, pixels + uoff, false);
            readYUVTex(mVtex, pixels + voff, false);
            DDD("done");
        }
    } else if (mFormat == FRAMEWORK_FORMAT_NV12) {
        readYUVTex(mUVtex, pixels + uoff, true);
        if (mCbFormat == FRAMEWORK_FORMAT_YUV_420_888) {
            // do a conversion here inplace: NV12 to YUV 420 888
            uint8_t* scrath_memory = pixels;
            NV12ToYUV420PlanarInPlaceConvert(width, height, pixels,
                                             scrath_memory);
            DDD("done");
        }
        DDD("done");
    } else if (mFormat == FRAMEWORK_FORMAT_YV12) {
            readYUVTex(mUtex, pixels + uoff, false);
            readYUVTex(mVtex, pixels + voff, false);
            DDD("done");
    }
    // read Y the last, because we can might used it as a scratch space
    readYUVTex(mYtex, pixels + yoff, false);
    DDD("done");
}

void YUVConverter::swapTextures(uint32_t type, uint32_t* textures) {
    if (type == FRAMEWORK_FORMAT_NV12) {
        mFormat = FRAMEWORK_FORMAT_NV12;
        std::swap(textures[0], mYtex);
        std::swap(textures[1], mUVtex);
    } else if (type == FRAMEWORK_FORMAT_YUV_420_888) {
        mFormat = FRAMEWORK_FORMAT_YUV_420_888;
        std::swap(textures[0], mYtex);
        std::swap(textures[1], mUtex);
        std::swap(textures[2], mVtex);
    } else {
        FATAL("Unknown format: 0x%x", type);
    }
}

// drawConvert: per-frame updates.
// Update YUV textures, then draw the fullscreen
// quad set up above, which results in a framebuffer
// with the RGB colors.
void YUVConverter::drawConvert(int x, int y,
                               int width, int height,
                               char* pixels) {
    saveGLState();
    if (pixels && (width != mWidth || height != mHeight)) {
        reset();
    }

    if (mProgram == 0) {
        init(width, height, mFormat);
    }
    s_gles2.glViewport(x, y, width, height);
    uint32_t yoff, uoff, voff, ywidth, cwidth, cheight;
    getYUVOffsets(width, height, mFormat, &yoff, &uoff, &voff, &ywidth,
                  &cwidth);
    cheight = height / 2;
    updateCutoffs(width, ywidth, width / 2, cwidth);

    if (!pixels) {
        // special case: draw from texture, only support NV12 for now
        // as cuvid's native format is NV12.
        // TODO: add more formats if there are such needs in the future.
        assert(mFormat == FRAMEWORK_FORMAT_NV12);
        s_gles2.glActiveTexture(GL_TEXTURE1);
        s_gles2.glBindTexture(GL_TEXTURE_2D, mUVtex);
        s_gles2.glActiveTexture(GL_TEXTURE0);
        s_gles2.glBindTexture(GL_TEXTURE_2D, mYtex);

        doYUVConversionDraw(mProgram, mYWidthCutoffLoc, mCWidthCutoffLoc,
                            mYSamplerLoc, mUSamplerLoc, mVSamplerLoc,
                            mVUSamplerLoc, mInCoordLoc, mPosLoc, mVbuf, mIbuf,
                            width, ywidth, width / 2, cwidth, mYWidthCutoff,
                            mCWidthCutoff, true);

        restoreGLState();
        return;
    }

    subUpdateYUVGLTex(GL_TEXTURE0, mYtex,
                      x, y, ywidth, height,
                      pixels + yoff, false);

    switch (mFormat) {
        case FRAMEWORK_FORMAT_YV12:
            subUpdateYUVGLTex(GL_TEXTURE1, mUtex,
                              x, y, cwidth, cheight,
                              pixels + uoff, false);
            subUpdateYUVGLTex(GL_TEXTURE2, mVtex,
                              x, y, cwidth, cheight,
                              pixels + voff, false);
            doYUVConversionDraw(mProgram,
                                mYWidthCutoffLoc,
                                mCWidthCutoffLoc,
                                mYSamplerLoc,
                                mUSamplerLoc,
                                mVSamplerLoc,
                                mVUSamplerLoc,
                                mInCoordLoc,
                                mPosLoc,
                                mVbuf, mIbuf,
                                width, ywidth,
                                width / 2, cwidth,
                                mYWidthCutoff,
                                mCWidthCutoff,
                                false);
            break;
        case FRAMEWORK_FORMAT_YUV_420_888:
            if (emugl::emugl_feature_is_enabled(
                android::featurecontrol::YUV420888toNV21)) {
                subUpdateYUVGLTex(GL_TEXTURE1, mVUtex,
                                  x, y, cwidth, cheight,
                                  pixels + voff, true);
                doYUVConversionDraw(mProgram,
                                    mYWidthCutoffLoc,
                                    mCWidthCutoffLoc,
                                    mYSamplerLoc,
                                    mUSamplerLoc,
                                    mVSamplerLoc,
                                    mVUSamplerLoc,
                                    mInCoordLoc,
                                    mPosLoc,
                                    mVbuf, mIbuf,
                                    width, ywidth,
                                    width / 2, cwidth,
                                    mYWidthCutoff,
                                    mCWidthCutoff,
                                    true);
            } else {
                subUpdateYUVGLTex(GL_TEXTURE1, mUtex,
                                  x, y, cwidth, cheight,
                                  pixels + uoff, false);
                subUpdateYUVGLTex(GL_TEXTURE2, mVtex,
                                  x, y, cwidth, cheight,
                                  pixels + voff, false);
                doYUVConversionDraw(mProgram,
                                    mYWidthCutoffLoc,
                                    mCWidthCutoffLoc,
                                    mYSamplerLoc,
                                    mUSamplerLoc,
                                    mVSamplerLoc,
                                    mVUSamplerLoc,
                                    mInCoordLoc,
                                    mPosLoc,
                                    mVbuf, mIbuf,
                                    width, ywidth,
                                    width / 2, cwidth,
                                    mYWidthCutoff,
                                    mCWidthCutoff,
                                    false);
            }
            break;
        case FRAMEWORK_FORMAT_NV12:
            subUpdateYUVGLTex(GL_TEXTURE1, mUVtex,
                              x, y, cwidth, cheight,
                              pixels + uoff, true);
            doYUVConversionDraw(mProgram,
                                mYWidthCutoffLoc,
                                mCWidthCutoffLoc,
                                mYSamplerLoc,
                                mUSamplerLoc,
                                mVSamplerLoc,
                                mVUSamplerLoc,
                                mInCoordLoc,
                                mPosLoc,
                                mVbuf, mIbuf,
                                width, ywidth,
                                width / 2, cwidth,
                                mYWidthCutoff,
                                mCWidthCutoff,
                                true);
            break;
        default:
            FATAL("Unknown format: 0x%x", mFormat);
    }

    restoreGLState();
}

void YUVConverter::updateCutoffs(float width, float ywidth,
                                 float halfwidth, float cwidth) {
    switch (mFormat) {
    case FRAMEWORK_FORMAT_YV12:
        mYWidthCutoff = ((float)width) / ((float)ywidth);
        mCWidthCutoff = ((float)halfwidth) / ((float)cwidth);
        break;
    case FRAMEWORK_FORMAT_YUV_420_888:
        mYWidthCutoff = 1.0f;
        mCWidthCutoff = 1.0f;
        break;
    case FRAMEWORK_FORMAT_NV12:
        mYWidthCutoff = 1.0f;
        mCWidthCutoff = 1.0f;
        break;
    case FRAMEWORK_FORMAT_GL_COMPATIBLE:
        FATAL("Input not a YUV format!");
    }
}

void YUVConverter::reset() {
    if (mIbuf) s_gles2.glDeleteBuffers(1, &mIbuf);
    if (mVbuf) s_gles2.glDeleteBuffers(1, &mVbuf);
    if (mProgram) s_gles2.glDeleteProgram(mProgram);
    if (mYtex) s_gles2.glDeleteTextures(1, &mYtex);
    if (mUtex) s_gles2.glDeleteTextures(1, &mUtex);
    if (mVtex) s_gles2.glDeleteTextures(1, &mVtex);
    if (mVUtex) s_gles2.glDeleteTextures(1, &mVUtex);
    if (mUVtex) s_gles2.glDeleteTextures(1, &mUVtex);
    mIbuf = 0;
    mVbuf = 0;
    mProgram = 0;
    mYtex = 0;
    mUtex = 0;
    mVtex = 0;
    mVUtex = 0;
    mUVtex = 0;
}

YUVConverter::~YUVConverter() {
    reset();
}
