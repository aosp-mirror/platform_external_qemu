// Copyright 2016 The Android Open Source Project
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

#include <string>

#include <GLES2/gl2.h>

#include <GLSLANG/ShaderLang.h>

class ANGLEShaderParser {
public:
    static bool initialize();

    static bool translate(const char* src, GLenum shaderType,
                          std::string* outInfolog, std::string* outObjCode);

private:
    static void initializeResources();

    static ShBuiltInResources kResources;
    static ShHandle kVertCompiler;
    static ShHandle kFragCompiler;
    static bool kInitialized;

    static const ShShaderOutput kGlslVersion = SH_GLSL_COMPATIBILITY_OUTPUT;
};
