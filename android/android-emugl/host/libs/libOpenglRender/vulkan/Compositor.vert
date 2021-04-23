#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform UniformBufferObject {
    mat4 posTransform;
    mat4 texcoordTransform;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = vec4((ubo.posTransform * vec4(inPosition, 0.0, 1.0)).xy, 0.0, 1.0);
    fragTexCoord = (ubo.texcoordTransform * vec4(texCoord, 0.0, 1.0)).xy;
}
