#version 450
// *****************************************************
// Shader that implements Linear Skinning
// *****************************************************

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

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNorm;

void main()
{
    vec4 newPosition;

    // Loop between 4 bones for position
    mat4 finalBoneTransform = ubo.inBoneTransforms[inBoneIDs.x] * inBoneWeights.x;
    finalBoneTransform += ubo.inBoneTransforms[inBoneIDs.y] * inBoneWeights.y;
    finalBoneTransform += ubo.inBoneTransforms[inBoneIDs.z] * inBoneWeights.z;
    finalBoneTransform += ubo.inBoneTransforms[inBoneIDs.w] * inBoneWeights.w;

    // Calculate final vertex position
    newPosition = finalBoneTransform * vec4(inPos, 1.0f);

    // Calculate final normal direction
    vec4 newNormal = finalBoneTransform * vec4(inNorm, 0.0f);

    // TODO: Tangent and Bitangent will also be affected by finalBoneTransform!

    // gl_Position = ubo.proj * ubo.view * ubo.model * newPosition;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0f);
    // fragNorm = mat3(transpose(inverse(ubo.model))) * newNormal.xyz;
    fragNorm = inNorm;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}