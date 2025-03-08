#include "device.h"
#include "core/asserts.h"
#include "core/logger.h"
#include "helpers/dinoarray.h"
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

b8 getVulkanDevice(VulkanInfo* vi) {
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
