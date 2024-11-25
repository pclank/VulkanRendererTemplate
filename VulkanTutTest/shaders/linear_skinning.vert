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
layout(location = 6) in vec3 inTangent;
layout(location = 7) in vec3 inBiTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNorm;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out mat3 fragTBN;

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

    gl_Position = ubo.proj * ubo.view * ubo.model * newPosition;
    // gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0f);
    // fragNorm = mat3(transpose(inverse(ubo.model))) * newNormal.xyz;
    // fragNorm = inNorm;
    fragNorm = newNormal.xyz;
    // fragNorm = (ubo.proj * ubo.view * ubo.model * newPosition).xyz;
    // fragNorm = vec3(inBoneIDs.x, inBoneIDs.y, inBoneIDs.z);
    // fragNorm = vec3(inBoneWeights.x, inBoneWeights.y, inBoneWeights.z);
	
	
	vec4 newTangent = finalBoneTransform * vec4(inTangent, 0.0);
	vec4 newBiTangent = finalBoneTransform * vec4(inBiTangent, 0.0);
	// vec3 T = normalize(vec3(ubo.model * newTangent));
	// vec3 B = normalize(vec3(ubo.model * newBiTangent));
	// vec3 N = normalize(vec3(ubo.model * newNormal));
	
	vec3 T = normalize(vec3(ubo.model * newTangent));
	vec3 N = normalize(vec3(ubo.model * newNormal));
	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);
	// then retrieve perpendicular vector B with the cross product of T and N
	vec3 B = cross(N, T);
	fragTBN = mat3(T, B, N);
	
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragPos = newPosition.xyz;
}