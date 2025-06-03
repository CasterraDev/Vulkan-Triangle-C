#include "device.h"
#include "core/asserts.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "helpers/dinoarray.h"
#include "renderer/vulkan/vulkanSwapchain.h"
#include "vulkan/vulkan_core.h"

typedef struct deviceRequirements {
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;
    // Dino array
    const char** deviceExts;
    b8 samplerAnisotropy;
    b8 discreteGPU;
} deviceRequirements;

typedef struct deviceQueueFamilyInfo {
    u32 graphicsFamilyIdx;
    u32 presentFamilyIdx;
    u32 computeFamilyIdx;
    u32 transferFamilyIdx;
} deviceQueueFamilyInfo;

void vulkanDeviceQuerySwapchainSupport(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    VulkanSwapchainSupportInfo* outSupportInfo) {
    // Surface capabilities
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice, surface, &outSupportInfo->capabilities));
    // Surface formats
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice, surface, &outSupportInfo->formatCnt, 0));
    if (outSupportInfo->formatCnt != 0) {
        if (!outSupportInfo->formats) {
            outSupportInfo->formats = fallocate(sizeof(VkSurfaceFormatKHR) *
                                                    outSupportInfo->formatCnt,
                                                MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &outSupportInfo->formatCnt,
            outSupportInfo->formats));
    }
    // Present modes
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, surface, &outSupportInfo->presentModeCnt, 0));
    if (outSupportInfo->presentModeCnt != 0) {
        if (!outSupportInfo->presentModes) {
            outSupportInfo->presentModes = fallocate(
                sizeof(VkPresentModeKHR) * outSupportInfo->presentModeCnt,
                MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, surface, &outSupportInfo->presentModeCnt,
            outSupportInfo->presentModes));
    }
    FDEBUG("Queried for swapchain support.");
}

