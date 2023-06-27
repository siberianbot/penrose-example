#version 460

struct ModelData {
    mat4 matrix;
    mat4 rotOnly;
};

layout (push_constant) uniform RenderData {
    mat4 matrix;
    ModelData model;
} data;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inUV;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec2 outUV;

void main() {
    gl_Position = data.matrix * vec4(inPos, 1);

    outPos = (data.model.matrix * vec4(inPos, 1)).xyz;
    outNormal = (data.model.rotOnly * vec4(inNormal, 1)).xyz;
    outColor = inColor;
    outUV = inUV;
}
