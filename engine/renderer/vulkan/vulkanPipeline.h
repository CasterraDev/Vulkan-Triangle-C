#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "vulkan/vulkan_core.h"

typedef struct VulkanPipelineConfig {
    VulkanRenderpass* renderpass;
    u32 stageCnt;
    VkPipelineShaderStageCreateInfo* stages;
    VkViewport viewport;
    VkRect2D scissor;
    b8 isWireframe;
    b8 depthTested;
    VkVertexInputAttributeDescription* attributes;
} VulkanPipelineConfig;

b8 vulkanPipelineCreate(VulkanInfo* vi, VulkanPipelineConfig vpc, VulkanPipeline* outPipeline);
b8 vulkanPipelineDestroy(VulkanInfo* vi, VulkanPipeline* pipeline);
void vulkanPipelineBind(VkCommandBuffer cb, VkPipelineBindPoint bindPoint, VkPipeline pipeline);
