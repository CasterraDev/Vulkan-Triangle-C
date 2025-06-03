// TODO: Make a better more advanced hashtable with linear collision and more options.
// Also has the entry struct to find invalid spots faster

#pragma once

#include "defines.h"

// typedef struct entry {
//     char key[50];
//     void* value;
// } entry;

typedef struct hashtable {
    u64 elementStride;
    u32 elementCnt;
    void* memory;
} hashtable;

CT_API void hashtableCreate(u64 elementStride, u32 elementCnt, void* memory, hashtable* outHashtable);
CT_API void hashtableDestroy(hashtable* table);

CT_API b8 hashtableSet(hashtable* table, const char* name, void* value);
CT_API b8 hashtableGet(hashtable* table, const char* name, void* outValue);
CT_API b8 hashtableFill(hashtable* table, void* value);
