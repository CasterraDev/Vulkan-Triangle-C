#include "vulkanBuffer.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "defines.h"
#include "helpers/freelist.h"
#include "renderer/vulkan/vulkanCommandBuffers.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "vulkan/vulkan_core.h"

i32 findMemoryType(VulkanInfo* vi, u32 typeFilter,
                   VkMemoryPropertyFlags props) {
    for (u32 i = 0; i < vi->device.memory.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) &&
            (vi->device.memory.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    FERROR("Failed to find suitable memory type. Returning idx -1.");
    return -1;
}

b8 vulkanBufferCreate(VulkanInfo* vi, u64 size, b8 useFreelist,
                      VkBufferUsageFlags usageFlags,
                      VkMemoryPropertyFlags memProperties,
                      VulkanBuffer* outBuffer) {
    outBuffer->bufferSize = size;
    outBuffer->memProperties = memProperties;
    outBuffer->usageFlags = usageFlags;
    outBuffer->usesFreelist = useFreelist;

    VkBufferCreateInfo bci;
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = size;
    bci.usage = usageFlags;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bci.flags = 0;
    bci.pNext = 0;

    VK_CHECK(vkCreateBuffer(vi->device.device, &bci, vi->allocator,
                            &outBuffer->handle));

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(vi->device.device, outBuffer->handle,
                                  &memReq);
    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(vi, memReq.memoryTypeBits, memProperties);
    allocInfo.pNext = 0;

    if (vkAllocateMemory(vi->device.device, &allocInfo, vi->allocator,
                         &outBuffer->bufferMemory)) {
        FERROR("Failed to allocate buffer memory.");
        return false;
    }

    if (useFreelist) {
        outBuffer->freelistReqMem = 0;
        freelistCreate(size, &outBuffer->freelistReqMem, 0,
                       &outBuffer->bufferFreelist);
        outBuffer->freelistMemory =
            fallocate(outBuffer->freelistReqMem, MEMORY_TAG_RENDERER);
        freelistCreate(size, &outBuffer->freelistReqMem,
                       outBuffer->freelistMemory, &outBuffer->bufferFreelist);
    }
    return true;
}

void vulkanBufferDestroy(VulkanInfo* vi, VulkanBuffer* buffer) {
    if (buffer->usesFreelist) {
        ffree(buffer->freelistMemory, buffer->bufferSize, MEMORY_TAG_RENDERER);
    }

    if (buffer->bufferMemory) {
        vkFreeMemory(vi->device.device, buffer->bufferMemory, vi->allocator);
    }

    if (buffer->handle) {
        vkDestroyBuffer(vi->device.device, buffer->handle, vi->allocator);
    }

    buffer->bufferSize = 0;
    buffer->handle = 0;
}

b8 vulkanBufferCopy(VulkanInfo* vi, VkCommandPool cp, VkQueue queue,
                    VkFence fence, VulkanBuffer* src, u64 srcOffset, u64 size,
                    VulkanBuffer* dest, u64 destOffset) {
    VulkanCommandBuffer vcb;
    vulkanCommandBufferAllocate(vi, cp, true, &vcb);
    vulkanCommandBufferBegin(&vcb, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy bc;
    bc.srcOffset = srcOffset;
    bc.dstOffset = destOffset;
    bc.size = size;
    vkCmdCopyBuffer(vcb.handle, src->handle, dest->handle, 1, &bc);
    vulkanCommandBufferEnd(&vcb);

    VkSubmitInfo si;
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &vcb.handle;
    si.pNext = 0;

    vkQueueSubmit(queue, 1, &si, fence);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(vi->device.device, cp, 1, &vcb.handle);
    return true;
}

void* vulkanBufferMapMem(VkDevice device, u64 offset, u64 size,
                         VkMemoryMapFlags flags, VulkanBuffer* buffer) {
    void* x;
    vkMapMemory(device, buffer->bufferMemory, offset, size, flags, &x);
    return x;
}
void vulkanBufferUnmapMem(VkDevice device, VulkanBuffer* buffer) {
    vkUnmapMemory(device, buffer->bufferMemory);
}

void vulkanBufferBind(VkDevice device, VulkanBuffer* buffer, u64 offset) {
    VK_CHECK(vkBindBufferMemory(device, buffer->handle, buffer->bufferMemory,
                                offset));
}

void vulkanBufferInsertData(void* memoryPtr, const void* data, u64 size) {
    fcopyMemory(memoryPtr, data, size);
}

b8 vulkanBufferAllocate(VulkanBuffer* buffer, u64 size, u64* outOffset) {
    if (!buffer->usesFreelist) {
        FWARN("vulkanBufferAllocate: Buffer must use freelist to use this FN");
        return false;
    }

    return freelistAllocateBlock(&buffer->bufferFreelist, size, outOffset);
}

b8 vulkanBufferFree(VulkanBuffer* buffer, u64 size, u64 offset) {
    if (!buffer->usesFreelist) {
        FWARN("vulkanBufferFree: Buffer must use freelist to use this FN");
        return false;
    }

    return freelistFreeBlock(&buffer->bufferFreelist, size, offset);
}
