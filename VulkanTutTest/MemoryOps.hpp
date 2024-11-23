#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <assert.h>
#include <stdexcept>
#include <fstream>

/// <summary>
/// Finds memory type based on properties and physical device
/// </summary>
/// <param name="physicalDevice"></param>
/// <param name="typeFilter"></param>
/// <param name="properties"></param>
/// <returns></returns>
inline uint32_t FindMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

/// <summary>
/// Reads file (meant for SPVs)
/// </summary>
/// <param name="filename"></param>
/// <returns></returns>
static std::vector<char> ReadFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    // Allocate memory
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    // Read from beginning
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

/// <summary>
/// Returns whether the given format includes a stencil component
/// </summary>
/// <param name="format"></param>
/// <returns></returns>
inline bool HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

/// <summary>
/// Creates a shader module from SPV code
/// </summary>
/// <param name="code"></param>
/// <returns></returns>
VkShaderModule CreateShaderModule(VkDevice& device, const std::vector<char>& code);

/// <summary>
/// Transitions image from a layout to another, as part of the current command buffer
/// </summary>
/// <param name="image"></param>
/// <param name="mipLevels"></param>
/// <param name="format"></param>
/// <param name="oldLayout"></param>
/// <param name="newLayout"></param>
/// <param name="aspect"></param>
/// <param name="layerCount"></param>
void TransitionImageLayoutCmd(VkImage image, uint32_t mipLevels, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspect, VkCommandBuffer commandBuffer, uint32_t layerCount = 1);
