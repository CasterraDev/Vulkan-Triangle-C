#include "core/asserts.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "device.h"
#include "math/fsnmath.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "vulkan/vulkan_core.h"
#include "vulkanSwapchain.h"

b8 vulkanSwapchainCreate(VulkanInfo* header, u32 width, u32 height,
                         VulkanSwapchain* outSwapchain) {

    VulkanSwapchainSupportInfo swapchainInfo = header->device.swapchainSupport;

    VkSurfaceFormatKHR format = swapchainInfo.formats[0];
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D swapExtent = {width, height};

    // Get the format
    for (u8 i = 0; i < swapchainInfo.formatCnt; i++) {
        FDEBUG("Format: %d vs %d, Colorspace: %d vs %d", swapchainInfo.formats[i].format, VK_FORMAT_B8G8R8A8_UNORM,
               swapchainInfo.formats[i].colorSpace, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
        if (swapchainInfo.formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
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
        FDEBUG("PresentMode: %d vs %d", swapchainInfo.presentModes[i], VK_PRESENT_MODE_MAILBOX_KHR);
        if (swapchainInfo.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = swapchainInfo.presentModes[i];
            FDEBUG("Swapchain present mode found: %d", presentMode);
            break;
        }
    }

    vulkanDeviceQuerySwapchainSupport(header->device.physicalDevice,
                                      header->surface,
                                      &header->device.swapchainSupport);

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

    u32 imageCnt = swapchainInfo.capabilities.minImageCount;

    // TODO: Look into this. Present me doesn't know why past me wrote it 
    // if (swapchainInfo.capabilities.maxImageCount > 0 &&
    //     imageCnt + 1 > swapchainInfo.capabilities.maxImageCount) {
    //     imageCnt = swapchainInfo.capabilities.maxImageCount;
    // }

    header->curFrame = 0;
    outSwapchain->maxNumOfFramesInFlight = imageCnt;

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

    header->curImageIdx = 0;

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

    FDEBUG("Image Count: %d", outSwapchain->imageCnt);
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

    if (!vulkanDeviceDetectDepthFormat(&header->device)) {
        FFATAL("Failed to get device depth format.");
        return false;
    }

    return true;
}

b8 vulkanSwapchainRecreate(VulkanInfo* header, u32 width, u32 height,
                           VulkanSwapchain* swapchain) {
    // TODO: Make this function actually return false if it fails
    vulkanSwapchainDestroy(header, swapchain);
    vulkanSwapchainCreate(header, width, height, swapchain);
    return true;
}

void vulkanSwapchainDestroy(VulkanInfo* header, VulkanSwapchain* swapchain) {
    vkDeviceWaitIdle(header->device.device);

    for (u32 i = 0; i < swapchain->imageCnt; ++i) {
        vkDestroyImageView(header->device.device, header->swapchain.views[i],
                           header->allocator);
    }

    vkDestroySwapchainKHR(header->device.device, header->swapchain.handle,
                          header->allocator);
}

b8 vulkanSwapchainGetNextImgIdx(VulkanInfo* header, VulkanSwapchain* swapchain,
                                u64 timeoutNS, VkSemaphore imgAvailSemaphore,
                                VkFence fence, u32* outImgIdx) {
    vkAcquireNextImageKHR(header->device.device, swapchain->handle, timeoutNS,
                          imgAvailSemaphore, fence, outImgIdx);
    return true;
}

void vulkanSwapchainPresent(VulkanInfo* header, VulkanSwapchain* swapchain,
                            VkQueue graphicsQueue, VkQueue presentQueue,
                            VkSemaphore renderCompleteSemaphore,
                            u32 presentImgIdx) {
    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain->handle;
    presentInfo.pImageIndices = &presentImgIdx;
    presentInfo.pResults = 0;

    VkResult res = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        vulkanSwapchainRecreate(header, header->framebufferWidth,
                                header->framebufferHeight, swapchain);
    } else if (res != VK_SUCCESS) {
        FFATAL("Failed to acquire next swapchain image idx");
    }

    header->curFrame =
        (header->curFrame + 1) % swapchain->maxNumOfFramesInFlight;
}
