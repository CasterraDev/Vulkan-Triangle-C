#pragma once

#include "defines.h"

struct ShaderRS;
struct Shader;
struct ShaderUniform;

typedef enum rendererBackendAPI {
    RENDERER_BACKEND_API_VULKAN
} rendererBackendAPI;

typedef struct renderInfo {
    f32 deltaTime;
} renderInfo;

// The info and PFN signatures that will connect the engine's render abstraction
// layer to vulkan
typedef struct rendererBackend {
    // State of the platform. Will be different for each platform.
    struct platformState* pState;
    // rendererDraw increments the frameNum
    u64 frameNum;

    b8 (*init)(struct rendererBackend* backend, const char* appName,
               u64 appWidth, u64 appHeight);

    void (*shutdown)(struct rendererBackend* backend);

    b8 (*draw)(renderInfo* ri);
    b8 (*beginFrame)(struct rendererBackend* backend, f32 deltaTime);
    b8 (*endFrame)(struct rendererBackend* backend, f32 deltaTime);
    b8 (*beginRenderpass)(struct rendererBackend* backend, u8 renderpassId);
    b8 (*endRenderpass)(struct rendererBackend* backend, u8 renderpassId);
    b8 (*onResize)(u16 width, u16 height);

    b8 (*shaderCreate)(const struct ShaderRS* srs, struct Shader* outShader);
    b8 (*shaderDelete)(struct Shader* shader);
    void (*shaderUse)(struct Shader* s);

    b8 (*shaderApplyInstances)(struct Shader* shader);
    b8 (*shaderApplyGlobals)(struct Shader* shader);
    b8 (*shaderBindInstances)(struct Shader* shader);
    b8 (*shaderBindGlobals)(struct Shader* shader);
    b8 (*shaderSetUniform)(struct Shader* shader, struct ShaderUniform* uniform,
                           const void* value);

} rendererBackend;
