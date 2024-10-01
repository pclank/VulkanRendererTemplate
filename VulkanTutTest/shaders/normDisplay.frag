#version 450

layout(location = 0) out vec4 outColor;

// layout(binding = 1) uniform sampler2D texSampler;
// 
// layout(binding = 2) uniform LightDataUBO {  
//     vec3 lightPos;
//     vec3 camPos;
//     bool blinn;
// } lightData;
// 
// layout(binding = 3) uniform sampler2D normalSampler;

void main()
{
    outColor = vec4(1.0f, 1.0f, 1.0f, 0.6f);
}