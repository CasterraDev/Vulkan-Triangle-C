#pragma once

#include "defines.h"
#include "resources/resourcesTypes.h"
#include "helpers/hashtable.h"

typedef struct shaderSystemSettings {
    u32 maxShaders;
} shaderSystemSettings;

typedef struct ShaderUniform {
    char* name;
    u64 offset;

    u16 location;
    u16 uniformIdx;
    u16 size;

    ShaderScope scope;

    ShaderUniformType type;
} ShaderUniform;

typedef struct ShaderAttribute {
    char* name;
    u16 size;
    ShaderAttributeType type;
} ShaderAttribute;

typedef struct Shader {
    u32 id;
    char* name;

    u32 refCnt;
    b8 autoDelete;

    b8 supportsInstances;
    b8 supportsLocals;

    u32 globalUboSize;
    u32 uboSize;
    
    /** Memory Block for the uniform hashtable **/
    void* uniformHashtableBlock;
    /** Stores the idx of uniforms to use for the `uniforms` DinoArray **/
    hashtable uniformHashtable;
    /** DinoArray **/
    ShaderUniform* uniforms;

    /** Memory Block for the attribute hashtable **/
    void* attributeHashtableBlock;
    /** Stores the idx of attributes to use for the `attributes` DinoArray **/
    hashtable attributeHashtable;
    /** DinoArray **/
    ShaderAttribute* attributes;

    void* rendererData;
} Shader;

b8 shaderSystemInit(u64* memoryReq, void* memory, shaderSystemSettings settings);
void shaderSystemShutdown();

/* It will create then give the shader */
b8 shaderCreate(const ShaderRS* srs, Shader* outShader);
/* derefernce the shader. If the shader has no references and autoDelete = 1, then it will delete itself. */
void shaderDeref(const char* shaderName);
/* Actually delete the shader. Should only be used by `shaderDeref` or if you know the shader is bugged/gone */
b8 shaderDelete(Shader* shader);
/* Get a shader's Id by name */
u32 shaderGetId(const char* shaderName);
/* Get a shader by name */
Shader* shaderGet(char* name);
/* Get a shader by id */
Shader* shaderGetById(u32 id);
/* Use a shader */
void shaderUse(Shader* s);
