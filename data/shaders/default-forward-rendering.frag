#version 460

layout (set = 0, binding = 0) uniform sampler2D albedoTexture;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inUV;

layout (location = 0) out vec4 outAlbedo;

void main() {
    outAlbedo = vec4(inColor, 1) * texture(albedoTexture, inUV);
}
