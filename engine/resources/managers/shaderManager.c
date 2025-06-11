#include "shaderManager.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "defines.h"
#include "helpers/dinoarray.h"
#include "platform/filesystem.h"
#include "resources/resourceManager.h"
#include "resources/resourcesTypes.h"

b8 shaderManagerLoad(resourceManager* self, const char* name,
                     Resource* outResource) {
    char* fmtStr = "%s%s";
    char fileLocation[512];
    strFmt(fileLocation, fmtStr, resourceManagerRootAssetPath(), name);
    FDEBUG("File Location: %s", fileLocation)
    fileHandle f;
    if (!fsOpen(fileLocation, FILE_MODE_READ, false, &f)) {
        FERROR("Could not open file: %s", fileLocation);
        return false;
    }
    outResource->fullPath = strDup(fileLocation);

    ShaderRS* r = fallocate(sizeof(ShaderRS), MEMORY_TAG_RESOURCE);
    r->name = 0;
    r->stages = dinoCreate(ShaderStage);
    r->stageCnt = 0;

    r->supportsInstances = 0;
    r->supportsLocals = 0;
    r->stageNames = dinoCreate(char*);
    r->stageFiles = dinoCreate(char*);
    r->renderpassName = 0;

    b8 attributeStarted = false;
    b8 uniformStarted = false;

    char line[512] = "";
    char* p = &line[0];
    u64 lineLen = 0;
    while (fsReadLine(&f, 511, &p, &lineLen)) {
        char* ln = strTrim(line);
        lineLen = strLen(ln);

        if (lineLen < 1 || ln[0] == '#') {
            continue;
        }

        i32 equalIdx = strIdxOf(ln, '=');
        if (equalIdx == -1 && !uniformStarted && !attributeStarted) {
            if (strEqualI(ln, "UNIFORMS")){
                uniformStarted = true;
                attributeStarted = false;
            }else if (strEqualI(ln, "ATTRIBUTES")){
                uniformStarted = false;
                attributeStarted = true;
            }else{
                FERROR("Formating error in %s. No '=' Found, Skipping Line.", fileLocation);
            }
            continue;
        }

        char varName[64];
        fzeroMemory(varName, sizeof(char) * 64);
        strCut(varName, ln, 0, equalIdx);
        char* tvar = strTrim(varName);

        char value[446];
        fzeroMemory(value, sizeof(char) * 446);
        strCut(value, ln, equalIdx + 1, -1);
        char* tval = strTrim(value);

        if (strEqualI(tvar, "Name")) {
            r->name = strDup(tval);
        } else if (strEqualI(tvar, "renderpass")) {
            r->renderpassName = strDup(tval);
        } else if (strEqualI(tvar, "stages")) {
            r->stageCnt = strSplit(tval, ',', &r->stageNames, true, true);
            for (u8 i = 0; i < r->stageCnt; i++) {
                if (strSub(r->stageNames[i], "frag")) {
                    dinoPush(r->stages, SHADER_STAGE_FRAGMENT);
                } else if (strSub(r->stageNames[i], "vert")) {
                    dinoPush(r->stages, SHADER_STAGE_VERTEX);
                } else if (strSub(r->stageNames[i], "geo")) {
                    dinoPush(r->stages, SHADER_STAGE_GEOMETRY);
                } else if (strSub(r->stageNames[i], "comp")) {
                    dinoPush(r->stages, SHADER_STAGE_COMPUTE);
                }
            }
        } else if (strEqualI(tvar, "stagefiles")) {
            r->stageCnt = strSplit(tval, ',', &r->stageFiles, true, true);
        } else if (strEqualI(tvar, "supportsInstances")) {
            strToBool(tval, &r->supportsInstances);
        } else if (strEqualI(tvar, "supportsLocals")) {
            strToBool(tval, &r->supportsLocals);
        } else if (strEqualI(tvar, "ATTRIBUTES")) {
            attributeStarted = true;
            uniformStarted = false;
        } else if (strEqualI(tvar, "UNIFORMS")) {
            attributeStarted = false;
            uniformStarted = true;
        } else {
            if (attributeStarted) {
                if (!r->attributes){
                    r->attributes = dinoCreate(ShaderAttributeConfig);
                }
                char** fields = dinoCreate(char*);
                u32 fieldAmt = strSplit(tval, ' ', &fields, true, true);
                if (fieldAmt != 2) {
                    FERROR("ShaderCfg %s: Incorrect attribute syntax", r->name);
                    continue;
                }
                ShaderAttributeConfig at;

                at.name = strDup(fields[0]);
                at.nameLen = strLen(fields[0]);

                // Parse field type
                if (strEqualI(fields[1], "i8")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_INT8;
                    at.size = 1;
                } else if (strEqualI(fields[1], "i16")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_INT16;
                    at.size = 2;
                } else if (strEqualI(fields[1], "i32")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_INT32;
                    at.size = 4;
                } else if (strEqualI(fields[1], "u8")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_UINT8;
                    at.size = 1;
                } else if (strEqualI(fields[1], "u16")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_UINT16;
                    at.size = 2;
                } else if (strEqualI(fields[1], "u32")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_UINT32;
                    at.size = 4;
                } else if (strEqualI(fields[1], "f32")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_FLOAT32;
                    at.size = 4;
                } else if (strEqualI(fields[1], "vec2")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_FLOAT32_2;
                    at.size = 8;
                } else if (strEqualI(fields[1], "vec3")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_FLOAT32_3;
                    at.size = 12;
                } else if (strEqualI(fields[1], "vec4")) {
                    at.type = SHADER_ATTRIBUTE_TYPE_FLOAT32_4;
                    at.size = 16;
                } else {
                    FERROR("ShaderCfg %s: Invalid attribute type used.",
                           r->name);
                    FWARN("Defaulting to f32.");
                    at.type = SHADER_ATTRIBUTE_TYPE_FLOAT32;
                    at.size = 4;
                }
                dinoPush(r->attributes, at);
                r->attributeCnt++;

                strCleanDinoArray(fields);
                dinoDestroy(fields);
            } else if (uniformStarted) {
                if (!r->uniforms){
                    r->uniforms = dinoCreate(ShaderUniformConfig);
                }
                char** fields = dinoCreate(char*);
                u32 fieldAmt = strSplit(tval, ' ', &fields, true, true);
                if (fieldAmt != 3) {
                    FERROR("ShaderCfg %s: Incorrect uniform syntax", r->name);
                    continue;
                }
                ShaderUniformConfig un;
                // Parse field type
                un.name = strDup(fields[0]);
                un.nameLen = strLen(fields[0]);

                if (strEqualI(fields[1], "3")) {
                    un.scope = SHADER_SCOPE_GLOBAL;
                } else if (strEqualI(fields[1], "2")) {
                    un.scope = SHADER_SCOPE_INSTANCE;
                } else if (strEqualI(fields[1], "1")) {
                    un.scope = SHADER_SCOPE_LOCAL;
                }

                if (strEqualI(fields[2], "i8")) {
                    un.type = SHADER_UNIFORM_TYPE_INT8;
                    un.size = 1;
                } else if (strEqualI(fields[2], "i16")) {
                    un.type = SHADER_UNIFORM_TYPE_INT16;
                    un.size = 2;
                } else if (strEqualI(fields[2], "i32")) {
                    un.type = SHADER_UNIFORM_TYPE_INT32;
                    un.size = 4;
                } else if (strEqualI(fields[2], "u8")) {
                    un.type = SHADER_UNIFORM_TYPE_UINT8;
                    un.size = 1;
                } else if (strEqualI(fields[2], "u16")) {
                    un.type = SHADER_UNIFORM_TYPE_UINT16;
                    un.size = 2;
                } else if (strEqualI(fields[2], "u32")) {
                    un.type = SHADER_UNIFORM_TYPE_UINT32;
                    un.size = 4;
                } else if (strEqualI(fields[2], "f32")) {
                    un.type = SHADER_UNIFORM_TYPE_FLOAT32;
                    un.size = 4;
                } else if (strEqualI(fields[2], "vec2")) {
                    un.type = SHADER_UNIFORM_TYPE_FLOAT32_2;
                    un.size = 8;
                } else if (strEqualI(fields[2], "vec3")) {
                    un.type = SHADER_UNIFORM_TYPE_FLOAT32_3;
                    un.size = 12;
                } else if (strEqualI(fields[2], "vec4")) {
                    un.type = SHADER_UNIFORM_TYPE_FLOAT32_4;
                    un.size = 16;
                } else if (strEqualI(fields[2], "mat4")) {
                    un.type = SHADER_UNIFORM_TYPE_MATRIX_4;
                    un.size = 64;
                } else if (strEqualI(fields[2], "sampler") ||
                           strEqualI(fields[2], "samp")) {
                    un.type = SHADER_UNIFORM_TYPE_SAMPLER;
                    un.size = 0;
                    FINFO("Sampler read");
                } else {
                    FERROR("ShaderCfg %s: Invalid uniform type used. %s",
                           r->name, fields[2]);
                    FWARN("Defaulting to f32.");
                    un.type = SHADER_UNIFORM_TYPE_FLOAT32;
                    un.size = 4;
                }

                dinoPush(r->uniforms, un);
                r->uniformCnt++;

                strCleanDinoArray(fields);
                dinoDestroy(fields);
            }
        }
        fzeroMemory(ln, sizeof(char) * 512);
    }
    fsClose(&f);
    outResource->data = r;
    outResource->dataSize = sizeof(ShaderRS);
    return true;
}

