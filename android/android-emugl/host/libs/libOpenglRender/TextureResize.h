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
#ifndef _LIBRENDER_TEXTURERESIZE_H
#define _LIBRENDER_TEXTURERESIZE_H

#include "android/skin/rect.h"
#include <GLES2/gl2.h>
#include <memory>

class TextureResize {
public:
    TextureResize(GLuint width, GLuint height);
    ~TextureResize();

    // Scales the given texture for the current viewport and returns the scaled
    // texture. May return the input if no scaling is required.
    GLuint update(GLuint texture);
    GLuint update(GLuint texture, int width, int height, SkinRotation rotation);

    struct Framebuffer {
        GLuint texture = 0;
        GLuint framebuffer = 0;
        GLuint program = 0;
        GLuint aPosition = 0;
        GLuint uTexture = 0;
    };

    class GenericResizer {
    public:
        GenericResizer();
        ~GenericResizer();

        // Renders the contents of 2D |input_texture| on screen
        // |width| and |height| are the dimensions of the texture.
        GLuint draw(GLuint texture, int width, int height, SkinRotation rotation);

    private:
        GLuint mProgram;
        GLuint mVertexBuffer;
        GLuint mIndexBuffer;
        GLuint mInputUniformLocation;
        GLuint mPositionAttribLocation;
        GLuint mInCoordAttribLocation;
        Framebuffer mFrameBuffer;
        int mWidth;
        int mHeight;
    };

private:
    void setupFramebuffers(unsigned int factor);
    void resize(GLuint texture);

private:
    GLuint mWidth;
    GLuint mHeight;
    unsigned int mFactor;
    Framebuffer mFBWidth;
    Framebuffer mFBHeight;
    GLuint mVertexBuffer;
    GLenum mTextureDataType;
    GLenum mTextureFilteringMode = GL_LINEAR;
    std::unique_ptr<GenericResizer> mGenericResizer;
};

#endif
