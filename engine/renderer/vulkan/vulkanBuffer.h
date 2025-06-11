#include "vulkan/vulkan_core.h"
#include "vulkanTypes.h"

b8 vulkanBufferCreate(VulkanInfo* vi, u64 size, b8 useFreelist,
                      VkBufferUsageFlags usageFlags,
                      VkMemoryPropertyFlags memProperties,
                      VulkanBuffer* outBuffer);
void vulkanBufferDestroy(VulkanInfo* vi, VulkanBuffer* buffer);

b8 vulkanBufferCopy(VulkanInfo* vi, VkCommandPool cp, VkQueue queue, VkFence fence,
                    VulkanBuffer* src, u64 srcOffset, u64 size,
                    VulkanBuffer* dest, u64 destOffset);
void* vulkanBufferMapMem(VkDevice device, u64 offset, u64 size,
                         VkMemoryMapFlags flags, VulkanBuffer* buffer);
void vulkanBufferUnmapMem(VkDevice device, VulkanBuffer* buffer);
void vulkanBufferBind(VkDevice device, VulkanBuffer* buffer, u64 offset);
void vulkanBufferInsertData(void* memoryPtr, const void* data, u64 size);
b8 vulkanBufferAllocate(VulkanBuffer* buffer, u64 size, u64* outOffset);
b8 vulkanBufferFree(VulkanBuffer* buffer, u64 size, u64 offset);
