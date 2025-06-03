#include "vulkanCommandBuffers.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "vulkan/vulkan_core.h"

void vulkanCommandBufferAllocate(VulkanInfo* vi, VkCommandPool comPool, b8 isPrimaryLevel, VulkanCommandBuffer* outCommandBuffer){
        outCommandBuffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;

        VkCommandBufferAllocateInfo alloc;
        alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc.commandPool = comPool;
        alloc.level = isPrimaryLevel ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        alloc.commandBufferCount = 1;
        alloc.pNext = 0;

        VK_CHECK(vkAllocateCommandBuffers(vi->device.device, &alloc, &outCommandBuffer->handle))
        outCommandBuffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkanCommandBufferBegin(VulkanCommandBuffer* commandBuffer){
    VkCommandBufferBeginInfo begin;
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = 0;
    begin.pInheritanceInfo = 0;
    begin.pNext = 0;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer->handle, &begin) != VK_SUCCESS);
    commandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}

void vulkanCommandBufferEnd(VulkanCommandBuffer* commandBuffer){
    VK_CHECK(vkEndCommandBuffer(commandBuffer->handle));
    commandBuffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkanCommandBufferReset(VulkanCommandBuffer* commandBuffer){
    commandBuffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkanCommandBufferUpdateSubmitted(VulkanCommandBuffer* commandBuffer){
    commandBuffer->state = COMMAND_BUFFER_STATE_SUBMITTED;
}
