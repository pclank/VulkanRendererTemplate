#include <RenderPass.hpp>

RenderPass::RenderPass()
{

}

RenderPass::RenderPass(VkDevice device, VkPipelineBindPoint bindPoint,
    const std::vector<VkFormat>& formats,
    const std::vector<VkSampleCountFlagBits>& samples,
	const std::vector<VkAttachmentLoadOp>& loadOps,
	const std::vector<VkAttachmentStoreOp>& storeOps,
	const std::vector<VkImageLayout>& layouts, const std::string& name, VkRenderPass& renderPass)
	:
	device(device), name(name)
{
    assert("all vector sizes must match!", formats.size() == samples.size() && formats.size() == loadOps.size() &&
        formats.size() == storeOps.size() && formats.size() == layouts.size());

    // Color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = formats[0];
    colorAttachment.samples = samples[0];

    /*colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;*/
    colorAttachment.loadOp = loadOps[0];
    colorAttachment.storeOp = storeOps[0];

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = layouts[0];

    // Depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = formats[1];
    depthAttachment.samples = samples[1];

    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    /*depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;*/
    depthAttachment.stencilLoadOp = loadOps[1];
    depthAttachment.stencilStoreOp = storeOps[1];

    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = layouts[1];

    // Color resolve attachments
    VkAttachmentDescription colorResolveAttachment{};
    colorResolveAttachment.format = formats[2];
    colorResolveAttachment.samples = samples[2];

    /*colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;*/
    colorResolveAttachment.loadOp = loadOps[2];
    colorResolveAttachment.storeOp = storeOps[2];

    colorResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorResolveAttachment.finalLayout = layouts[2];

    // Subpasses
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorResolveAttachmentRef{};
    colorResolveAttachmentRef.attachment = 2;
    colorResolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = bindPoint;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorResolveAttachmentRef;

    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorResolveAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    // Subpass dependancy
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("failed to create render pass!");

    std::cout << "Created render pass: " << name << std::endl;
    RenderPass::renderPass = &renderPass;
}

RenderPass::~RenderPass()
{}

void RenderPass::Destroy() const
{
    vkDestroyRenderPass(device, *renderPass, nullptr);
}