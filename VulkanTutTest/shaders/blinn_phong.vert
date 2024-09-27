#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNorm;
layout(location = 4) in ivec4 inBoneIDs;        // Size of 4 is in accordance with the 4 bone per vertex convention
layout(location = 5) in vec4 inBoneWeights;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNorm;
layout(location = 3) out vec3 fragPos;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0f);
    fragColor = inColor;
    // fragColor = inBoneWeights.xyz;
    fragTexCoord = inTexCoord;
    // fragNorm = inNorm;
    fragNorm = mat3(transpose(inverse(ubo.model))) * inNorm;
    fragPos = inPos;
}