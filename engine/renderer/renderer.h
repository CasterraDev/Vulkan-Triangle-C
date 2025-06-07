#pragma once

#include "renderTypes.h"

struct platformState;
struct ShaderUniform;

b8 rendererInit(u64* memoryRequirement, void* memoryState, const char* appName, u64 appWidth, u64 appHeight);
void rendererShutdown();

b8 rendererDraw(renderInfo* ri);
b8 rendererOnResized(u16 width, u16 height);

b8 rendererShaderCreate(const struct ShaderRS* srs, struct Shader* outShader);
b8 rendererShaderDelete(struct Shader* shader);

void rendererShaderUse(struct Shader* shader);

b8 rendererShaderApplyInstances(struct Shader* shader);
b8 rendererShaderApplyGlobals(struct Shader* shader);

b8 rendererShaderBindInstances(struct Shader* shader);
b8 rendererShaderBindGlobals(struct Shader* shader);

b8 rendererShaderSetUniform(struct Shader* shader, struct ShaderUniform* uniform, const void* value);
