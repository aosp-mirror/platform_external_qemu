// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "OpenGLESDispatch/GLESv2Dispatch.h"

// This class encapsulates the logic necessary to apply a screen-space
// antialiasing technique.
class GLAntiAliasing {
public:
    // |gl_dispatch| is the GL ES 2 dispatch table.
    explicit GLAntiAliasing(const GLESv2Dispatch* gl_dispatch);

    // Frees the resources associated with this instance.
    ~GLAntiAliasing();

    // Renders the contents of 2D |input_texture| on screen with anti-aliasing
    // applied. |width| and |height| are the dimensions of the texture.
    void apply(GLuint input_texture, int width, int height);

private:
    const GLESv2Dispatch* mGLES2;
    GLuint mAAProgram;
    GLuint mAAVertexBuffer;
    GLuint mAAInputUniformLocation;
    GLuint mAAResolutionUniformLocation;
    GLuint mAAPositionAttribLocation;
};