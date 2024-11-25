#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNorm;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in mat3 fragTBN;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    // outColor = vec4(fragColor, 1.0);
    // outColor = vec4(texture(texSampler, fragTexCoord).rgb * fragColor, 1.0f);
    outColor = texture(texSampler, fragTexCoord);
    // outColor = vec4(fragNorm, 1.0f);
}