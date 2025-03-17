#include "core/asserts.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "math/fsnmath.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "vulkan/vulkan_core.h"
#include "device.h"
#include "vulkanSwapchain.h"

void vulkanSwapchainCreate(VulkanInfo* header, u32 width, u32 height,
                           VulkanSwapchain* outSwapchain) {
    VulkanSwapchainSupportInfo swapchainInfo = header->device.swapchainSupport;

    VkSurfaceFormatKHR format = swapchainInfo.formats[0];
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D swapExtent = {width, height};

    // Get the format
    for (u8 i = 0; i < swapchainInfo.formatCnt; i++) {
        if (swapchainInfo.formats[i].format == VK_FORMAT_B8G8R8_SRGB &&
            swapchainInfo.formats[i].colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format = swapchainInfo.formats[i];
            FDEBUG("Swapchain format found: %d:%d", format.format,
                   format.colorSpace);
            break;
        }
    }
    outSwapchain->imgFormat = format;

    // Get present mode
    for (u8 i = 0; i < swapchainInfo.presentModeCnt; i++) {
        if (swapchainInfo.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = swapchainInfo.presentModes[i];
            FDEBUG("Swapchain present mode found: %d", presentMode);
            break;
        }
    }

    if (swapchainInfo.capabilities.currentExtent.width != UINT32_MAX) {
        swapExtent = swapchainInfo.capabilities.currentExtent;
    } else {
        swapExtent.width = FCLAMP(
            swapExtent.width, swapchainInfo.capabilities.minImageExtent.width,
            swapchainInfo.capabilities.maxImageExtent.width);
        swapExtent.height = FCLAMP(
            swapExtent.height, swapchainInfo.capabilities.minImageExtent.height,
            swapchainInfo.capabilities.maxImageExtent.height);
    }

    u32 imageCnt = swapchainInfo.capabilities.minImageCount + 1;

    if (swapchainInfo.capabilities.maxImageCount > 0 &&
        imageCnt > swapchainInfo.capabilities.maxImageCount) {
        imageCnt = swapchainInfo.capabilities.maxImageCount;
    }

    outSwapchain->maxNumOfFramesInFlight = imageCnt - 1;

    VkSwapchainCreateInfoKHR swapCI;
    swapCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapCI.surface = header->surface;
    swapCI.minImageCount = imageCnt;
    swapCI.imageFormat = format.format;
    swapCI.imageColorSpace = format.colorSpace;
    swapCI.imageExtent = swapExtent;
    swapCI.imageArrayLayers = 1;
    swapCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (header->device.graphicsQueueIdx != header->device.presentQueueIdx) {
        swapCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        u32 queueFamilyIndices[] = {(u32)header->device.graphicsQueueIdx,
                                    (u32)header->device.presentQueueIdx};
        swapCI.queueFamilyIndexCount = 2;
        swapCI.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapCI.queueFamilyIndexCount = 0;
        swapCI.pQueueFamilyIndices = 0;
    }

    swapCI.preTransform = swapchainInfo.capabilities.currentTransform;
    swapCI.presentMode = presentMode;
    swapCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapCI.clipped = VK_TRUE;
    swapCI.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(header->device.device, &swapCI,
                                  header->allocator, &outSwapchain->handle))

    header->currentFrame = 0;

    header->swapchain.imageCnt = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(header->device.device,
                                     outSwapchain->handle,
                                     &outSwapchain->imageCnt, 0));

    if (!outSwapchain->images) {
        outSwapchain->images = (VkImage*)fallocate(
            sizeof(VkImage) * outSwapchain->imageCnt, MEMORY_TAG_RENDERER);
    }

    if (!outSwapchain->views) {
        outSwapchain->views = (VkImageView*)fallocate(
            sizeof(VkImageView) * outSwapchain->imageCnt, MEMORY_TAG_RENDERER);
    }

    VK_CHECK(
        vkGetSwapchainImagesKHR(header->device.device, outSwapchain->handle,
                                &outSwapchain->imageCnt, outSwapchain->images));

    // Views
    for (u32 i = 0; i < outSwapchain->imageCnt; ++i) {
        VkImageViewCreateInfo viewInfo = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = outSwapchain->images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = outSwapchain->imgFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(header->device.device, &viewInfo,
                                   header->allocator, &outSwapchain->views[i]));
    }

    if (!vulkanDeviceDetectDepthFormat(&header->device)){
        FFATAL("Failed to get device depth format.");
    }

}

void vulkanSwapchainRecreate(VulkanInfo* header, u32 width, u32 height,
                             VulkanSwapchain* swapchain) {
}

void vulkanSwapchainDestroy(VulkanInfo* header, VulkanSwapchain* swapchain) {
    vkDeviceWaitIdle(header->device.device);

    for (u32 i = 0; i < swapchain->imageCnt; ++i) {
        vkDestroyImageView(header->device.device, header->swapchain.views[i], header->allocator);
    }

    vkDestroySwapchainKHR(header->device.device, header->swapchain.handle,
                          header->allocator);
}

b8 vulkanSwapchainGetNextImgIdx(VulkanInfo* header, VulkanSwapchain* swapchain,
                                u64 timeoutNS, VkSemaphore imgAvailSemaphore,
                                VkFence fence, u32* outImgIdx) {
    return true;
}

void vulkanSwapchainPresent(VulkanInfo* header, VulkanSwapchain* swapchain,
                            VkQueue graphicsQueue, VkQueue presentQueue,
                            VkSemaphore renderCompleteSemaphore,
                            u32 presentImgIdx) {
}
