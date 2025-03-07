#include "core/logger.h"
#include "renderer.h"
#include "renderer/vulkan/vulkan.h"

typedef struct rendererSystem{
    rendererBackend rb;
    u32 frameBufferWidth;
    u32 frameBufferHeight;
} rendererSystem;

static rendererSystem* systemPtr;

b8 rendererCreate(rendererBackendAPI api, rendererBackend* rb){
    // If renderer is VULKAN
    if (api == RENDERER_BACKEND_API_VULKAN){
        rb->init = vulkanInit;
        rb->shutdown = vulkanShutdown;

        return true;
    }
    return false;
}

b8 rendererDestroy(rendererBackend* rb){
    rb->init = 0;
    rb->shutdown = 0;
    return true;
}

b8 rendererInit(u64* memoryRequirement, void* memoryState, const char* appName, u64 appWidth, u64 appHeight){
    *memoryRequirement = sizeof(rendererSystem);
    if (memoryState == 0){
        return true;
    }
    systemPtr = memoryState;

    systemPtr->frameBufferWidth = appWidth;
    systemPtr->frameBufferHeight = appHeight;

    //Assign the pointer functions to the real functions
    if(!rendererCreate(RENDERER_BACKEND_API_VULKAN, &systemPtr->rb)){
        FERROR("Renderer Create Failed");
        return false;
    }
    systemPtr->rb.frameNum = 0;
    //Init the renderer
    if (!systemPtr->rb.init(&systemPtr->rb, appName, appWidth, appHeight)){
        FERROR("Renderer Init Failed");
        return false;
    }
    return true;
}

void rendererShutdown(){
    systemPtr->rb.shutdown(&systemPtr->rb);
    rendererDestroy(&systemPtr->rb);
}
