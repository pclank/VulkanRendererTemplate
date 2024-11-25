#version 450

const int MAX_BONES = 120;                      // We need a maximum number, and 120 should be safe for the vast majority of rigs

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 inBoneTransforms[MAX_BONES];
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNorm;
layout(location = 4) in ivec4 inBoneIDs;        // Size of 4 is in accordance with the 4 bone per vertex convention
layout(location = 5) in vec4 inBoneWeights;
layout(location = 6) in vec3 inTangent;
layout(location = 7) in vec3 inbiTangent;

layout(location = 0) out VS_OUT {
    mat4 geomProj;
    vec3 geomNorm;
} vs_out;

void main()
{
    // gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0f);
    gl_Position = ubo.view * ubo.model * vec4(inPos, 1.0f);
    vs_out.geomProj = ubo.proj;
    mat3 normalMatrix = mat3(transpose(inverse(ubo.view * ubo.model)));
    vs_out.geomNorm = normalize(vec3(vec4(normalMatrix * inNorm, 0.0)));
}