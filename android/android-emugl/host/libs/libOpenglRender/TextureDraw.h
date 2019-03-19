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

#ifndef TEXTURE_DRAW_H
#define TEXTURE_DRAW_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include "Hwc2.h"


// Helper class used to draw a simple texture to the current framebuffer.
// Usage is pretty simple:
//
//   1) Create a TextureDraw instance.
//
//   2) Each time you want to draw a texture, call draw(texture, rotation),
//      where |texture| is the name of a GLES 2.x texture object, and
//      |rotation| is an angle in degrees describing the clockwise rotation
//      in the GL y-upwards coordinate space. This function fills the whole
//      framebuffer with texture content.
//
class TextureDraw {
public:
    TextureDraw();
    ~TextureDraw();

    // Fill the current framebuffer with the content of |texture|, which must
    // be the name of a GLES 2.x texture object. |rotationDegrees| is a
    // clockwise rotation angle in degrees (clockwise in the GL Y-upwards
    // coordinate space; only supported values are 0, 90, 180, 270). |dx,dy| is
    // the translation of the image towards the origin.
    bool draw(GLuint texture, float rotationDegrees, float dx, float dy) {
        return drawImpl(texture, rotationDegrees, dx, dy, false);
    }
    // Same as 'draw()', but if an overlay has been provided, that overlay is
    // drawn on top of everything else.
    bool drawWithOverlay(GLuint texture, float rotationDegrees, float dx, float dy) {
        return drawImpl(texture, rotationDegrees, dx, dy, true);
    }

    void setScreenMask(int width, int height, const unsigned char* rgbaData);
    void drawLayer(ComposeLayer* l, int frameWidth, int frameHeight,
                   int cbWidth, int cbHeight, GLuint texture);
    void prepareForDrawLayer();
    void cleanupForDrawLayer();

private:
    bool drawImpl(GLuint texture, float rotationDegrees, float dx, float dy, bool wantOverlay);

    GLuint mVertexShader;
    GLuint mFragmentShader;
    GLuint mProgram;
    GLint mAlpha;
    GLint mComposeMode;
    GLint mColor;
    GLint mCoordTranslation;
    GLint mCoordScale;
    GLint mPositionSlot;
    GLint mInCoordSlot;
    GLint mScaleSlot;
    GLint mTextureSlot;
    GLint mTranslationSlot;
    GLuint mVertexBuffer;
    GLuint mIndexBuffer;

    GLuint mMaskTexture;
    int    mMaskWidth;
    int    mMaskHeight;
    bool   mHaveNewMask;
    bool   mMaskIsValid;
    const unsigned char* mMaskPixels;
    bool   mBlendResetNeeded = false;
};

#endif  // TEXTURE_DRAW_H
