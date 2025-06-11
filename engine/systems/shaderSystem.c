#include "shaderSystem.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "defines.h"
#include "helpers/dinoarray.h"
#include "helpers/hashtable.h"
#include "renderer/renderer.h"
#include "resources/resourceManager.h"
#include "resources/resourcesTypes.h"
#include "vulkan/vulkan_core.h"

typedef struct shaderSystemState {
    shaderSystemSettings settings;
    hashtable shaderTable;
    void* tableMemory;
    u32 curShaderId;
    Shader* shaderArray;
    // TODO: Temp
    Shader* materialShader;
} shaderSystemState;

static shaderSystemState* systemPtr = 0;

b8 shaderSystemInit(u64* memoryReq, void* memory,
                    shaderSystemSettings settings) {
    u64 stateReq = sizeof(shaderSystemState);
    u64 tableReq = sizeof(u32) * settings.maxShaders;
    u64 shaderArrReq = sizeof(Shader) * settings.maxShaders;
    *memoryReq = stateReq + tableReq + shaderArrReq;

    if (!memory) {
        return true;
    }

    systemPtr = memory;
    systemPtr->tableMemory = (void*)((u64)memory + stateReq);
    systemPtr->shaderArray = (void*)((u64)systemPtr->tableMemory + tableReq);
    systemPtr->settings = settings;
    systemPtr->curShaderId = INVALID_ID;

    hashtableCreate(sizeof(u32), settings.maxShaders, systemPtr->tableMemory,
                    &systemPtr->shaderTable);

    u32 v = INVALID_ID;
    hashtableFill(&systemPtr->shaderTable, &v);

    for (u32 i = 0; i < settings.maxShaders; i++){
        systemPtr->shaderArray[i].id = INVALID_ID;
    }

    Resource vertRes;
    resourceLoad("shader.shadercfg", RESOURCE_TYPE_SHADER, &vertRes);
    ShaderRS* vrs = (ShaderRS*)vertRes.data;
    shaderCreate(vrs, systemPtr->materialShader);
    resourceUnload(&vertRes);
    FDEBUG("Created builtin shaders.");
    return true;
}

void shaderSystemShutdown() {
    if (systemPtr) {
        for (u32 i = 0; i < systemPtr->settings.maxShaders; i++) {
            Shader* s = &systemPtr->shaderArray[i];
            if (s->id != INVALID_ID) {
                FDEBUG("Deleting Shader Name: %s", s->name);
                shaderDelete(s);
            }
        }
        hashtableDestroy(&systemPtr->shaderTable);
        fzeroMemory(systemPtr, sizeof(shaderSystemState));
        systemPtr = 0;
    }
}

b8 shaderCreate(const ShaderRS* srs, Shader* outShader) {
    u64 shaderId = INVALID_ID;
    for (u32 i = 0; i < systemPtr->settings.maxShaders; i++){
        if (systemPtr->shaderArray[i].id == INVALID_ID){
            shaderId = i;
        }
    }

    if (shaderId == INVALID_ID){
        FERROR("Shader Array is full.")
    }

    outShader = &systemPtr->shaderArray[shaderId];

    // TODO: Some hardcoded
    outShader->name = srs->name;
    outShader->autoDelete = 1;
    outShader->supportsInstances = srs->supportsInstances;
    outShader->supportsLocals = srs->supportsLocals;
    outShader->id = shaderId;
    outShader->refCnt++;

    // Attributes
    ShaderAttribute* attDescs = dinoCreate(ShaderAttribute);
    for (u32 i = 0; i < srs->attributeCnt; i++){
        ShaderAttribute sa;
        sa.size = srs->attributes[i].size;
        sa.name = strDup(srs->attributes[i].name);
        sa.type = srs->attributes[i].type;
        dinoPush(attDescs, sa);
    }

    ShaderUniform* uniDescs = dinoCreate(ShaderUniform);
    for (u32 i = 0; i < srs->uniformCnt; i++){
        ShaderUniform su;
        su.name = strDup(srs->uniforms[i].name);
        su.uniformIdx = i;
        su.scope = srs->uniforms[i].scope;
        su.type = srs->uniforms[i].type;
        if (su.scope == SHADER_SCOPE_LOCAL){

        }else{
            su.size = srs->uniforms[i].size;
            su.offset = (su.scope == SHADER_SCOPE_GLOBAL) ? outShader->globalUboSize : outShader->uboSize;
        }
    }
    

    FDEBUG("Create the render shader")
    if (!rendererShaderCreate(srs, outShader)) {
        FERROR("Renderer Shader Create failed");
        return false;
    }

    hashtableSet(&systemPtr->shaderTable, srs->name, &shaderId);
    FDEBUG("Created new shader: %s", systemPtr->shaderArray[shaderId].name);
    return true;
}

b8 shaderDelete(Shader* shader){
    return rendererShaderDelete(shader);
}

Shader* shaderGet(char* name){
    u32 id;
    hashtableGet(&systemPtr->shaderTable, name, &id);
    return &systemPtr->shaderArray[id];
}

void shaderUse(Shader* s){
    rendererShaderUse(s);
}

u32 getAttributeSize(ShaderAttributeType t) {
    switch (t) {
        case SHADER_ATTRIBUTE_TYPE_INT8:
        case SHADER_ATTRIBUTE_TYPE_UINT8:
            return 1;
        case SHADER_ATTRIBUTE_TYPE_INT16:
        case SHADER_ATTRIBUTE_TYPE_UINT16:
            return 2;
        case SHADER_ATTRIBUTE_TYPE_FLOAT32:
        case SHADER_ATTRIBUTE_TYPE_INT32:
        case SHADER_ATTRIBUTE_TYPE_UINT32:
            return 4;
        case SHADER_ATTRIBUTE_TYPE_FLOAT32_2:
            return 8;
        case SHADER_ATTRIBUTE_TYPE_FLOAT32_3:
            return 12;
        case SHADER_ATTRIBUTE_TYPE_FLOAT32_4:
            return 16;
        default:
            return 4;
    }
}
