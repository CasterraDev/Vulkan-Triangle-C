#pragma once

#include "defines.h"

#define FILENAME_MAX_LENGTH 256
#define MATERIAL_MAX_LENGTH 256
#define TEXTURE_MAX_TEXTURES 1024
#define MAX_MATERIAL_COUNT 1024

typedef enum ResourceType {
    RESOURCE_TYPE_TEXT = 0,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_SHADER,
    RESOURCE_TYPE_CUSTOM
} ResourceType;

typedef struct resource {
    u32 managerID;
    const char* name;
    char* fullPath;
    u32 dataSize;
    void* data;
} resource;
