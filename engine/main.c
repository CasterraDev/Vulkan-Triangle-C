#include "core/fmemory.h"
#include "core/logger.h"
#include "defines.h"
#include "platform/platform.h"
#include "renderer/renderer.h"
#include "resources/resourceManager.h"
#include <stdint.h>

#define APP_WIDTH 1280
#define APP_HEIGHT 720

// TODO: Move to renderer's abstraction layer


typedef struct App {
    u64 platformMemReq;
    void* platformState;

    u64 resourceManagerMemReq;
    void* resourceManagerState;

    u64 rendererMemReq;
    void* rendererState;
} App;

static App* app;

int main(void) {
    FINFO("Hello There.\n");

    memorySystemSettings memorySettings;
    memorySettings.totalSize = GIGABYTES(1);
    memoryInit(memorySettings);

    app = fallocate(sizeof(App), MEMORY_TAG_APPLICATION);

    app->platformMemReq = 0;
    platformStartup(&app->platformMemReq, 0, "Triangle", 0, 0, APP_WIDTH, APP_HEIGHT);
    app->platformState = fallocate(app->platformMemReq, MEMORY_TAG_APPLICATION);
    platformStartup(&app->platformMemReq, app->platformState, "Triangle", 0, 0,
                    APP_WIDTH, APP_HEIGHT);

    resourceManagerSettings resourceManagerSettings;
    resourceManagerSettings.maxManagers = 2;
    resourceManagerSettings.rootAssetPath = "./Assets/";
    resourceManagerInit(&app->resourceManagerMemReq, 0, resourceManagerSettings);
    app->resourceManagerState = fallocate(app->resourceManagerMemReq, MEMORY_TAG_APPLICATION);
    resourceManagerInit(&app->resourceManagerMemReq, app->resourceManagerState, resourceManagerSettings);

    rendererInit(&app->rendererMemReq, 0, "", APP_WIDTH, APP_HEIGHT);
    app->rendererState = fallocate(app->rendererMemReq, MEMORY_TAG_RENDERER);
    rendererInit(&app->rendererMemReq, app->rendererState, "Triangle", APP_WIDTH, APP_HEIGHT);

    printMemoryUsage();
    platformSleep(2000);

    rendererShutdown();
    platformShutdown();
    memoryShutdown();
    return 0;
}
