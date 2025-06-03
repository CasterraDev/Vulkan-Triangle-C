#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "helpers/dinoarray.h"
#include "renderer/vulkan/vulkanPipeline.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "resources/resourceManager.h"
#include "resources/resourcesTypes.h"
#include "systems/shaderSystem.h"
#include "vulkan/vulkan_core.h"

static VulkanInfo* vss;

VkShaderStageFlagBits convertResourceStageFlagtoVulkan(ShaderStage s);

b8 createShaderModule(VulkanInfo* header, const char* fileName,
                      VkShaderStageFlagBits stageFlagBits,
                      VulkanShaderStage* outShaderStage) {
    char filename[512];
    strFmt(filename, "%s", fileName);

    Resource binRes;
    if (!resourceLoad(filename, RESOURCE_TYPE_BINARY, &binRes)) {
        FERROR("Unable to read shader file: %s", filename);
        return false;
    }

    fzeroMemory(&outShaderStage->moduleCreateInfo,
                sizeof(VkShaderModuleCreateInfo));

    outShaderStage->moduleCreateInfo.sType =
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    outShaderStage->moduleCreateInfo.codeSize = binRes.dataSize;
    outShaderStage->moduleCreateInfo.pCode = (u32*)binRes.data;

    VK_CHECK(vkCreateShaderModule(header->device.device,
                                  &outShaderStage->moduleCreateInfo,
                                  header->allocator, &outShaderStage->module));

    resourceUnload(&binRes);

    fzeroMemory(&outShaderStage->stageCreateInfo,
                sizeof(VkPipelineShaderStageCreateInfo));

    outShaderStage->stageCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    outShaderStage->stageCreateInfo.stage = stageFlagBits;
    outShaderStage->stageCreateInfo.module = outShaderStage->module;
    outShaderStage->stageCreateInfo.pName = "main";

    return true;
}

VkShaderStageFlagBits convertResourceStageFlagtoVulkan(ShaderStage s) {
    switch (s) {
        case (SHADER_STAGE_VERTEX):
            return VK_SHADER_STAGE_VERTEX_BIT;
        case SHADER_STAGE_GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case SHADER_STAGE_FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case SHADER_STAGE_COMPUTE:
            return VK_SHADER_STAGE_COMPUTE_BIT;
    }
}

b8 vulkanShaderInit(VulkanInfo* vi) {
    vss = vi;

    return true;
}

b8 vulkanShaderShutdown(VulkanInfo* vi) {
    vss = 0;
    return true;
}

b8 vulkanShaderCreate(const ShaderRS* shaderConfig, Shader* outShader) {
    outShader->rendererData =
        fallocate(sizeof(VulkanShader), MEMORY_TAG_RENDERER);

    u32 maxDescriptorAllocateCnt = 1024;

    VulkanShader* vs = (VulkanShader*)outShader->rendererData;

    VkPipelineShaderStageCreateInfo* stages = dinoCreateReserve(
        shaderConfig->stageCnt, VkPipelineShaderStageCreateInfo);

    vs->stages = dinoCreateReserve(shaderConfig->stageCnt, VulkanShaderStage);
    // Create the shader modules
    for (u32 i = 0; i < shaderConfig->stageCnt; i++) {
        createShaderModule(
            vss, shaderConfig->stageFiles[i],
            convertResourceStageFlagtoVulkan(shaderConfig->stages[i]),
            &vs->stages[i]);
        dinoPush(stages, vs->stages[i].stageCreateInfo);
    }
    // Set the length of vs->stages to be the correct length of stages. (This is
    // because we didn't use dinoPush to push items so length is still 0)
    dinoLengthSet(vs->stages, dinoLength(stages));

    VkRect2D scissor;
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent =
        (VkExtent2D){vss->framebufferWidth, vss->framebufferHeight};

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = vss->framebufferWidth;
    viewport.height = vss->framebufferHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VulkanPipelineConfig vpc;
    vpc.stages = stages;
    vpc.stageCnt = shaderConfig->stageCnt;
    vpc.isWireframe = false;
    vpc.scissor = scissor;
    vpc.viewport = viewport;
    vpc.depthTested = false;
    vpc.renderpass = &vss->renderpass;

    FDEBUG("Create the pipeline")
    if (!vulkanPipelineCreate(vss, vpc, &vs->pipeline)) {
        FERROR("Failed to create pipeline.");
        return false;
    }

    dinoDestroy(stages);
    return true;
}

b8 vulkanShaderDestroy(Shader* shader) {
    VulkanShader* vs = (VulkanShader*)shader->rendererData;

    u32 len = dinoLength(vs->stages);
    FDEBUG("LEN: %d", len);
    for (u32 i = 0; i < len; i++) {
        vkDestroyShaderModule(vss->device.device, vs->stages[i].module,
                              vss->allocator);
    }
    dinoDestroy(vs->stages);

    vkDeviceWaitIdle(vss->device.device);

    vkDestroyPipelineLayout(vss->device.device, vs->pipeline.layout,
                            vss->allocator);
    vkDestroyPipeline(vss->device.device, vs->pipeline.handle, vss->allocator);

    return true;
}

void vulkanShaderUse(Shader* shader) {
    VulkanShader* vs = (VulkanShader*)shader->rendererData;
    vulkanPipelineBind(vss->graphicsCommandBuffers[vss->curImageIdx].handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vs->pipeline.handle);
}

b8 vulkanShaderApplyInstances(Shader* shader) {
    return false;
}

b8 vulkanShaderApplyGlobals(Shader* shader) {
    return false;
}

b8 vulkanShaderBindInstances(Shader* shader) {
    return false;
}

b8 vulkanShaderBindGlobals(Shader* shader) {
    return false;
}

b8 vulkanShaderSetUniform(Shader* shader, ShaderUniform* uniform,
                          const void* value) {
    return false;
}
