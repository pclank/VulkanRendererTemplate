#version 450
// *****************************************************
// Shader used for the Skybox
// *****************************************************

layout(location = 0) in vec3 fragTexCoord;

layout(binding = 1) uniform samplerCube skybox;

layout(location = 0) out vec4 outColor;

void main()
{    
    outColor = texture(skybox, fragTexCoord);
}