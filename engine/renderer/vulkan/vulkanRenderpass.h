#pragma once

#include "math/matrixMath.h"
#include "renderer/vulkan/vulkanTypes.h"

typedef struct VulkanRenderpassConfig {
    vector4 renderArea;
    vector4 clearColor;
    f32 depth;
    u32 stencil;
    u8 clearFlags;
    b8 hasPrevPass;
    b8 hasNextPass;
} VulkanRenderpassConfig;

b8 vulkanRenderpassCreate(VulkanInfo vi,
                          VulkanRenderpassConfig renderpassConfig,
                          VulkanRenderpass* outRenderpass);

void vulkanRenderpassDestroy(VulkanInfo* vi, VulkanRenderpass* renderpass);

void vulkanRenderpassBegin(VulkanInfo* vi, VulkanCommandBuffer commandBuffer, VulkanRenderpass* renderpass, VkFramebuffer framebuffer);
void vulkanRenderpassEnd(VulkanCommandBuffer commandBuffer, VulkanRenderpass* renderpass);