void shaderManagerUnload(resourceManager* self, Resource* resource) {
    ShaderRS* r = (ShaderRS*)resource->data;

    ffree(r->renderpassName, sizeof(char) * (strLen(r->renderpassName) + 1),
          MEMORY_TAG_STRING);
    ffree(r->name, sizeof(char) * (strLen(r->name) + 1), MEMORY_TAG_STRING);

    // Destroy stage Dinos. There filled with strings so use
    // strCleanDinoArray
    strCleanDinoArray(r->stageFiles);
    strCleanDinoArray(r->stageNames);
    dinoDestroy(r->stageFiles);
    dinoDestroy(r->stageNames);
    dinoDestroy(r->stages);

    // Destroy attributes/uniforms Dinos
    if (r->attributes) {
        u32 c = dinoLength(r->attributes);
        for (u32 i = 0; i < c; i++) {
            ffree(r->attributes[i].name,
                  sizeof(char) * (strLen(r->attributes[i].name) + 1),
                  MEMORY_TAG_STRING);
        }
        dinoDestroy(r->attributes);
    }

    if (r->uniforms) {
        u32 c = dinoLength(r->uniforms);
        for (u32 i = 0; i < c; i++) {
            ffree(r->uniforms[i].name,
                  sizeof(char) * (strLen(r->uniforms[i].name) + 1),
                  MEMORY_TAG_STRING);
        }
        dinoDestroy(r->uniforms);
    }

    // Zero out the resource memory
    fzeroMemory(r, sizeof(ShaderRS));

    ffree(resource->fullPath, sizeof(char) * (strLen(resource->fullPath) + 1),
          MEMORY_TAG_STRING);
    ffree(resource->data, resource->dataSize, MEMORY_TAG_RESOURCE);
    resource->data = 0;
    resource->dataSize = 0;
    resource->managerID = INVALID_ID;
}

resourceManager shaderManagerCreate() {
    resourceManager m;
    m.resourceType = RESOURCE_TYPE_SHADER;
    m.id = RESOURCE_TYPE_SHADER;
    m.load = shaderManagerLoad;
    m.unload = shaderManagerUnload;
    return m;
}
