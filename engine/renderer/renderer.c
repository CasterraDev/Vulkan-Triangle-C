#include "renderer.h"
#include "core/logger.h"
#include "renderer/vulkan/vulkan.h"
#include "renderer/vulkan/vulkanShader.h"

typedef struct rendererSystem {
    rendererBackend rb;
    u32 frameBufferWidth;
    u32 frameBufferHeight;
} rendererSystem;

static rendererSystem* systemPtr;

b8 rendererCreate(rendererBackendAPI api, rendererBackend* rb) {
    // If renderer is VULKAN
    if (api == RENDERER_BACKEND_API_VULKAN) {
        rb->init = vulkanInit;
        rb->shutdown = vulkanShutdown;

        rb->draw = vulkanDraw;
        rb->beginFrame = vulkanBeginFrame;
        rb->endFrame = vulkanEndFrame;
        rb->beginRenderpass = vulkanBeginRenderpass;
        rb->endRenderpass = vulkanEndRenderpass;

        rb->shaderCreate = vulkanShaderCreate;
        rb->shaderDelete = vulkanShaderDestroy;
        rb->shaderUse = vulkanShaderUse;
        rb->shaderApplyInstances = vulkanShaderApplyInstances;
        rb->shaderApplyGlobals = vulkanShaderApplyGlobals;
        rb->shaderBindInstances = vulkanShaderBindInstances;
        rb->shaderBindGlobals = vulkanShaderBindGlobals;
        rb->shaderSetUniform = vulkanShaderSetUniform;

        return true;
    }
    return false;
}

b8 rendererDestroy(rendererBackend* rb) {
    rb->init = 0;
    rb->shutdown = 0;

    rb->draw = 0;
    rb->beginFrame = 0;
    rb->endFrame = 0;
    rb->beginRenderpass = 0;
    rb->endRenderpass = 0;

    rb->shaderCreate = 0;
    rb->shaderDelete = 0;
    rb->shaderUse = 0;
    rb->shaderApplyInstances = 0;
    rb->shaderApplyGlobals = 0;
    rb->shaderBindInstances = 0;
    rb->shaderBindGlobals = 0;
    rb->shaderSetUniform = 0;
    return true;
}

b8 rendererInit(u64* memoryRequirement, void* memoryState, const char* appName,
                u64 appWidth, u64 appHeight) {
    *memoryRequirement = sizeof(rendererSystem);
    if (memoryState == 0) {
        return true;
    }
    systemPtr = memoryState;

    systemPtr->frameBufferWidth = appWidth;
    systemPtr->frameBufferHeight = appHeight;

    // Assign the pointer functions to the real functions
    if (!rendererCreate(RENDERER_BACKEND_API_VULKAN, &systemPtr->rb)) {
        FERROR("Renderer Create Failed");
        return false;
    }
    systemPtr->rb.frameNum = 0;
    // Init the renderer
    if (!systemPtr->rb.init(&systemPtr->rb, appName, appWidth, appHeight)) {
        FERROR("Renderer Init Failed");
        return false;
    }
    FDEBUG("Renderer inited.");
    return true;
}

void rendererShutdown() {
    systemPtr->rb.shutdown(&systemPtr->rb);
    rendererDestroy(&systemPtr->rb);
}

b8 rendererDraw(renderInfo* ri){
    if (systemPtr->rb.beginFrame(&systemPtr->rb, ri->deltaTime)){
        if (!systemPtr->rb.beginRenderpass(&systemPtr->rb, 0)){
            FERROR("BeginRenderpass failed")
            return false;
        }

        // TODO: Update global state

        systemPtr->rb.draw(ri);

        if (!systemPtr->rb.endRenderpass(&systemPtr->rb, 0)){
            FERROR("EndRenderpass failed");
            return false;
        }

        if (!systemPtr->rb.endFrame(&systemPtr->rb, ri->deltaTime)){
            FERROR("EndFrame failed");
            return false;
        }
        systemPtr->rb.frameNum++;
    }
    return true;
}

b8 rendererShaderCreate(const ShaderRS* srs, Shader* outShader) {
    return systemPtr->rb.shaderCreate(srs, outShader);
}

b8 rendererShaderDelete(Shader* shader) {
    return systemPtr->rb.shaderDelete(shader);
}

void rendererShaderUse(Shader* shader) {
    systemPtr->rb.shaderUse(shader);
}

b8 rendererShaderApplyInstances(Shader* shader) {
    return systemPtr->rb.shaderApplyInstances(shader);
}

b8 rendererShaderApplyGlobals(Shader* shader) {
    return systemPtr->rb.shaderApplyGlobals(shader);
}

b8 rendererShaderBindInstances(Shader* shader) {
    return systemPtr->rb.shaderBindInstances(shader);
}

b8 rendererShaderBindGlobals(Shader* shader) {
    return systemPtr->rb.shaderBindGlobals(shader);
}

b8 rendererShaderSetUniform(Shader* shader, ShaderUniform* uniform,
                            const void* value) {
    return systemPtr->rb.shaderSetUniform(shader, uniform, value);
}
