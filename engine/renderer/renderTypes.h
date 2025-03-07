#pragma once

#include "defines.h"

typedef enum builtinRenderpass {
    BUILTIN_RENDERPASS_WORLD,
    BUILTIN_RENDERPASS_UI,
} builtinRenderpass;

typedef enum rendererBackendAPI {
    RENDERER_BACKEND_API_VULKAN
} rendererBackendAPI;

typedef struct rendererHeader {
    f32 deltaTime;
} rendererHeader;

// The info and PFN signatures that will connect the engine's render abstraction layer to vulkan
typedef struct rendererBackend {
    // State of the platform. Will be different for each platform.
    struct platformState* pState;
    // rendererDraw increments the frameNum
    u64 frameNum;

    b8 (*init)(struct rendererBackend* backend, const char* appName, u64 appWidth, u64 appHeight);

    void (*shutdown)(struct rendererBackend* backend);
} rendererBackend;
