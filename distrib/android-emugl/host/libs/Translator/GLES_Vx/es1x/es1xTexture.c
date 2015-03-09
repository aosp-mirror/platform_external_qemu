/*
 * Copyright (C) 2011 The Android Open Source Project
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

 
#include "es1xTexture.h"

const GLenum s_supportedCompressedTextureFormats[ES1X_NUM_SUPPORTED_COMPRESSED_TEXTURE_FORMATS] =
{
  GL_PALETTE4_RGB8_OES,
  GL_PALETTE4_RGBA8_OES,
  GL_PALETTE4_R5_G6_B5_OES,
  GL_PALETTE4_RGBA4_OES,
  GL_PALETTE4_RGB5_A1_OES,
  GL_PALETTE8_RGB8_OES,
  GL_PALETTE8_RGBA8_OES,
  GL_PALETTE8_R5_G6_B5_OES,
  GL_PALETTE8_RGBA4_OES,
  GL_PALETTE8_RGB5_A1_OES
};
