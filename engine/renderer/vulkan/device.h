#include "renderer/vulkan/vulkanTypes.h"

b8 vulkanDeviceGetCreate(VulkanInfo* vi);
void vulkanDeviceDestroy(VulkanInfo* vi);
b8 vulkanDeviceDetectDepthFormat(VulkanDevice* device);
