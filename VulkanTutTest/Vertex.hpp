#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

#define MAXIMUM_BONES 4

/// <summary>
/// Struct containing Bone information
/// </summary>
struct BoneInfo
{
    int id;										// Bone ID index
    glm::mat4 offsetMatrix;						// Offset matrix for bone space
    glm::mat4 bone_transform = glm::mat4(0.0f);	// Final bone tranformation matrix
    glm::mat4x2 dual_quat = glm::mat4x2(0.0f);	// Final bone dual quaternion
};

/// <summary>
/// Struct containing Texture information
/// </summary>
struct Texture
{
    unsigned int id;							// Texture ID
    std::string type;							// Type of texture (diffuse, normal, specular, height)
    std::string path;							// Expected path of texture
};

/// <summary>
/// Struct containing Vertex information
/// </summary>
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 norm;
    glm::vec3 tangent;							// NOT USED YET ~~~ 
    glm::vec3 biTangent;						// NOT USED YET ~~~
    unsigned int bone_num = 0;					// Number of bones affecting the vertex. Used to add bone IDs and weights properly
    int boneIDs[MAXIMUM_BONES] = { 0 };			// Initialized to zeros
    float weights[MAXIMUM_BONES] = { 0.0f };	// Initialized to zeros

    bool operator==(const Vertex& other) const
    {
        // TODO: Extend!
        return pos == other.pos && color == other.color && texCoord == other.texCoord && norm == other.norm;
    }

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 6> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, norm);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SINT;
        attributeDescriptions[4].offset = offsetof(Vertex, boneIDs);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[5].offset = offsetof(Vertex, weights);

        return attributeDescriptions;
    }
};
