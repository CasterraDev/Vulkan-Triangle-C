#pragma once

#include "core/asserts.h"
#include "defines.h"
#include "math/matrixMath.h"
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

typedef enum VulkanRenderpassState {
    VULKAN_RENDERPASS_READY,
    VULKAN_RENDERPASS_RECORDING,
    VULKAN_RENDERPASS_IN_RENDER_PASS,
    VULKAN_RENDERPASS_RECORDING_ENDED,
    VULKAN_RENDERPASS_SUBMITTED,
    VULKAN_RENDERPASS_NOT_ALLOCATED
} VulkanRenderpassState;

typedef struct vulkanRenderpass {
    VkRenderPass handle;
    vector4 renderArea;
    vector4 clearColor;

    f32 depth;
    u32 stencil;

    u8 clearFlags;

    b8 hasPrevPass;
    b8 hasNextPass;

    VulkanRenderpassState state;
} VulkanRenderpass;

typedef struct VulkanSwapchain {
    VkSurfaceFormatKHR imgFormat;
    u8 maxNumOfFramesInFlight;
    VkSwapchainKHR handle;
    u32 imageCnt;
    VkImage* images;
    VkImageView* views;
    VulkanImage depthAttachment;

    // Framebuffers used for on-screen rendering, one per frame
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

typedef enum VulkanCommandBufferState {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED,
    COMMAND_BUFFER_STATE_SUBMITTED
} VulkanCommandBufferState;

typedef struct VulkanCommandBuffer {
    VkCommandBuffer handle;
    const char* name;
    VulkanCommandBufferState state;
} VulkanCommandBuffer;

//==================== Shaders ====================

typedef struct VulkanShaderStage {
    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo;
    VkPipelineShaderStageCreateInfo stageCreateInfo;
} VulkanShaderStage;

typedef struct VulkanPipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
} VulkanPipeline;

typedef struct VulkanShader {
    /* Dino */
    VulkanShaderStage* stages;

    VkDescriptorPool descriptorPool;
    /** Idx global=0 and instance=1 **/
    VkDescriptorSetLayout descriptorSetLayouts[2];
    /** One per frame **/
    VkDescriptorSet globalDescriptorSets[3];

    VulkanPipeline pipeline;
} VulkanShader;

//==================== VulkanInfo ====================

typedef struct VulkanInfo {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    u32 deltaTime;
    VkDebugUtilsMessengerEXT debugMessenger;
    VulkanDevice device;
    VkSurfaceKHR surface;
    VulkanSwapchain swapchain;
    u32 framebufferWidth;
    u32 framebufferHeight;
    VulkanRenderpass renderpass;
    // Dino
    VulkanCommandBuffer* graphicsCommandBuffers;
    // Current idx of swapchain image. Used for command buffers, etc 
    u32 curImageIdx;
    u32 curFrame;

    // Dino
    VkSemaphore* imageAvailableSemaphores;
    // Dino
    VkSemaphore* renderFinishedSemaphores;
    // Dino
    VkFence* inFlightFences;
    VkFence* imagesInFlight[3];

    b8 recreatingSwapchain;
    u32 framebufferSizeGen;
    u32 framebufferSizeGenLast;
} VulkanInfo;
