#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "defines.h"
#include "helpers/dinoarray.h"
#include "platform/platform.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <stdio.h>

// TODO: Move to renderer's abstraction layer
// Checks the given expression's return value is OK.
#define VK_CHECK(expr)                                                         \
    {                                                                          \
        FFATAL(expr == VK_SUCCESS);                                            \
    }

typedef struct App {
    u64 platformMemReq;
    void* platformState;

    // TODO: Move this to an abstraction layer
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkDebugUtilsMessengerEXT debugMessenger;
} App;

static App* app;

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
    if (func != 0) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != 0) {
        func(instance, debugMessenger, pAllocator);
    }
}

int main(void) {
    printf("Hello There.\n");

    memorySystemSettings memorySettings;
    memorySettings.totalSize = GIGABYTES(1);
    memoryInit(memorySettings);

    app = fallocate(sizeof(App), MEMORY_TAG_APPLICATION);

    app->allocator = 0;

    app->platformMemReq = 0;
    platformStartup(&app->platformMemReq, 0, "Triangle", 0, 0, 1280, 720);
    app->platformState = fallocate(app->platformMemReq, MEMORY_TAG_APPLICATION);
    platformStartup(&app->platformMemReq, app->platformState, "Triangle", 0, 0,
                    1280, 720);

    VkApplicationInfo vkAppInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
    };

    VkInstanceCreateInfo vkCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &vkAppInfo,
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
#endif //_Debug

    vkCreateInfo.enabledLayerCount = requiredValLayersCnt;
    vkCreateInfo.ppEnabledLayerNames = requiredValLayers;
    vkCreateInfo.enabledExtensionCount = dinoLength(requiredExts);
    vkCreateInfo.ppEnabledExtensionNames = requiredExts;

    VkResult result = vkCreateInstance(&vkCreateInfo, 0, &app->instance);

    if (result != VK_SUCCESS) {
        FERROR("Failed to create vk instance.\n");
    }

#if defined(_DEBUG)
    FDEBUG("Creating Vulkan debugger...");
    u32 wantedLogSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; //|
                                                      //    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerEXT debugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugCreateInfo.messageSeverity = wantedLogSeverity;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    if (createDebugUtilsMessengerEXT(app->instance, &debugCreateInfo, app->allocator, &debugMessenger)){
        FFATAL("Failed to setup debug messenger.");
    }
    FDEBUG("Vulkan debugger created.");
#endif

    platformSleep(2000);

#if defined(_DEBUG)
    destroyDebugUtilsMessengerEXT(app->instance, debugMessenger, app->allocator);
#endif // _Debug
    platformShutdown();
    memoryShutdown();
    return 0;
}
