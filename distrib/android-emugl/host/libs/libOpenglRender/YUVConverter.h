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

// The purpose of YUVConverter is to use
// OpenGL shaders to convert YUV images to RGB
// images that can be displayed on screen.
// Doing this on the GPU can be much faster than
// on the CPU.
class YUVConverter {
public:
    // call ctor when creating a gralloc buffer
    // with YUV format
    YUVConverter(int width, int height);
    // destroy when ColorBuffer is destroyed
    ~YUVConverter();
    // call when gralloc_unlock updates
    // the host color buffer
    // (rcUpdateColorBuffer)
    void drawConvert(int x, int y, int width, int height, char* pixels);
private:
    // We need the following GL objects:
    GLuint mVshader = 0;
    GLuint mFshader = 0; // Fragment shader (does actual conversion)
    GLuint mProgram = 0;
    GLuint mVbuf = 0;
    GLuint mIbuf = 0;
    GLuint mYtex = 0;
    GLuint mUtex = 0;
    GLuint mVtex = 0;
};
