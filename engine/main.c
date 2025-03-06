#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct App{
    u64 platformMemReq;
    void* platformState;
} App;

static App* app;

int main(void){
    printf("Hello There.\n");
    app = (App*)malloc(sizeof(App));
    app->platformMemReq = 0;
    platformStartup(&app->platformMemReq, 0, "Triangle", 0, 0, 1280, 720);
    app->platformState = malloc(app->platformMemReq);
    platformStartup(&app->platformMemReq, app->platformState, "Triangle", 0, 0, 1280, 720);

    platformSleep(2000);

    platformShutdown();
    return 0;
}
