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
	VkDeviceMemory imageMemory;
	VkImageView imageView;

	Image(VkDevice& device, VkPhysicalDevice physicalDevice, VkFormat format, uint32_t width, uint32_t height,
		uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags, std::string name = "unknown",
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1);

	Image();

	~Image();

	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	VkImageView CreateImageView(VkImage image, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags,
		VkImageViewType viewType, uint32_t layerCount);

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
