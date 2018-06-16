// Copyright (C) 2018 The Android Open Source Project
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

#include <gtest/gtest.h>

#include "OpenGLTestContext.h"
#include "TextureDraw.h"

namespace emugl {

using namespace gltest;

static testing::AssertionResult RowsMatch(unsigned char* expected,
                                          unsigned char* actual, int rowIndex, size_t rowBytes) {
    for (size_t i = 0; i < rowBytes; ++i) {
        if (expected[i] != actual[i]) {
            return testing::AssertionFailure() <<
                       "row " << rowIndex << " byte " << i << " mismatch. expected: " <<
                       "(" << expected[i] << "), actual: " << "(" << actual[i] << ")";
        }
    }
    return testing::AssertionSuccess();
}

TEST_F(GLTest, TextureDrawBasic) {
    const GLESv2Dispatch* gl = LazyLoadedGLESv2Dispatch::get();

    GLint viewport[4] = {};
    gl->glGetIntegerv(GL_VIEWPORT, viewport);
    EXPECT_EQ(0, viewport[0]);
    EXPECT_EQ(0, viewport[1]);
    const int width = viewport[2];
    const int height = viewport[3];
    const GLint internalformat = (GLint)GL_RGBA;
    const GLenum format = GL_RGBA;
    const GLenum type = GL_UNSIGNED_BYTE;
    const int bpp = 4;
    const int bytes = width * height * bpp;

    GLuint textureToDraw;

    gl->glGenTextures(1, &textureToDraw);
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, textureToDraw);

    gl->glPixelStorei(GL_PACK_ALIGNMENT, 1);
    gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::vector<unsigned char> pixels(bytes);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pixels[i * width * bpp + j * bpp + 0] = (0xaa + i) % 0x100;
            pixels[i * width * bpp + j * bpp + 1] = (0x00 + j) % 0x100;
            pixels[i * width * bpp + j * bpp + 2] = (0x11 + i) % 0x100;
            pixels[i * width * bpp + j * bpp + 3] = (0xff + j) % 0x100;
        }
    }

    gl->glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0,
                     format, type, pixels.data());
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLint fbStatus = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    EXPECT_EQ((GLint)GL_FRAMEBUFFER_COMPLETE, fbStatus);

    TextureDraw textureDraw;

    textureDraw.draw(textureToDraw, 0, 0, 0);

    std::vector<unsigned char> pixelsOut(bytes, 0xff);

    gl->glReadPixels(0, 0, width, height, format, type, pixelsOut.data());

    // Check that the texture is drawn upside down (because that's what SurfaceFlinger wants)
    for (int i = 0; i < height; i++) {
        size_t rowBytes = width * bpp;
        EXPECT_TRUE(RowsMatch(pixels.data() + i * rowBytes,
                              pixelsOut.data() + (height - i - 1) * rowBytes,
                              i,
                              width * bpp));
    }
}

}  // namespace emugl
