#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNorm;
layout(location = 3) in vec3 fragPos;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform LightDataUBO {  
    vec3 lightPos;
    vec3 camPos;
    bool blinn;
} lightData;

layout(binding = 3) uniform sampler2D normalSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = texture(texSampler, fragTexCoord).rgb;

    // ambient
    vec3 ambient = 0.05f * color;

    // diffuse
    vec3 lightDir = normalize(lightData.lightPos - fragPos);
    // vec3 normal = normalize(fragNorm);
    vec3 normal = normalize(texture(normalSampler, fragTexCoord).rgb);
    float diff = max(dot(lightDir, normal), 0.0f);
    vec3 diffuse = diff * color;

    // specular
    vec3 viewDir = normalize(lightData.camPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float spec = 0.0;
    if (lightData.blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0f), 32.0f);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0f), 8.0f);
    }

    vec3 specular = vec3(0.3f) * spec; // assuming bright white light color

    // Output
    outColor = vec4(ambient + diffuse + specular, 1.0f);
    // outColor = vec4(texture(normalSampler, fragTexCoord).rgb, 1.0f);
}