b8 createVulkanLogicalDevice(VulkanInfo* header) {
    FINFO("Creating logical device...");
    b8 isGraphicsPresentQueueSame =
        header->device.graphicsQueueIdx == header->device.presentQueueIdx;
    b8 isGraphicsTransferQueueSame =
        header->device.graphicsQueueIdx == header->device.transferQueueIdx;
    u32 idxCnt = 1;
    if (!isGraphicsPresentQueueSame) {
        idxCnt++;
    }
    if (!isGraphicsTransferQueueSame) {
        idxCnt++;
    }
    u32 indices[32];
    u8 index = 0;
    indices[index++] = header->device.graphicsQueueIdx;
    if (!isGraphicsPresentQueueSame) {
        indices[index++] = header->device.presentQueueIdx;
    }
    if (!isGraphicsTransferQueueSame) {
        indices[index++] = header->device.transferQueueIdx;
    }

    VkDeviceQueueCreateInfo queueCIs[32];
    for (u32 i = 0; i < idxCnt; ++i) {
        queueCIs[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCIs[i].queueFamilyIndex = indices[i];
        queueCIs[i].queueCount = 1;
        queueCIs[i].flags = 0;
        queueCIs[i].pNext = 0;
        f32 queuePriority = 1.0f;
        queueCIs[i].pQueuePriorities = &queuePriority;
    }

    // Request device features.
    // TODO: should be config driven
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCI = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCI.queueCreateInfoCount = idxCnt;
    deviceCI.pQueueCreateInfos = queueCIs;
    deviceCI.pEnabledFeatures = &deviceFeatures;
    deviceCI.enabledExtensionCount = 1;
    const char* extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    deviceCI.ppEnabledExtensionNames = &extension_names;

    // Deprecated and ignored, so pass nothing.
    deviceCI.enabledLayerCount = 0;
    deviceCI.ppEnabledLayerNames = 0;

    // Create the device.
    VK_CHECK(vkCreateDevice(header->device.physicalDevice, &deviceCI,
                            header->allocator, &header->device.device));

    FINFO("Logical device created.");

    // Get queues.
    vkGetDeviceQueue(header->device.device, header->device.graphicsQueueIdx, 0,
                     &header->device.graphicsQueue);

    vkGetDeviceQueue(header->device.device, header->device.presentQueueIdx, 0,
                     &header->device.presentQueue);

    vkGetDeviceQueue(header->device.device, header->device.transferQueueIdx, 0,
                     &header->device.transferQueue);
    FINFO("Queues obtained.");

    VkCommandPoolCreateInfo poolCreateInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolCreateInfo.queueFamilyIndex = header->device.graphicsQueueIdx;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_CHECK(vkCreateCommandPool(header->device.device, &poolCreateInfo,
                                 header->allocator,
                                 &header->device.graphicsCommandPool));
    FINFO("Graphics command pool created.");

    vulkanDeviceQuerySwapchainSupport(header->device.physicalDevice,
                                      header->surface,
                                      &header->device.swapchainSupport);
    return true;
}

b8 doesDeviceMeetRequirements(VkPhysicalDevice device, VkSurfaceKHR surface,
                              const VkPhysicalDeviceProperties* properties,
                              const VkPhysicalDeviceFeatures* features,
                              deviceRequirements* requirements,
                              deviceQueueFamilyInfo* outQueuesInfo,
                              VulkanSwapchainSupportInfo* outSwapchainSupport) {
    if (requirements->discreteGPU) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            FINFO(
                "Device is not a discrete GPU, and one is required. Skipping.");
            return false;
        }
    }

    u32 extLen;
    vkEnumerateDeviceExtensionProperties(device, 0, &extLen, 0);
    VkExtensionProperties* extArray =
        dinoCreateReserve(extLen, VkExtensionProperties);
    vkEnumerateDeviceExtensionProperties(device, 0, &extLen, extArray);

    for (u32 i = 0; i < dinoLength(requirements->deviceExts); i++) {
        b8 found = false;
        for (u32 j = 0; j < extLen; j++) {
            if (strEqualI(requirements->deviceExts[i],
                          extArray[j].extensionName)) {
                found = true;
                break;
            }
        }
        if (!found) {
            FERROR("Failed to find extension %s", requirements->deviceExts[i]);
            dinoDestroy(extArray);
            return false;
        }
    }
    dinoDestroy(extArray);

    outQueuesInfo->graphicsFamilyIdx = -1;
    outQueuesInfo->presentFamilyIdx = -1;
    outQueuesInfo->computeFamilyIdx = -1;
    outQueuesInfo->transferFamilyIdx = -1;

    u32 queuesCnt = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queuesCnt, 0);
    VkQueueFamilyProperties queues[32];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queuesCnt, queues);

    u8 minTransferScore = 255;
    for (u32 i = 0; i < queuesCnt; ++i) {
        u8 currentTransferScore = 0;

        // Graphics queue
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            outQueuesInfo->graphicsFamilyIdx = i;
            ++currentTransferScore;
        }

        // Compute queue
        if (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            outQueuesInfo->computeFamilyIdx = i;
            ++currentTransferScore;
        }

        // Transfer queue
        if (queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            if (currentTransferScore <= minTransferScore) {
                minTransferScore = currentTransferScore;
                outQueuesInfo->transferFamilyIdx = i;
            }
        }

        // Present queue
        VkBool32 supportsPresent = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &supportsPresent);
        if (supportsPresent) {
            outQueuesInfo->presentFamilyIdx = i;
        }
    }

    return true;
}

