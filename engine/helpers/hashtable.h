#pragma once

#include "defines.h"

typedef struct hashtable {
    u64 elementStride;
    u32 elementLength;
    b8 isPointerData;
    void* memory;
} hashtable;

typedef struct entry {
    char key[50];
    void* value;
} entry;

CT_API void hashtableCreate(u64 elementStride, u32 elementLength, void* memory, b8 isPointerData, hashtable* outHashtable);
CT_API void hashtableDestroy(hashtable* ht);

/** @brief Set a hashtable entry by using a name key; will overwrite; does not look for collisions or do any linear probing*/
CT_API b8 hashtableSet(hashtable* ht, const char* key, void* value);
CT_API b8 hashtableGet(hashtable* ht, const char* key, void* outValue);
CT_API b8 hashtableGetID(hashtable* ht, const char* key, u64* outValue);
CT_API void hashtableClear(hashtable* ht, const char* key);
