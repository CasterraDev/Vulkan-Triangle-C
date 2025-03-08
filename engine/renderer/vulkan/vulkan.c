#include "core/fstring.h"
#include "core/logger.h"
#include "helpers/dinoarray.h"
#include "renderer/renderTypes.h"
#include "renderer/vulkan/device.h"
#include "renderer/vulkan/vulkanPlatform.h"
#include "vulkan/vulkan_core.h"
#include "vulkanTypes.h"

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

    if (!getVulkanDevice(&header)) {
        FERROR("Failed to create surface");
        return false;
    }

    return true;
}

void vulkanShutdown(rendererBackend* backend) {
    FINFO("Shutting down Vulkan.");
    vkDeviceWaitIdle(header.device.device);
#if defined(_DEBUG)
    if (header.debugMessenger) {
        FDEBUG("Destroying Vulkan debugger...");
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                header.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(header.instance, header.debugMessenger, header.allocator);
    }
#endif

    vkDestroyCommandPool(header.device.device,
                         header.device.graphicsCommandPool, header.allocator);
    vkDestroyDevice(header.device.device, header.allocator);
    header.device.physicalDevice = 0;

    vkDestroySurfaceKHR(header.instance, header.surface, header.allocator);
    vkDestroyInstance(header.instance, header.allocator);
}
