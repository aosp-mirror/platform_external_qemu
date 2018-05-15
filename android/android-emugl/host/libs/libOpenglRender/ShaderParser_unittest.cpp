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

#include "android/base/StringView.h"
#include "android/base/memory/LazyInstance.h"

#include <GLSLANG/ShaderLang.h>
#include <gtest/gtest.h>

#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

using android::base::LazyInstance;
using android::base::StringView;

namespace emugl {

// Class and LazyInstance to handle the fact that ShInitialize and ShFinalize
// are only called once per process.
class ShaderParserSetup {
public:
    ShaderParserSetup() {
        ShInitialize();
    }

    ~ShaderParserSetup() {
        ShFinalize();
    }
};

static LazyInstance<ShaderParserSetup> sParser = LAZY_INSTANCE_INIT;

class ShaderParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        sParser.get();

        ShInitBuiltInResources(&mResources);

        // A reasonable mix of Swiftshader capabilities and spec minimums for
        // ES 3.0.
        mResources.MaxVertexAttribs = 16;
        mResources.MaxVertexUniformVectors = 256;
        mResources.MaxVaryingVectors = 32;
        mResources.MaxVertexTextureImageUnits = 16;
        mResources.MaxCombinedTextureImageUnits = 32;
        mResources.MaxTextureImageUnits = 32;
        mResources.MaxFragmentUniformVectors = 224;
        mResources.MaxDrawBuffers = 8;
        mResources.MaxVertexOutputVectors = 16;
        mResources.MaxFragmentInputVectors = 16;
        mResources.MinProgramTexelOffset = -8;
        mResources.MaxProgramTexelOffset = 7;
        mResources.MaxDualSourceDrawBuffers = 1;

        mResources.OES_standard_derivatives = 1;
        // Allow testing external samplers
        mResources.OES_EGL_image_external = 1;
        mResources.EXT_gpu_shader5 = 1;
    }

    void TearDown() override { }

    std::string compileShaderWithSpec(StringView shader,
                                      GLenum shaderType,
                                      ShShaderSpec inputSpec,
                                      ShShaderOutput outputSpec,
                                      bool* success) {
        ShHandle compilerHandle =
            ShConstructCompiler(
                shaderType, inputSpec, outputSpec, &mResources);

        const char* shaderSrc = shader.c_str();
        bool res = ShCompile(compilerHandle, &shaderSrc, 1, SH_OBJECT_CODE | SH_VARIABLES);

        if (success) *success = res;

        std::string infolog = ShGetInfoLog(compilerHandle);
        std::string result = ShGetObjectCode(compilerHandle);

        ShClearResults(compilerHandle);

        ShDestruct(compilerHandle);

        return result;
    }

private:

    ShBuiltInResources mResources;
};
 
TEST_F(ShaderParserTest, Init) { }

TEST_F(ShaderParserTest, SimpleShader) {
    const char fragmentWithExternalSampler[] = R"(
uniform samplerExternalOES samp; // external sampler that we hope to convert to sampler2D

void main() {
    gl_FragColor = texture2D(samp, vec2(0.0,0.0));
}
)";

    bool success;

    auto res = compileShaderWithSpec(fragmentWithExternalSampler, GL_FRAGMENT_SHADER, SH_GLES2_SPEC, SH_GLSL_330_CORE_OUTPUT, &success);

    EXPECT_TRUE(success);
    EXPECT_NE(res, "");
}


}  // namespace emugl
