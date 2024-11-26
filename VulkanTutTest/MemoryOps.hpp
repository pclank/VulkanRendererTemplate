#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <assert.h>
#include <stdexcept>
#include <fstream>
#include <UtilStructs.hpp>

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
/// Finds queue families of physical device
/// </summary>
/// <param name="device"></param>
/// <returns></returns>
inline QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        if (!(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
            indices.transferFamily = i;

        VkBool32 presentSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
            indices.presentFamily = i;

        if (indices.IsComplete())
            break;

        i++;
    }

    return indices;
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
/// Begins command buffer command
/// </summary>
/// <param name="device"></param>
/// <param name="commandPool"></param>
/// <returns></returns>
inline VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}


/// <summary>
/// Submits command buffer
/// </summary>
/// <param name="device"></param>
/// <param name="commandBuffer"></param>
/// <param name="commandPool"></param>
/// <param name="queue"></param>
inline void EndSingleTimeCommands(VkDevice device, VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

/// <summary>
/// Copies a staging buffer to image
/// </summary>
/// <param name="buffer"></param>
/// <param name="image"></param>
/// <param name="device"></param>
/// <param name="commandPool"></param>
/// <param name="graphicsQueue"></param>
/// <param name="width"></param>
/// <param name="height"></param>
/// <param name="layerCount"></param>
inline void CopyBufferToImage(VkBuffer buffer, VkImage image, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
    uint32_t width, uint32_t height, uint32_t layerCount = 1)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    EndSingleTimeCommands(device, commandBuffer, commandPool, graphicsQueue);
}

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

/// <summary>
/// Transitions image from a layout to another, starting a new command buffer
/// </summary>
/// <param name="image"></param>
/// <param name="device"></param>
/// <param name="commandPool"></param>
/// <param name="graphicsQueue"></param>
/// <param name="mipLevels"></param>
/// <param name="format"></param>
/// <param name="oldLayout"></param>
/// <param name="newLayout"></param>
/// <param name="aspect"></param>
/// <param name="layerCount"></param>
void TransitionImageLayout(VkImage image, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, uint32_t mipLevels, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspect, uint32_t layerCount = 1);

/// <summary>
/// Creates staging buffer (and more???)
/// </summary>
/// <param name="device"></param>
/// <param name="physicalDevice"></param>
/// <param name="surface"></param>
/// <param name="size"></param>
/// <param name="usage"></param>
/// <param name="properties"></param>
/// <param name="buffer"></param>
/// <param name="bufferMemory"></param>
void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);