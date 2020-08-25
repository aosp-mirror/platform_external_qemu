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

#pragma once

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "FrameworkFormats.h"

#include <stdint.h>
#include <cstring>
#include <vector>
// The purpose of YUVConverter is to use
// OpenGL shaders to convert YUV images to RGB
// images that can be displayed on screen.
// Doing this on the GPU can be much faster than
// on the CPU.

// Usage:
// 0. Have a current OpenGL context working.
// 1. Constructing the YUVConverter object will allocate
//    OpenGL resources needed to convert, given the desired
//    |width| and |height| of the buffer.
// 2. To convert a given YUV buffer of |pixels|, call
//    the drawConvert method (with x, y, width, and height
//    arguments depicting the region to convert).
//    The RGB version of the YUV buffer will be drawn
//    to the current framebuffer. To retrieve
//    the result, if the user of the result is an OpenGL texture,
//    it suffices to have that texture be the color attachment
//    of the framebuffer object. Or, if you want the results
//    on the CPU, call glReadPixels() after the call to drawConvert().
class YUVConverter {
public:
    // call ctor when creating a gralloc buffer
    // with YUV format
    YUVConverter(int width, int height, FrameworkFormat format);
    // destroy when ColorBuffer is destroyed
    ~YUVConverter();
    // call when gralloc_unlock updates
    // the host color buffer
    // (rcUpdateColorBuffer)
    void drawConvert(int x, int y, int width, int height, char* pixels);

    uint32_t getDataSize();
    // read YUV data into pixels, exactly pixels_size bytes;
    // if size mismatches, will read nothing.
    void readPixels(uint8_t* pixels, uint32_t pixels_size);

    void swapTextures(uint32_t type, uint32_t* textures);

    // NV12 and YUV420 are all packed
    static void NV12ToYUV420PlanarInPlaceConvert(int nWidth,
                                                 int nHeight,
                                                 uint8_t* pFrame,
                                                 uint8_t* pQuad) {
        std::vector<uint8_t> tmp;
        if (pQuad == nullptr) {
            tmp.resize(nWidth * nHeight / 4);
            pQuad = tmp.data();
        }
        int nPitch = nWidth;
        uint8_t *puv = pFrame + nPitch * nHeight, *pu = puv,
                *pv = puv + nPitch * nHeight / 4;
        for (int y = 0; y < nHeight / 2; y++) {
            for (int x = 0; x < nWidth / 2; x++) {
                pu[y * nPitch / 2 + x] = puv[y * nPitch + x * 2];
                pQuad[y * nWidth / 2 + x] = puv[y * nPitch + x * 2 + 1];
            }
        }
        memcpy(pv, pQuad, nWidth * nHeight / 4);
    }

    // public so other classes can call
    static void createYUVGLTex(GLenum texture_unit,
                               GLsizei width,
                               GLsizei height,
                               GLuint* texName_out,
                               bool uvInterleaved);
private:
    void init(int w, int h, FrameworkFormat format);
    void reset();

    // For dealing with n-pixel-aligned buffers
    void updateCutoffs(float width, float ywidth, float halfwidth, float cwidth);
    int mWidth = 0;
    int mHeight = 0;
    FrameworkFormat mFormat;
    // colorbuffer w/h/format, could be different
    int mCbWidth = 0;
    int mCbHeight = 0;
    FrameworkFormat mCbFormat;
    // We need the following GL objects:
    GLuint mProgram = 0;
    GLuint mVbuf = 0;
    GLuint mIbuf = 0;
    GLuint mYtex = 0;
    GLuint mUtex = 0;
    GLuint mVtex = 0;
    GLuint mVUtex = 0;
    GLuint mUVtex = 0;
    // shader uniform locations
    GLint mYWidthCutoffLoc = -1;
    GLint mCWidthCutoffLoc = -1;
    GLint mYSamplerLoc = -1;
    GLint mUSamplerLoc = -1;
    GLint mVSamplerLoc = -1;
    GLint mVUSamplerLoc = -1;
    GLint mInCoordLoc = -1;
    GLint mPosLoc = -1;
    float mYWidthCutoff = 1.0;
    float mCWidthCutoff = 1.0;

    // YUVConverter can end up being used
    // in a TextureDraw / subwindow context, and subsequently
    // overwrite the previous state.
    // This section is so YUVConverter can be used in the middle
    // of any GL context without impacting what's
    // already going on there, by saving/restoring the state
    // that it is impacting.
    void saveGLState();
    void restoreGLState();
    // Impacted state
    GLfloat mCurrViewport[4] = {};
    GLint mCurrTexUnit = 0;
    GLint mCurrProgram = 0;
    GLint mCurrTexBind = 0;
    GLint mCurrVbo = 0;
    GLint mCurrIbo = 0;
};
