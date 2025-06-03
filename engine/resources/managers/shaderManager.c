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

    r->hasInstances = 0;
    r->hasLocals = 0;
    r->stageNames = dinoCreate(char*);
    r->stageFiles = dinoCreate(char*);
    r->renderpassName = 0;

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
        if (equalIdx == -1) {
            FERROR("Formating error in %s. Skipping Line.", fileLocation);
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
        } else if (strEqualI(tvar, "hasInstances")) {
            strToBool(tval, &r->hasInstances);
        } else if (strEqualI(tvar, "hasLocals")) {
            strToBool(tval, &r->hasLocals);
        } else if (strEqualI(tvar, "attribute")) {
            char** fields = dinoCreate(char*);
            u32 fieldAmt = strSplit(tval, ',', &fields, true, true);
            if (fieldAmt != 2) {
                FERROR("ShaderCfg %s: Incorrect attribute syntax", r->name);
                continue;
            }
            ShaderAttributeConfig at;
            // Parse field type
            if (strEqualI(fields[0], "f32")) {
                at.type = SHADER_ATTRIB_TYPE_FLOAT32;
                at.size = 4;
            } else if (strEqualI(fields[0], "vec2")) {
                at.type = SHADER_ATTRIB_TYPE_FLOAT32_2;
                at.size = 8;
            } else if (strEqualI(fields[0], "vec3")) {
                at.type = SHADER_ATTRIB_TYPE_FLOAT32_3;
                at.size = 12;
            } else if (strEqualI(fields[0], "vec4")) {
                at.type = SHADER_ATTRIB_TYPE_FLOAT32_4;
                at.size = 16;
            } else if (strEqualI(fields[0], "u8")) {
                at.type = SHADER_ATTRIB_TYPE_UINT8;
                at.size = 1;
            } else if (strEqualI(fields[0], "u16")) {
                at.type = SHADER_ATTRIB_TYPE_UINT16;
                at.size = 2;
            } else if (strEqualI(fields[0], "u32")) {
                at.type = SHADER_ATTRIB_TYPE_UINT32;
                at.size = 4;
            } else if (strEqualI(fields[0], "i8")) {
                at.type = SHADER_ATTRIB_TYPE_INT8;
                at.size = 1;
            } else if (strEqualI(fields[0], "i16")) {
                at.type = SHADER_ATTRIB_TYPE_INT16;
                at.size = 2;
            } else if (strEqualI(fields[0], "i32")) {
                at.type = SHADER_ATTRIB_TYPE_INT32;
                at.size = 4;
            } else {
                FERROR("ShaderCfg %s: Invalid attribute type used.", r->name);
                FWARN("Defaulting to f32.");
                at.type = SHADER_ATTRIB_TYPE_FLOAT32;
                at.size = 4;
            }
            at.name = strDup(fields[1]);
            at.nameLen = strLen(fields[1]);
            dinoPush(r->attributes, at);
            r->attributeCnt++;

            strCleanDinoArray(fields);
            dinoDestroy(fields);
        } else if (strEqualI(tvar, "uniform")) {
            char** fields = dinoCreate(char*);
            u32 fieldAmt = strSplit(tval, ',', &fields, true, true);
            if (fieldAmt != 3) {
                FERROR("ShaderCfg %s: Incorrect uniform syntax", r->name);
                continue;
            }
            ShaderUniformConfig un;
            // Parse field type
            if (strEqualI(fields[0], "f32")) {
                un.type = SHADER_UNIFORM_TYPE_FLOAT32;
                un.size = 4;
            } else if (strEqualI(fields[0], "vec2")) {
                un.type = SHADER_UNIFORM_TYPE_FLOAT32_2;
                un.size = 8;
            } else if (strEqualI(fields[0], "vec3")) {
                un.type = SHADER_UNIFORM_TYPE_FLOAT32_3;
                un.size = 12;
            } else if (strEqualI(fields[0], "vec4")) {
                un.type = SHADER_UNIFORM_TYPE_FLOAT32_4;
                un.size = 16;
            } else if (strEqualI(fields[0], "u8")) {
                un.type = SHADER_UNIFORM_TYPE_UINT8;
                un.size = 1;
            } else if (strEqualI(fields[0], "u16")) {
                un.type = SHADER_UNIFORM_TYPE_UINT16;
                un.size = 2;
            } else if (strEqualI(fields[0], "u32")) {
                un.type = SHADER_UNIFORM_TYPE_UINT32;
                un.size = 4;
            } else if (strEqualI(fields[0], "i8")) {
                un.type = SHADER_UNIFORM_TYPE_INT8;
                un.size = 1;
            } else if (strEqualI(fields[0], "i16")) {
                un.type = SHADER_UNIFORM_TYPE_INT16;
                un.size = 2;
            } else if (strEqualI(fields[0], "i32")) {
                un.type = SHADER_UNIFORM_TYPE_INT32;
                un.size = 4;
            } else if (strEqualI(fields[0], "mat4")) {
                un.type = SHADER_UNIFORM_TYPE_MATRIX_4;
                un.size = 64;
            } else if (strEqualI(fields[0], "sampler") ||
                       strEqualI(fields[0], "samp")) {
                un.type = SHADER_UNIFORM_TYPE_SAMPLER;
                un.size = 0;
                FINFO("Sampler read");
            } else {
                FERROR("ShaderCfg %s: Invalid uniform type used. %s", r->name,
                       fields[0]);
                FWARN("Defaulting to f32.");
                un.type = SHADER_UNIFORM_TYPE_FLOAT32;
                un.size = 4;
            }

            if (strEqualI(fields[1], "0")) {
                un.scope = SHADER_SCOPE_GLOBAL;
            } else if (strEqualI(fields[1], "1")) {
                un.scope = SHADER_SCOPE_INSTANCE;
            } else if (strEqualI(fields[1], "2")) {
                un.scope = SHADER_SCOPE_LOCAL;
            }

            un.name = strDup(fields[2]);
            un.nameLen = strLen(fields[2]);
            dinoPush(r->uniforms, un);
            r->uniformCnt++;

            strCleanDinoArray(fields);
            dinoDestroy(fields);
        }
        fzeroMemory(ln, sizeof(char) * 512);
    }
    fsClose(&f);
    FDEBUG("StageCnt: %d", r->stageCnt);
    outResource->data = r;
    outResource->dataSize = sizeof(ShaderRS);
    return true;
}

void shaderManagerUnload(resourceManager* self, Resource* resource) {
    ShaderRS* r = (ShaderRS*)resource->data;

    ffree(r->renderpassName, sizeof(char) * (strLen(r->renderpassName) + 1),
          MEMORY_TAG_STRING);
    ffree(r->name, sizeof(char) * (strLen(r->name) + 1), MEMORY_TAG_STRING);

    // Destroy stage Dinos. There filled with strings so use strCleanDinoArray
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
