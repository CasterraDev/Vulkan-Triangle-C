#include "core/event.h"
#include "core/fmemory.h"
#include "core/input.h"
#include "core/logger.h"
#include "defines.h"
#include "platform/platform.h"
#include "renderer/renderer.h"
#include "resources/resourceManager.h"
#include "systems/shaderSystem.h"
#include <stdint.h>

#define APP_WIDTH 1280
#define APP_HEIGHT 720

// TODO: Move to renderer's abstraction layer


typedef struct App {
    b8 shouldQuit;
    u64 width;
    u64 height;
    u64 platformMemReq;
    void* platformState;

    u64 resourceManagerMemReq;
    void* resourceManagerState;

    u64 eventMemReq;
    void* eventState;

    u64 inputMemReq;
    void* inputState;

    u64 rendererMemReq;
    void* rendererState;

    u64 shaderSystemMemReq;
    void* shaderSystemState;
} App;

static App* app;

b8 quitApp(u16 code, void* sender, void* listenerInst, eventContext ec){
    app->shouldQuit = 1;
    return true;
}

b8 appResized(u16 code, void* sender, void* listenerInst, eventContext ec){
    u16 width = ec.data.u16[0];
    u16 height = ec.data.u16[1];

    app->width = width;
    app->height = height;

    if (width == 0 || height == 0){
        // TODO: Window is minimized
        return true;
    }else{
        // TODO: Renderer abstracted onResize FN
        rendererOnResized(width, height);
    }

    return false;
}

int main(void) {
    FINFO("Hello There.\n");

    memorySystemSettings memorySettings;
    memorySettings.totalSize = GIGABYTES(1);
    memoryInit(memorySettings);

    app = fallocate(sizeof(App), MEMORY_TAG_APPLICATION);
    app->shouldQuit = 0;

    eventInit(&app->eventMemReq, 0);
    app->eventState = fallocate(app->eventMemReq, MEMORY_TAG_UNKNOWN);
    eventInit(&app->eventMemReq, app->eventState);

    eventRegister(EVENT_CODE_APPLICATION_QUIT, 0, quitApp);
    eventRegister(EVENT_CODE_RESIZED, 0, appResized);

    inputInit(&app->inputMemReq, 0);
    app->inputState = fallocate(app->inputMemReq, MEMORY_TAG_UNKNOWN);
    inputInit(&app->inputMemReq, app->inputState);

    app->width = APP_WIDTH;
    app->height = APP_HEIGHT;
    app->platformMemReq = 0;
    platformStartup(&app->platformMemReq, 0, "Triangle", 0, 0, APP_WIDTH, APP_HEIGHT);
    app->platformState = fallocate(app->platformMemReq, MEMORY_TAG_APPLICATION);
    platformStartup(&app->platformMemReq, app->platformState, "Triangle", 0, 0,
                    APP_WIDTH, APP_HEIGHT);

    resourceManagerSettings resourceManagerSettings;
    resourceManagerSettings.maxManagers = 5;
    resourceManagerSettings.rootAssetPath = "./Assets/";
    resourceManagerInit(&app->resourceManagerMemReq, 0, resourceManagerSettings);
    app->resourceManagerState = fallocate(app->resourceManagerMemReq, MEMORY_TAG_APPLICATION);
    resourceManagerInit(&app->resourceManagerMemReq, app->resourceManagerState, resourceManagerSettings);


    rendererInit(&app->rendererMemReq, 0, "", APP_WIDTH, APP_HEIGHT);
    app->rendererState = fallocate(app->rendererMemReq, MEMORY_TAG_RENDERER);
    rendererInit(&app->rendererMemReq, app->rendererState, "Triangle", APP_WIDTH, APP_HEIGHT);

    shaderSystemSettings sss;
    sss.maxShaders = 100;
    shaderSystemInit(&app->shaderSystemMemReq, 0, sss);
    app->shaderSystemState = fallocate(app->shaderSystemMemReq, MEMORY_TAG_RENDERER);
    shaderSystemInit(&app->shaderSystemMemReq, app->shaderSystemState, sss);

    printMemoryUsage();

    u32 li = 0;
    renderInfo ri;
    ri.deltaTime = 100;
    while(!app->shouldQuit){
        platformPumpMessages();
        rendererDraw(&ri);
    };

    FINFO("Shutting Down Engine...");

    shaderSystemShutdown();
    rendererShutdown();
    resourceManagerShutdown(&app->resourceManagerState);
    platformShutdown();
    inputShutdown(&app->inputState);
    eventShutdown();
    memoryShutdown();
    return 0;
}
