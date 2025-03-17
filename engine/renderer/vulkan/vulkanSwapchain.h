#pragma once

#include "vulkanTypes.h"

void vulkanSwapchainCreate(VulkanInfo* header, u32 width, u32 height, VulkanSwapchain* outSwapchain);
void vulkanSwapchainRecreate(VulkanInfo* header, u32 width, u32 height, VulkanSwapchain* swapchain);
void vulkanSwapchainDestroy(VulkanInfo* header, VulkanSwapchain* swapchain);
b8 vulkanSwapchainGetNextImgIdx(VulkanInfo* header, VulkanSwapchain* swapchain, u64 timeoutNS, VkSemaphore imgAvailSemaphore, VkFence fence, u32* outImgIdx);
void vulkanSwapchainPresent(VulkanInfo* header, VulkanSwapchain* swapchain, VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 presentImgIdx);
