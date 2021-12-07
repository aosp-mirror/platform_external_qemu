// Copyright 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <qobjectdefs.h>  // for Q_OBJECT, slots

#include "GLES3/gl3.h"
#include "android/skin/qt/gl-widget.h"

class MultiDisplayWidget : public GLWidget {
    Q_OBJECT
public:
    MultiDisplayWidget(uint32_t frameWidth,
                       uint32_t frameHeight,
                       uint32_t displayId,
                       QWidget* parent = 0);
    ~MultiDisplayWidget();
    void paintWindow(uint32_t colorBufferId);

private:
    uint32_t mFrameWidth;
    uint32_t mFrameHeight;
    uint32_t mDisplayId;
    GLuint mFrameTexture;
    GLuint mProgram;
    GLuint mVertexShader;
    GLuint mFragmentShader;
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
    uint32_t mColorBufferId;
    GLuint createShader(GLint shaderType, const char* shaderText);
    void clearGL();

protected:
    // This is called once, after the GL context is created, to do some one-off
    // setup work.
    bool initGL() override;

    // Called every time the widget changes its dimensions.
    void resizeGL(int, int) override {}

    // Called every time the widget needs to be repainted.
    void repaintGL() override;
};
