#pragma once

#include "defines.h"
#include "math/matrixMath.h"

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

typedef struct Resource {
    u32 managerID;
    const char* name;
    char* fullPath;
    u32 dataSize;
    void* data;
} Resource;

typedef struct Vertex {
    vector2 position;
    vector3 color;
} Vertex;

/** @brief Shader stages available. */
typedef enum ShaderStage {
    SHADER_STAGE_VERTEX = 0x00000001,
    SHADER_STAGE_GEOMETRY = 0x00000002,
    SHADER_STAGE_FRAGMENT = 0x00000004,
    SHADER_STAGE_COMPUTE = 0x0000008
} ShaderStage;

/** @brief Available attribute types. */
typedef enum ShaderAttributeType {
    SHADER_ATTRIBUTE_TYPE_FLOAT32 = 0U,
    SHADER_ATTRIBUTE_TYPE_FLOAT32_2 = 1U,
    SHADER_ATTRIBUTE_TYPE_FLOAT32_3 = 2U,
    SHADER_ATTRIBUTE_TYPE_FLOAT32_4 = 3U,
    SHADER_ATTRIBUTE_TYPE_INT8 = 5U,
    SHADER_ATTRIBUTE_TYPE_UINT8 = 6U,
    SHADER_ATTRIBUTE_TYPE_INT16 = 7U,
    SHADER_ATTRIBUTE_TYPE_UINT16 = 8U,
    SHADER_ATTRIBUTE_TYPE_INT32 = 9U,
    SHADER_ATTRIBUTE_TYPE_UINT32 = 10U,
} ShaderAttributeType;

/** @brief Available uniform types. */
typedef enum ShaderUniformType {
    SHADER_UNIFORM_TYPE_FLOAT32 = 0U,
    SHADER_UNIFORM_TYPE_FLOAT32_2 = 1U,
    SHADER_UNIFORM_TYPE_FLOAT32_3 = 2U,
    SHADER_UNIFORM_TYPE_FLOAT32_4 = 3U,
    SHADER_UNIFORM_TYPE_INT8 = 4U,
    SHADER_UNIFORM_TYPE_UINT8 = 5U,
    SHADER_UNIFORM_TYPE_INT16 = 6U,
    SHADER_UNIFORM_TYPE_UINT16 = 7U,
    SHADER_UNIFORM_TYPE_INT32 = 8U,
    SHADER_UNIFORM_TYPE_UINT32 = 9U,
    SHADER_UNIFORM_TYPE_MATRIX_4 = 10U,
    SHADER_UNIFORM_TYPE_SAMPLER = 11U,
    SHADER_UNIFORM_TYPE_CUSTOM = 255U
} ShaderUniformType;

/**
 * @brief Defines shader scope, which indicates how
 * often it gets updated.
 */
typedef enum ShaderScope {
    /** @brief Global shader scope, generally updated once per frame. */
    SHADER_SCOPE_GLOBAL = 0,
    /** @brief Instance shader scope, generally updated "per-instance" of the shader. */
    SHADER_SCOPE_INSTANCE = 1,
    /** @brief Local shader scope, generally updated per-object */
    SHADER_SCOPE_LOCAL = 2
} ShaderScope;

typedef struct ShaderAttributeConfig {
    /* The name of the Attribute */
    char* name;
    /* The length of the name of the Attribute */
    u8 nameLen;
    /* The size of the Attribute */
    u8 size;
    /* The type of the Attribute */
    ShaderAttributeType type;
} ShaderAttributeConfig;

typedef struct ShaderUniformConfig {
    /* The name of the Uniform */
    char* name;
    /* The length of the name of the Uniform */
    u8 nameLen;
    /* The size of the Uniform */
    u8 size;
    /* The type of the Uniform */
    ShaderUniformType type;
    /* The location of the Uniform */
    u32 location;
    /* The scope of the Uniform */
    ShaderScope scope;
} ShaderUniformConfig;

typedef struct ShaderRS {
    char* name;
    b8 supportsInstances;
    b8 supportsLocals;

    u8 stageCnt;
    ShaderStage* stages;
    char** stageNames;
    char** stageFiles;

    char* renderpassName;

    u8 attributeCnt;
    ShaderAttributeConfig* attributes;

    u8 uniformCnt;
    ShaderUniformConfig* uniforms;
} ShaderRS;
