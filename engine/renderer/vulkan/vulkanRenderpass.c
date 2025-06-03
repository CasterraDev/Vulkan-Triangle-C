#include "vulkanRenderpass.h"
#include "helpers/dinoarray.h"
#include "renderer/vulkan/vulkanCommandBuffers.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "vulkan/vulkan_core.h"

b8 vulkanRenderpassCreate(VulkanInfo vi,
                          VulkanRenderpassConfig renderpassConfig,
                          VulkanRenderpass* outRenderpass) {
    outRenderpass->clearColor = renderpassConfig.clearColor;
    outRenderpass->clearFlags = renderpassConfig.clearFlags;
    outRenderpass->renderArea = renderpassConfig.renderArea;
    outRenderpass->hasPrevPass = renderpassConfig.hasPrevPass;
    outRenderpass->hasNextPass = renderpassConfig.hasNextPass;
    outRenderpass->depth = renderpassConfig.depth;
    outRenderpass->stencil = renderpassConfig.stencil;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkAttachmentDescription* attachmentDescriptions =
        dinoCreateReserve(2, VkAttachmentDescription);

    VkAttachmentDescription colorAttachment;
    colorAttachment.format = vi.swapchain.imgFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout =
        renderpassConfig.hasPrevPass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                     : VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = renderpassConfig.hasNextPass
                                      ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                      : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.flags = 0;

    dinoPush(attachmentDescriptions, colorAttachment);

    VkAttachmentReference car;
    car.attachment = 0;
    car.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &car;

    // TODO: Depth testing
    subpass.pDepthStencilAttachment = 0;

    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = 0;

    subpass.pResolveAttachments = 0;

    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = 0;

    VkSubpassDependency dep;
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dependencyFlags = 0;

    VkRenderPassCreateInfo rpci;
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = dinoLength(attachmentDescriptions);
    rpci.pAttachments = attachmentDescriptions;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;
    rpci.pNext = 0;
    rpci.flags = 0;

    VK_CHECK(vkCreateRenderPass(vi.device.device, &rpci, vi.allocator,
                                &outRenderpass->handle))

    return true;
}

void vulkanRenderpassDestroy(VulkanInfo* vi, VulkanRenderpass* renderpass){
    if (renderpass && renderpass->handle){
        vkDestroyRenderPass(vi->device.device, renderpass->handle, vi->allocator);
        renderpass->handle = 0;
    }
}

void vulkanRenderpassBegin(VulkanInfo* vi, VulkanCommandBuffer commandBuffer, VulkanRenderpass* renderpass, VkFramebuffer framebuffer){
    VkRenderPassBeginInfo renderpassInfo;
    renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpassInfo.renderPass = renderpass->handle;
    renderpassInfo.framebuffer = framebuffer;
    renderpassInfo.clearValueCount = 1;
    VkClearValue clearColor = {0.0f,1.0f,0.0f,1.0f};
    renderpassInfo.renderArea.offset.x = renderpass->renderArea.x;
    renderpassInfo.renderArea.offset.y = renderpass->renderArea.y;
    renderpassInfo.renderArea.extent.width = renderpass->renderArea.z;
    renderpassInfo.renderArea.extent.height = renderpass->renderArea.w;
    renderpassInfo.pClearValues = &clearColor;
    renderpassInfo.pNext = 0;

    vkCmdBeginRenderPass(commandBuffer.handle, &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void vulkanRenderpassEnd(VulkanCommandBuffer commandBuffer, VulkanRenderpass* renderpass){
    vkCmdEndRenderPass(commandBuffer.handle);
}