b8 vulkanDeviceGetCreate(VulkanInfo* vi) {
    u32 deviceCnt = 0;
    vkEnumeratePhysicalDevices(vi->instance, &deviceCnt, 0);

    if (deviceCnt == 0) {
        FFATAL("No devices support Vulkan.");
        return false;
    }

    VkPhysicalDevice devices[32];
    vkEnumeratePhysicalDevices(vi->instance, &deviceCnt, devices);

    // Set the requirements needed for the app
    // TODO: Make some of these configurable options if able
    deviceRequirements requirements = {};
    requirements.graphics = true;
    requirements.present = true;
    requirements.transfer = true;
    requirements.samplerAnisotropy = true;
    requirements.discreteGPU = true;
    requirements.deviceExts = dinoCreate(const char*);
    dinoPush(requirements.deviceExts, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    for (u32 i = 0; i < deviceCnt; ++i) {
        VkPhysicalDevice device = devices[i];

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(device, &memory);

        deviceQueueFamilyInfo queueInfo = {};
        b8 isGood = doesDeviceMeetRequirements(
            device, vi->surface, &deviceProperties, &deviceFeatures,
            &requirements, &queueInfo, &vi->device.swapchainSupport);

        if (isGood) {
            FINFO("Selected device: '%s'.", deviceProperties.deviceName);

            // Print GPU Info
            switch (deviceProperties.deviceType) {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    FINFO("GPU type is Unknown.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    FINFO("GPU type is Integrated.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    FINFO("GPU type is Descrete.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    FINFO("GPU type is Virtual.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    FINFO("GPU type is CPU.");
                    break;
            }

            FINFO("GPU Driver version: %d.%d.%d",
                  VK_VERSION_MAJOR(deviceProperties.driverVersion),
                  VK_VERSION_MINOR(deviceProperties.driverVersion),
                  VK_VERSION_PATCH(deviceProperties.driverVersion));

            // Vulkan API version.
            FINFO("Vulkan API version: %d.%d.%d",
                  VK_VERSION_MAJOR(deviceProperties.apiVersion),
                  VK_VERSION_MINOR(deviceProperties.apiVersion),
                  VK_VERSION_PATCH(deviceProperties.apiVersion));

            // Memory information
            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
                f32 memoryInGib = (((f32)memory.memoryHeaps[j].size) / 1024.0f /
                                   1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags &
                    VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    FINFO("Local GPU memory: %.2f GiB", memoryInGib);
                } else {
                    FINFO("Shared System memory: %.2f GiB", memoryInGib);
                }
            }

            // Save some info about the devices so we don't have to re-loop
            // through them
            vi->device.physicalDevice = devices[i];
            vi->device.graphicsQueueIdx = queueInfo.graphicsFamilyIdx;
            vi->device.presentQueueIdx = queueInfo.presentFamilyIdx;
            vi->device.computeQueueIdx = queueInfo.computeFamilyIdx;
            vi->device.transferQueueIdx = queueInfo.transferFamilyIdx;

            vi->device.properties = deviceProperties;
            vi->device.features = deviceFeatures;
            vi->device.memory = memory;
            break;
        }
    }

    // Make sure a device was gotten
    if (!vi->device.physicalDevice) {
        FERROR("No physical devices met the requirements.");
        return false;
    }

    FINFO("Physical device selected.");

    if (!createVulkanLogicalDevice(vi)) {
        FFATAL("Failed to create logical device");
    }

    return true;
}

void vulkanDeviceDestroy(VulkanInfo* vi) {
    vi->device.graphicsQueue = 0;
    vi->device.presentQueue = 0;
    vi->device.transferQueue = 0;

    vulkanSwapchainDestroy(vi, &vi->swapchain);

    vkDestroyCommandPool(vi->device.device,
                         vi->device.graphicsCommandPool, vi->allocator);

    vkDestroyDevice(vi->device.device, vi->allocator);

    if (vi->device.swapchainSupport.formats){
        ffree(vi->device.swapchainSupport.formats, sizeof(VkSurfaceFormatKHR) * vi->device.swapchainSupport.formatCnt, MEMORY_TAG_RENDERER);
        vi->device.swapchainSupport.formatCnt = -1;
    }

    if (vi->device.swapchainSupport.presentModes){
        ffree(vi->device.swapchainSupport.presentModes, sizeof(VkPresentModeKHR) * vi->device.swapchainSupport.presentModeCnt, MEMORY_TAG_RENDERER);
        vi->device.swapchainSupport.presentModeCnt = -1;
    }

    fzeroMemory(&vi->device.swapchainSupport.capabilities, sizeof(vi->device.swapchainSupport.capabilities));

    vi->device.physicalDevice = 0;

    vi->device.graphicsQueueIdx = -1;
    vi->device.presentQueueIdx = -1;
    vi->device.transferQueueIdx = -1;
}

b8 vulkanDeviceDetectDepthFormat(VulkanDevice* device) {
    // Format candidates
    const u64 candidateCnt = 3;
    VkFormat candidates[3] = {VK_FORMAT_D32_SFLOAT,
                              VK_FORMAT_D32_SFLOAT_S8_UINT,
                              VK_FORMAT_D24_UNORM_S8_UINT};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    device->depthFormat = VK_FORMAT_UNDEFINED;
    for (u64 i = 0; i < candidateCnt; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physicalDevice,
                                            candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            device->depthFormat = candidates[i];
            FDEBUG("Got depthFormat");
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depthFormat = candidates[i];
            FDEBUG("Got depthFormat 2");
            return true;
        }
    }

    return false;
}
