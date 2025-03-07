#pragma once

#include "defines.h"
#include "vulkan/vulkan_core.h"

typedef struct VulkanSwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 formatCnt;
    VkSurfaceFormatKHR* formats;
    u32 presentModeCnt;
    VkPresentModeKHR* presentModes;
} VulkanSwapchainSupportInfo;

typedef struct VulkanDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VulkanSwapchainSupportInfo swapchainSupport;
    i32 graphicsQueueIdx;
    i32 presentQueueIdx;
    i32 computeQueueIdx;
    i32 transferQueueIdx;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;

    VkCommandPool graphicsCommandPool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depthFormat;
} VulkanDevice;

typedef struct VulkanInfo {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkDebugUtilsMessengerEXT debugMessenger;
    VulkanDevice device;

} VulkanInfo;
