/*
* Copyright (C) 2017 The Android Open Source Project
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

const char kDrawTexOESCore_vshader[] = R"(#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texcoord;
out vec2 texcoord_varying;
void main() {
    gl_Position = vec4(pos.x, pos.y, pos.z, 1.0);
    texcoord_varying = texcoord;
}
)";

const char kDrawTexOESCore_fshader[] = R"(#version 330 core
uniform sampler2D tex_sampler;
in vec2 texcoord_varying;
out vec4 frag_color;
void main() {
    frag_color = texture(tex_sampler, texcoord_varying);
}
)";

// flat,
const char kGeometryDrawVShaderSrcTemplate[] = R"(#version 330 core
layout(location = 0) in vec4 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in float pointsize;
layout(location = 4) in vec4 texcoord;

uniform mat4 projection;
uniform mat4 modelview;
uniform mat4 texture_matrix;

out vec4 pos_varying;
out vec3 normal_varying;
%s out vec4 color_varying;
out float pointsize_varying;
out vec4 texcoord_varying;

void main() {

    pos_varying = modelview * pos;
    normal_varying = (modelview * vec4(normal.xyz, 0)).xyz;
    color_varying = color;
    pointsize_varying = pointsize;
    texcoord_varying = texture_matrix * texcoord;

    gl_Position = projection * modelview * pos;
}
)";

// flat,
const char kGeometryDrawFShaderSrcTemplate[] = R"(#version 330 core
uniform sampler2D tex_sampler;
uniform samplerCube tex_cube_sampler;
uniform bool enable_textures;
uniform bool enable_lighting;
uniform bool enable_fog;

uniform bool enable_reflection_map;

uniform int texture_env_mode;
uniform int texture_format;

in vec4 pos_varying;
in vec3 normal_varying;
%s in vec4 color_varying;
in float pointsize_varying;
in vec4 texcoord_varying;

out vec4 frag_color;

// Don't use the enum defines, it can result in
// syntax errors on some GPUs.
#define kModulate 0x2100
#define kCombine 0x8570
#define kReplace 0x1E01

#define kAlpha 0x1906
#define kRGB 0x1907
#define kRGBA 0x1908
#define kLuminance 0x1909
#define kLuminanceAlpha 0x190A

void main() {
    if (enable_textures) {
        vec4 textureColor;
        if (enable_reflection_map) {
            textureColor = texture(tex_cube_sampler, reflect(pos_varying.xyz, normalize(normal_varying)));
            frag_color = textureColor;
        } else {
            textureColor = texture(tex_sampler, texcoord_varying.xy);
            switch (texture_env_mode) {
            case kReplace:
                switch (texture_format) {
                case kAlpha:
                    frag_color.rgb = color_varying.rgb;
                    frag_color.a = textureColor.a;
                    break;
                case kRGBA:
                case kLuminanceAlpha:
                    frag_color.rgba = textureColor.rgba;
                    break;
                case kRGB:
                case kLuminance:
                default:
                    frag_color.rgb = textureColor.rgb;
                    frag_color.a = color_varying.a;
                    break;
                }
                break;
            case kCombine:
            case kModulate:
            default:
                switch (texture_format) {
                case kAlpha:
                    frag_color.rgb = color_varying.rgb;
                    frag_color.a = color_varying.a * textureColor.a;
                    break;
                case kRGBA:
                case kLuminanceAlpha:
                    frag_color.rgba = color_varying.rgba * textureColor.rgba;
                    break;
                case kRGB:
                case kLuminance:
                default:
                    frag_color.rgb = color_varying.rgb * textureColor.rgb;
                    frag_color.a = color_varying.a;
                    break;
                }
                break;
            }
        }
    } else {
        frag_color = color_varying;
    }

    if (enable_lighting) {
        frag_color= vec4(1,1,0,1);
    }
}
)";
