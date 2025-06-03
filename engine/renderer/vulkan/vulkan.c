#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "helpers/dinoarray.h"
#include "math/matrixMath.h"
#include "renderer/renderTypes.h"
#include "renderer/vulkan/device.h"
#include "renderer/vulkan/utils.h"
#include "renderer/vulkan/vulkanCommandBuffers.h"
#include "renderer/vulkan/vulkanPlatform.h"
#include "renderer/vulkan/vulkanRenderpass.h"
#include "renderer/vulkan/vulkanShader.h"
#include "renderer/vulkan/vulkanSwapchain.h"
#include "systems/shaderSystem.h"
#include "vulkan/vulkan_core.h"
#include "vulkanTypes.h"
#include <stdint.h>

static VulkanInfo header;

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData) {
    switch (messageSeverity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            FERROR(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            FWARN(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            FINFO(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            FTRACE(pCallbackData->pMessage);
            break;
    };
    return VK_FALSE;
}

VkResult createDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

b8 vulkanInit(rendererBackend* backend, const char* appName, u64 appWidth,
              u64 appHeight) {
    VkApplicationInfo vkheaderInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
    };

    VkInstanceCreateInfo vkCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &vkheaderInfo,
    };

    FDEBUG("Width/Height: %d/%d", appWidth, appHeight);
    header.framebufferWidth = appWidth;
    header.framebufferHeight = appHeight;
    FDEBUG("Width/Height: %d/%d", header.framebufferWidth,
           header.framebufferHeight);

    //--------Vulkan Extensions--------
    const char** requiredExts = dinoCreate(const char**);
    dinoPush(requiredExts, &VK_KHR_SURFACE_EXTENSION_NAME);
    platformGetRequiredExts(&requiredExts);

    // Debug vulkan extensions
#if defined(_DEBUG)
    dinoPush(requiredExts, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    FDEBUG("Required extensions:");
    u32 length = dinoLength(requiredExts);
    for (u32 i = 0; i < length; ++i) {
        FDEBUG(requiredExts[i]);
    }
#endif
    // Validation Layers
    const char** requiredValLayers = 0;
    u32 requiredValLayersCnt = 0;

#if defined(_DEBUG)
    requiredValLayers = dinoCreate(const char**);
    dinoPush(requiredValLayers, &"VK_LAYER_KHRONOS_validation");
    requiredValLayersCnt = dinoLength(requiredValLayers);

    // Obtain a list of available validation layers
    u32 availLayerCnt = 0;
    vkEnumerateInstanceLayerProperties(&availLayerCnt, 0);
    VkLayerProperties* availLayers =
        dinoCreateReserve(availLayerCnt, VkLayerProperties);
    vkEnumerateInstanceLayerProperties(&availLayerCnt, availLayers);

    // Verify all required layers are available.
    for (u32 i = 0; i < requiredValLayersCnt; ++i) {
        FINFO("Searching for layer: %s", requiredValLayers[i]);
        b8 found = false;
        for (u32 j = 0; j < availLayerCnt; ++j) {
            if (strEqual(requiredValLayers[i], availLayers[j].layerName)) {
                found = true;
                FINFO("Found.");
                break;
            }
        }

        if (!found) {
            FFATAL("Required validation layer is missing: %s",
                   requiredValLayers[i]);
            return false;
        }
    }
    dinoDestroy(availLayers);
    FINFO("All required validation layers are present.");

    // Setup up instance debugger
    VkDebugUtilsMessengerCreateInfoEXT debugInstanceCreateInfo = {};
    debugInstanceCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInstanceCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInstanceCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInstanceCreateInfo.pfnUserCallback = debugCallback;
    vkCreateInfo.pNext =
        (VkDebugUtilsMessengerCreateInfoEXT*)&debugInstanceCreateInfo;
#endif //_Debug

    vkCreateInfo.enabledLayerCount = requiredValLayersCnt;
    vkCreateInfo.ppEnabledLayerNames = requiredValLayers;
    vkCreateInfo.enabledExtensionCount = dinoLength(requiredExts);
    vkCreateInfo.ppEnabledExtensionNames = requiredExts;

    VkResult result = vkCreateInstance(&vkCreateInfo, 0, &header.instance);

    if (result != VK_SUCCESS) {
        FERROR("Failed to create vk instance.\n");
    }

#if defined(_DEBUG)
    FDEBUG("Creating Vulkan debugger...");
    u32 wantedLogSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugCreateInfo.messageSeverity = wantedLogSeverity;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    if (createDebugUtilsMessengerEXT(header.instance, &debugCreateInfo,
                                     header.allocator,
                                     &header.debugMessenger) != VK_SUCCESS) {
        FFATAL("Failed to setup debug messenger.");
    }
    FDEBUG("Vulkan debugger created.");
#endif

    if (!platformCreateVulkanSurface(&header)) {
        FERROR("Failed to create surface");
        return false;
    }

    if (!vulkanDeviceGetCreate(&header)) {
        FERROR("Failed to create surface");
        return false;
    }

    if (!vulkanSwapchainCreate(&header, appWidth, appHeight,
                               &header.swapchain)) {
        FERROR("Failed to create swapchain");
        return false;
    }

    VulkanRenderpassConfig rc;
    rc.renderArea =
        (vector4){0, 0, header.framebufferWidth, header.framebufferHeight};
    rc.clearColor = (vector4){0, 0, 0.2f, 1.0f};
    rc.depth = 1.0f;
    rc.stencil = 0;
    rc.hasPrevPass = false;
    rc.hasNextPass = false;

    if (!vulkanRenderpassCreate(header, rc, &header.renderpass)) {
        FERROR("Failed to create renderpass");
        return false;
    }

    // Create framebuffers
    for (u32 i = 0; i < 3; i++) {
        VkImageView att[1] = {header.swapchain.views[i]};
        VkFramebufferCreateInfo fci;
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = header.renderpass.handle;
        fci.attachmentCount = 1;
        fci.pAttachments = att;
        fci.width = header.framebufferWidth;
        fci.height = header.framebufferHeight;
        fci.layers = 1;

        VK_CHECK(vkCreateFramebuffer(header.device.device, &fci,
                                     header.allocator,
                                     &header.swapchain.framebuffers[i]))
    }
    FDEBUG("Created Framebuffers");

    if (!header.graphicsCommandBuffers) {
        header.graphicsCommandBuffers =
            dinoCreateReserve(header.swapchain.imageCnt, VulkanCommandBuffer);
        dinoClear(&header.graphicsCommandBuffers);
    }

    for (u32 i = 0; i < header.swapchain.imageCnt; i++) {
        if (header.graphicsCommandBuffers[i].handle) {
            vkFreeCommandBuffers(header.device.device,
                                 header.device.graphicsCommandPool, 1,
                                 &header.graphicsCommandBuffers[i].handle);
        }

        dinoClear(&header.graphicsCommandBuffers);

        vulkanCommandBufferAllocate(&header, header.device.graphicsCommandPool,
                                    true, &header.graphicsCommandBuffers[i]);
    }
    FDEBUG("Created Command Buffers");

    VkSemaphoreCreateInfo semaInfo;
    semaInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    header.imageAvailableSemaphores =
        dinoCreateReserveWithLengthSet(header.swapchain.imageCnt, VkSemaphore);
    header.renderFinishedSemaphores =
        dinoCreateReserveWithLengthSet(header.swapchain.imageCnt, VkSemaphore);
    header.inFlightFences =
        dinoCreateReserveWithLengthSet(header.swapchain.imageCnt, VkFence);

    for (u32 i = 0; i < header.swapchain.imageCnt; i++) {
        header.imagesInFlight[i] = 0;
    }

    for (u32 i = 0; i < header.swapchain.imageCnt; i++) {
        vkCreateSemaphore(header.device.device, &semaInfo, header.allocator,
                          &header.imageAvailableSemaphores[i]);
        vkCreateSemaphore(header.device.device, &semaInfo, header.allocator,
                          &header.renderFinishedSemaphores[i]);
        vkCreateFence(header.device.device, &fenceInfo, header.allocator,
                      &header.inFlightFences[i]);
    }

    header.framebufferWidth = appWidth;
    header.framebufferHeight = appHeight;

    if (!vulkanShaderInit(&header)) {
        FERROR("Failed to init shader");
        return false;
    }

    FDEBUG("Shader Inited.");

    return true;
}

void vulkanShutdown(rendererBackend* backend) {
    FINFO("Shutting down Vulkan.");
    vkDeviceWaitIdle(header.device.device);

    vulkanShaderShutdown(&header);

    for (u32 i = 0; i < header.swapchain.imageCnt; i++) {
        vkDestroySemaphore(header.device.device,
                           header.imageAvailableSemaphores[i],
                           header.allocator);
        vkDestroySemaphore(header.device.device,
                           header.renderFinishedSemaphores[i],
                           header.allocator);
        vkDestroyFence(header.device.device, header.inFlightFences[i],
                       header.allocator);

        if (header.graphicsCommandBuffers[i].handle) {
            vkFreeCommandBuffers(header.device.device,
                                 header.device.graphicsCommandPool, 1,
                                 &header.graphicsCommandBuffers[i].handle);
        }
    }

    for (u32 i = 0; i < 3; i++) {
        vkDestroyFramebuffer(header.device.device,
                             header.swapchain.framebuffers[i],
                             header.allocator);
    }

    vulkanRenderpassDestroy(&header, &header.renderpass);

#if defined(_DEBUG)
    if (header.debugMessenger) {
        FDEBUG("Destroying Vulkan debugger...");
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                header.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(header.instance, header.debugMessenger, header.allocator);
    }
#endif

    vulkanDeviceDestroy(&header);

    vkDestroySurfaceKHR(header.instance, header.surface, header.allocator);
    vkDestroyInstance(header.instance, header.allocator);
}

b8 vulkanDraw() {

    vkCmdDraw(header.graphicsCommandBuffers[header.curImageIdx].handle, 3, 1, 0,
              0);

    return true;
}

b8 vulkanBeginFrame(struct rendererBackend* backend, f32 deltaTime) {
    header.deltaTime = deltaTime;
    VulkanDevice* dev = &header.device;
    if (header.recreatingSwapchain) {
        VkResult res = vkDeviceWaitIdle(dev->device);
        if (!successfullVulkanResult(res)) {
            FERROR("Vulkan Renderer vkDeviceWaitIdle failed: %s",
                   vulkanResultStr(res, true));
            return false;
        }
        FINFO("Recreating swapchain... Booting...")
        return false;
    }

    // Means the window has most likely been resized
    if (header.framebufferSizeGen != header.framebufferSizeGenLast) {
        VkResult res = vkDeviceWaitIdle(dev->device);
        if (!successfullVulkanResult(res)) {
            FERROR("Vulkan Renderer vkDeviceWaitIdle failed: %s",
                   vulkanResultStr(res, true));
            return false;
        }

        if (!vulkanSwapchainRecreate(&header, header.framebufferWidth,
                                     header.framebufferHeight,
                                     &header.swapchain)) {
            return false;
        }

        FINFO("Resized. Booting...");
        return false;
    }

    // Wait for fences
    VkResult res =
        vkWaitForFences(dev->device, 1, &header.inFlightFences[header.curFrame],
                        true, UINT64_MAX);
    // Get next swapchain image idx
    if (!vulkanSwapchainGetNextImgIdx(
            &header, &header.swapchain, UINT64_MAX,
            header.imageAvailableSemaphores[header.curFrame], 0,
            &header.curImageIdx)) {
        FERROR("Failed to get next image idx");
        return false;
    }

    // Begin recording the command buffer
    VulkanCommandBuffer* cb =
        &header.graphicsCommandBuffers[header.curImageIdx];
    vulkanCommandBufferReset(cb);
    vulkanCommandBufferBegin(cb);

    // Set viewport/scissor

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = header.framebufferWidth;
    viewport.height = header.framebufferHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(header.graphicsCommandBuffers[header.curImageIdx].handle,
                     0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent =
        (VkExtent2D){header.framebufferWidth, header.framebufferHeight};
    vkCmdSetScissor(header.graphicsCommandBuffers[header.curImageIdx].handle, 0,
                    1, &scissor);
    return true;
}

b8 vulkanEndFrame(struct rendererBackend* backend, f32 deltaTime) {
    VulkanCommandBuffer* cb =
        &header.graphicsCommandBuffers[header.curImageIdx];

    vulkanCommandBufferEnd(cb);

    if (header.imagesInFlight[header.curImageIdx] != VK_NULL_HANDLE) {
        VkResult res = vkWaitForFences(header.device.device, 1,
                                       &header.inFlightFences[header.curFrame],
                                       true, UINT64_MAX);
        if (!successfullVulkanResult(res)) {
            FERROR("Vulkan Renderer vkWaitForFences failed: %s",
                   vulkanResultStr(res, true));
            return false;
        }
    }

    header.imagesInFlight[header.curImageIdx] =
        &header.inFlightFences[header.curFrame];

    VK_CHECK(vkResetFences(header.device.device, 1,
                           &header.inFlightFences[header.curFrame]));

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cb->handle;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores =
        &header.renderFinishedSemaphores[header.curFrame];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores =
        &header.imageAvailableSemaphores[header.curFrame];

    VkPipelineStageFlags flags[1] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = flags;

    VkResult res = vkQueueSubmit(header.device.graphicsQueue, 1, &submitInfo,
                                 header.inFlightFences[header.curFrame]);

    if (!successfullVulkanResult(res)) {
        FERROR("Vulkan Renderer vkQueueSubmit failed: %s",
               vulkanResultStr(res, true));
        return false;
    }

    vulkanCommandBufferUpdateSubmitted(cb);

    vulkanSwapchainPresent(
        &header, &header.swapchain, header.device.graphicsQueue,
        header.device.presentQueue,
        header.renderFinishedSemaphores[header.curFrame], header.curImageIdx);

    return true;
}

b8 vulkanBeginRenderpass(struct rendererBackend* backend, u8 renderpassId) {
    vulkanRenderpassBegin(
        &header, header.graphicsCommandBuffers[header.curImageIdx],
        &header.renderpass, header.swapchain.framebuffers[header.curImageIdx]);

    // TODO: Temp stuff
    Shader* s = shaderGet("FirstShader");
    vulkanShaderUse(s);
    return true;
}

b8 vulkanEndRenderpass(struct rendererBackend* backend, u8 renderpassId) {
    vulkanRenderpassEnd(header.graphicsCommandBuffers[header.curImageIdx],
                        &header.renderpass);
    return true;
}
