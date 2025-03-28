#pragma once

#include "defines.h"
#include "vulkan/vulkan_core.h"

// Checks the given expression's return value is OK.
#define VK_CHECK(expr)                                                         \
    {                                                                          \
        FASSERT(expr == VK_SUCCESS);                                           \
    }

typedef struct VulkanImage {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
    VkSampler sampler;
} VulkanImage;

typedef struct VulkanSwapchain {
    VkSurfaceFormatKHR imgFormat;
    u8 maxNumOfFramesInFlight;
    VkSwapchainKHR handle;
    u32 imageCnt;
    VkImage* images;
    VkImageView* views;
    VulkanImage depthAttachment;

    //Framebuffers used for on-screen rendering, one per frame
    VkFramebuffer framebuffers[3];
} VulkanSwapchain;

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
    VkSurfaceKHR surface;
    VulkanSwapchain swapchain;
    u32 currentFrame;

} VulkanInfo;
