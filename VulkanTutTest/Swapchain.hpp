#pragma once
#include <UtilStructs.hpp>
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <GLFW/glfw3.h>
#include <algorithm>

class Swapchain {
public:
	VkSwapchainKHR swapChain;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	VkFormat format;
	VkExtent2D extent;

	Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, GLFWwindow* window);

	Swapchain();
	~Swapchain();

	void CreateImageViews();

	VkImageView CreateImageView(VkImage image, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags);

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

	inline VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	inline VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availablePresentMode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	inline VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}

		// TODO: Consider ranking rather than returning the first format available!
		return availableFormats[0];
	}

	inline QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
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

private:
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
	GLFWwindow* window;
};
