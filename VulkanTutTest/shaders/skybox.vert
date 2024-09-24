#version 450
// *****************************************************
// Shader used for the Skybox
// *****************************************************

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 fragTexCoord;

void main()
{
    fragTexCoord = inPos;
    gl_Position = (ubo.proj * ubo.view * vec4(inPos, 1.0f)).xyww;
}  