#pragma once

#include <vulkan/vulkan.h>
#include <MemoryOps.hpp>
#include <Swapchain.hpp>
#include <Vertex.hpp>
#include <string>

class GraphicsPipeline {
public:
	VkPipeline* pipeline;

	GraphicsPipeline(VkDevice& device, Swapchain& swapChain, const VkSampleCountFlagBits msaaSamples,
		VkPipelineLayout& pipelineLayout, VkDescriptorSetLayout& descriptorSetLayout, VkRenderPass& renderPass,
		const char* vertShaderFile, const char* fragShaderFile, const char* geomShaderFile, const std::string name,
		VkPipeline& graphicsPipeline);

	GraphicsPipeline();

	~GraphicsPipeline();

private:
	VkDevice device;
	std::string name;
};
