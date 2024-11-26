#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <assert.h>
#include <stdexcept>
#include <MemoryOps.hpp>

class Image {
public:
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory imageMemory;

	Image(VkDevice& device, VkPhysicalDevice physicalDevice, VkFormat format, uint32_t width, uint32_t height,
		uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, std::string name = "unknown",
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1);

	Image();

	~Image();

	/// <summary>
	/// Handles image creation
	/// </summary>
	/// <param name="width"></param>
	/// <param name="height"></param>
	/// <param name="mipLevels"></param>
	/// <param name="numSamples"></param>
	/// <param name="format"></param>
	/// <param name="tiling"></param>
	/// <param name="usage"></param>
	/// <param name="properties"></param>
	/// <param name="image"></param>
	/// <param name="imageMemory"></param>
	/// <param name="arrayLevels"></param>
	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImage& image, VkDeviceMemory& imageMemory, uint32_t arrayLevels = 1);

	/// <summary>
	/// Handles imageView creation
	/// </summary>
	/// <param name="mipLevels"></param>
	/// <param name="format"></param>
	/// <param name="aspectFlags"></param>
	/// <param name="viewType"></param>
	/// <param name="layerCount"></param>
	void CreateImageView(uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags,
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1);

private:
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkFormat format;
	uint32_t width, height;
	VkImageTiling tiling;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlags properties;
	VkImageAspectFlags aspectFlags;
	VkImageViewType viewType;
	std::string name;
};
