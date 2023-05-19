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

#include "GLES3/gl3.h"  // for GLuint

namespace gfxstream {
namespace gl {
struct GLESv2Dispatch;
}  // namespace gl
}  // namespace gfxstream

// This class encapsulates the logic necessary to draw the
// contents of the texture on screen.
class TextureDraw {
public:
    // |gl_dispatch| is the GL ES 2 dispatch table.
    explicit TextureDraw(const gfxstream::gl::GLESv2Dispatch* gl_dispatch);

    // Frees the resources associated with this instance.
    ~TextureDraw();

    // Renders the contents of 2D |input_texture| on screen
    // |width| and |height| are the dimensions of the texture.
    void draw(GLuint input_texture, int width, int height);

private:
    const gfxstream::gl::GLESv2Dispatch* mGLES2;
    GLuint mProgram;
    GLuint mVertexBuffer;
    GLuint mInputUniformLocation;
    GLuint mResolutionUniformLocation;
    GLuint mPositionAttribLocation;
};
