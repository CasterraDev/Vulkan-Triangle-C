#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkanTypes.h"
#include "resources/resourcesTypes.h"
#include "systems/shaderSystem.h"

b8 vulkanShaderInit(VulkanInfo* vi);
b8 vulkanShaderShutdown(VulkanInfo* vi);

b8 vulkanShaderCreate(const ShaderRS* srs, Shader* outShader);

b8 vulkanShaderDestroy(Shader* shader);

void vulkanShaderUse(Shader* shader);

b8 vulkanShaderApplyInstances(Shader* shader);
b8 vulkanShaderApplyGlobals(Shader* shader);

b8 vulkanShaderBindInstances(Shader* shader);
b8 vulkanShaderBindGlobals(Shader* shader);

b8 vulkanShaderSetUniform(Shader* shader, ShaderUniform* uniform, const void* value);
