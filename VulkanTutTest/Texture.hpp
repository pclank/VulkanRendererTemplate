#pragma once

#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <Image.hpp>
#include <Paths.hpp>
#include <MemoryOps.hpp>

class Texture {
public:
	Image image;
	VkSampler sampler;
	int width, height, nChannels;
	uint32_t mipLevels;
	bool isNormal;
	std::string file;

	Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkCommandPool commandPool, VkQueue graphicsQueue,
		const char* file = TEXTURE_PATH.c_str(), bool isNormal = false);

	Texture();

	~Texture();

	void Setup(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkCommandPool commandPool, VkQueue graphicsQueue,
		const char* file, bool isNormal);

	/// <summary>
	/// Generate mipmaps for texture
	/// </summary>
	/// <param name="image"></param>
	/// <param name="imageFormat"></param>
	/// <param name="texWidth"></param>
	/// <param name="texHeight"></param>
	/// <param name="mipLevels"></param>
	/// <param name="device"></param>
	/// <param name="physicalDevice"></param>
	/// <param name="commandPool"></param>
	/// <param name="graphicsQueue"></param>
	/// <param name="layerCount"></param>
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels,
		VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, uint32_t layerCount = 1);

	/// <summary>
	/// Create the texture sampler
	/// </summary>
	/// <param name="device"></param>
	/// <param name="physicalDevice"></param>
	void CreateTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice);

	/// <summary>
	/// Clean up memory etc
	/// </summary>
	void Cleanup();

private:
	VkDevice device;
};

