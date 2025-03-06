#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "defines.h"
#include "helpers/dinoarray.h"
#include "platform/platform.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
} App;

static App* app;

int main(void) {
    printf("Hello There.\n");
    app = (App*)malloc(sizeof(App));

    memorySystemSettings memorySettings;
    memorySettings.totalSize = GIGABYTES(1);
    memoryInit(memorySettings);

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

    platformSleep(2000);

    platformShutdown();
    memoryShutdown();
    return 0;
}
