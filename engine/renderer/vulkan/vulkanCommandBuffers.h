#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "vulkan/vulkan_core.h"

void vulkanCommandBufferAllocate(VulkanInfo* vi, VkCommandPool comPool, b8 isPrimaryLevel, VulkanCommandBuffer* outCommandBuffer);
void vulkanCommandBufferBegin(VulkanCommandBuffer* commandBuffer, VkCommandBufferUsageFlags usageFlags);
void vulkanCommandBufferEnd(VulkanCommandBuffer* commandBuffer);
void vulkanCommandBufferReset(VulkanCommandBuffer* commandBuffer);
void vulkanCommandBufferUpdateSubmitted(VulkanCommandBuffer* commandBuffer);

