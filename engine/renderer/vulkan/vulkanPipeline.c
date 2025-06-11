#include "vulkanPipeline.h"
#include "core/logger.h"
#include "helpers/dinoarray.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "resources/resourcesTypes.h"
#include "vulkan/vulkan_core.h"

b8 vulkanPipelineCreate(VulkanInfo* vi, VulkanPipelineConfig vpc,
                        VulkanPipeline* outPipeline) {
    // Dynamic State
    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                       VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dsci;
    dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dsci.dynamicStateCount = 2;
    dsci.pDynamicStates = dynamicStates;

    VkVertexInputBindingDescription vertexBindingDesc;
    vertexBindingDesc.binding = 0;
    vertexBindingDesc.stride = sizeof(Vertex);
    vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vici;
    vici.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vici.vertexAttributeDescriptionCount = dinoLength(vpc.attributes);
    vici.pVertexAttributeDescriptions = vpc.attributes;
    vici.vertexBindingDescriptionCount = 1;
    vici.pVertexBindingDescriptions = &vertexBindingDesc;

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo iaci;
    iaci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    iaci.primitiveRestartEnable = VK_FALSE;

    // TODO: Make defaults for some of the config variables if they aren't given

    // Viewport & Scissor
    VkPipelineViewportStateCreateInfo viewportCI;
    viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCI.viewportCount = 1;
    viewportCI.pViewports = &vpc.viewport;
    viewportCI.scissorCount = 1;
    viewportCI.pScissors = &vpc.scissor;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasCI;
    rasCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasCI.depthClampEnable = VK_FALSE;
    rasCI.rasterizerDiscardEnable = VK_FALSE;
    rasCI.polygonMode =
        vpc.isWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasCI.lineWidth = 1.0f;
    rasCI.cullMode = VK_CULL_MODE_BACK_BIT;
    rasCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasCI.depthBiasEnable = VK_FALSE;
    rasCI.depthBiasConstantFactor = 0.0f;
    rasCI.depthBiasClamp = 0.0f;
    rasCI.depthBiasSlopeFactor = 0.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo msCI;
    msCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msCI.sampleShadingEnable = VK_FALSE;
    msCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msCI.minSampleShading = 1.0f;
    msCI.pSampleMask = 0;
    msCI.alphaToCoverageEnable = VK_FALSE;
    msCI.alphaToOneEnable = VK_FALSE;

    // Color Attachment
    VkPipelineColorBlendAttachmentState cbs;
    cbs.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cbs.blendEnable = VK_FALSE;
    cbs.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    cbs.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    cbs.colorBlendOp = VK_BLEND_OP_ADD;
    cbs.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cbs.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cbs.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo cbci;
    cbci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbci.logicOpEnable = VK_FALSE;
    cbci.logicOp = VK_LOGIC_OP_COPY;
    cbci.attachmentCount = 1;
    cbci.pAttachments = &cbs;
    cbci.blendConstants[0] = 0.0f;
    cbci.blendConstants[1] = 0.0f;
    cbci.blendConstants[2] = 0.0f;
    cbci.blendConstants[3] = 0.0f;
    cbci.flags = 0;
    cbci.pNext = 0;

    VkPipelineLayoutCreateInfo plci;
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 0;
    plci.pSetLayouts = 0;
    plci.pushConstantRangeCount = 0;
    plci.pPushConstantRanges = 0;

    VK_CHECK(vkCreatePipelineLayout(vi->device.device, &plci, vi->allocator,
                                    &outPipeline->layout));

    // Pipeline
    VkGraphicsPipelineCreateInfo pci;
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.pStages = vpc.stages;
    pci.stageCount = vpc.stageCnt;
    pci.pVertexInputState = &vici;
    pci.pInputAssemblyState = &iaci;
    pci.pViewportState = &viewportCI;
    pci.pRasterizationState = &rasCI;
    pci.pMultisampleState = &msCI;
    pci.pDepthStencilState = 0;
    pci.pColorBlendState = &cbci;
    pci.pDynamicState = &dsci;
    pci.pTessellationState = 0;

    pci.layout = outPipeline->layout;
    pci.renderPass = vpc.renderpass->handle;
    pci.subpass = 0;
    pci.basePipelineHandle = VK_NULL_HANDLE;
    pci.basePipelineIndex = -1;

    VK_CHECK(vkCreateGraphicsPipelines(vi->device.device, VK_NULL_HANDLE, 1,
                                       &pci, vi->allocator,
                                       &outPipeline->handle));

    FDEBUG("Graphics Pipeline Created");

    return true;
}

b8 vulkanPipelineDestroy(VulkanInfo* vi, VulkanPipeline* pipeline) {
    if (pipeline->handle) {
        vkDestroyPipeline(vi->device.device, pipeline->handle, vi->allocator);
        pipeline->handle = 0;
    }

    if (pipeline->layout) {
        vkDestroyPipelineLayout(vi->device.device, pipeline->layout,
                                vi->allocator);
        pipeline->layout = 0;
    }
    return true;
}

void vulkanPipelineBind(VkCommandBuffer cb, VkPipelineBindPoint bindPoint, VkPipeline pipeline){
    vkCmdBindPipeline(cb, bindPoint, pipeline);
}
