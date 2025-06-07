#include "renderer/renderTypes.h"

b8 vulkanInit(rendererBackend* backend, const char* appName, u64 appWidth, u64 appHeight);
void vulkanShutdown(rendererBackend* backend);

b8 vulkanDraw();
b8 vulkanOnResize(u16 width, u16 height);

b8 vulkanBeginFrame(struct rendererBackend* backend, f32 deltaTime);
b8 vulkanEndFrame(struct rendererBackend* backend, f32 deltaTime);
b8 vulkanBeginRenderpass(struct rendererBackend* backend, u8 renderpassId);
b8 vulkanEndRenderpass(struct rendererBackend* backend, u8 renderpassId);
