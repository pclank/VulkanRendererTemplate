#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <assert.h>
#include <stdexcept>

class RenderPass
{
public:
	RenderPass();

	RenderPass(VkDevice device, VkPipelineBindPoint bindPoint,
		const std::vector<VkFormat>& formats,
		const std::vector<VkSampleCountFlagBits>& samples,
		const std::vector<VkAttachmentLoadOp>& loadOps,
		const std::vector<VkAttachmentStoreOp>& storeOps,
		const std::vector<VkImageLayout>& initialLayouts,
		const std::vector<VkImageLayout>& finalLayouts,
		const VkPipelineStageFlags dependencySrcStageMask,
		const VkPipelineStageFlags dependencySrcAccessMask,
		const VkPipelineStageFlags dependencyDstStageMask,
		const VkPipelineStageFlags dependencyDstAccessMask,
		const std::string& name,
		VkRenderPass& renderPass);

	~RenderPass();

	void Destroy() const;

	VkRenderPass* renderPass;
	std::string name;

private:
	VkDevice device;
};
