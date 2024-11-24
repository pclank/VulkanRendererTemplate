#pragma once

#include <vulkan/vulkan.h>
#include <MemoryOps.hpp>
#include <Swapchain.hpp>
#include <Vertex.hpp>
#include <string>

class GraphicsPipeline {
public:
	VkPipeline* pipeline;

	/// <summary>
	/// Constructor for vertex, geometry, and fragment shader graphics pipeline
	/// </summary>
	/// <param name="device"></param>
	/// <param name="swapChain"></param>
	/// <param name="msaaSamples"></param>
	/// <param name="sampleShading"></param>
	/// <param name="polygonMode"></param>
	/// <param name="depthTest"></param>
	/// <param name="depthWrite"></param>
	/// <param name="vertexBindingDescriptions"></param>
	/// <param name="vertexAttributeDescriptions"></param>
	/// <param name="pipelineLayout"></param>
	/// <param name="descriptorSetLayout"></param>
	/// <param name="renderPass"></param>
	/// <param name="vertShaderFile"></param>
	/// <param name="fragShaderFile"></param>
	/// <param name="geomShaderFile"></param>
	/// <param name="name"></param>
	/// <param name="graphicsPipeline"></param>
	GraphicsPipeline(VkDevice& device, Swapchain& swapChain, const VkSampleCountFlagBits msaaSamples, const VkBool32 sampleShading,
		const VkPolygonMode polygonMode, const VkBool32 const depthTest, VkBool32 depthWrite,
		const VkVertexInputBindingDescription vertexBindingDescriptions, const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions,
		VkPipelineLayout& pipelineLayout, VkDescriptorSetLayout& descriptorSetLayout, VkRenderPass& renderPass,
		const char* vertShaderFile, const char* fragShaderFile, const char* geomShaderFile, const std::string name,
		VkPipeline& graphicsPipeline);

	/// <summary>
	/// Constructor for vertex and fragment shader graphics pipeline
	/// </summary>
	/// <param name="device"></param>
	/// <param name="swapChain"></param>
	/// <param name="msaaSamples"></param>
	/// <param name="sampleShading"></param>
	/// <param name="polygonMode"></param>
	/// <param name="depthTest"></param>
	/// <param name="depthWrite"></param>
	/// <param name="vertexBindingDescriptions"></param>
	/// <param name="vertexAttributeDescriptions"></param>
	/// <param name="pipelineLayout"></param>
	/// <param name="descriptorSetLayout"></param>
	/// <param name="renderPass"></param>
	/// <param name="vertShaderFile"></param>
	/// <param name="fragShaderFile"></param>
	/// <param name="geomShaderFile"></param>
	/// <param name="name"></param>
	/// <param name="graphicsPipeline"></param>
	GraphicsPipeline(VkDevice& device, Swapchain& swapChain, const VkSampleCountFlagBits msaaSamples, const VkBool32 sampleShading,
		const VkPolygonMode polygonMode, const VkBool32 depthTest, const VkBool32 depthWrite,
		const VkVertexInputBindingDescription vertexBindingDescriptions, const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions,
		VkPipelineLayout& pipelineLayout, VkDescriptorSetLayout& descriptorSetLayout, VkRenderPass& renderPass,
		const char* vertShaderFile, const char* fragShaderFile, const std::string name,
		VkPipeline& graphicsPipeline);

	GraphicsPipeline();

	~GraphicsPipeline();

private:
	VkDevice device;
	std::string name;
};
