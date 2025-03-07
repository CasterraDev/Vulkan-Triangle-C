#pragma once

#include "renderTypes.h"

struct platformState;

b8 rendererInit(u64* memoryRequirement, void* memoryState, const char* appName, u64 appWidth, u64 appHeight);
void rendererShutdown();
