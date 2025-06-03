#include "hashtable.h"

#include "core/fmemory.h"

u64 hashName(const char* name, u32 elementCnt) {
    static const u64 multiplier = 97;

    unsigned const char* us;
    u64 hash = 0;

    for (us = (unsigned const char*)name; *us; us++) {
        hash = hash * multiplier + *us;
    }

    hash %= elementCnt;

    return hash;
}

void hashtableCreate(u64 elementStride, u32 elementCnt, void* memory,
                     hashtable* outHashtable) {
    // TODO: Might want to do the same thing as systems and require two create
    // FN calls. First to get memReq, second to actually create the hashtable
    outHashtable->memory = memory;
    outHashtable->elementCnt = elementCnt;
    outHashtable->elementStride = elementStride;
    fzeroMemory(outHashtable->memory, elementStride * elementCnt);
}

void hashtableDestroy(hashtable* table) {
    if (table) {
        // TODO: If using allocator above, use it here too.
        fzeroMemory(table, sizeof(hashtable));
    }
}

b8 hashtableSet(hashtable* table, const char* name, void* value) {
    u64 hash = hashName(name, table->elementCnt);
    fcopyMemory(table->memory + (table->elementStride * hash), value,
                table->elementStride);
    return true;
}

b8 hashtableGet(hashtable* table, const char* name, void* outValue) {
    u64 hash = hashName(name, table->elementCnt);
    fcopyMemory(outValue, table->memory + (table->elementStride * hash),
                table->elementStride);
    return true;
}

b8 hashtableFill(hashtable* table, void* value) {
    for (u32 i = 0; i < table->elementCnt; ++i) {
        fcopyMemory(table->memory + (table->elementStride * i), value,
                    table->elementStride);
    }
    return true;
}
