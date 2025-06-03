#pragma once

#include "defines.h"
#include "math/matrixMath.h"

// Returns the length of the given str.
CT_API u64 strLen(const char* str);

CT_API char* strDup(const char* str);

CT_API char* strSub(const char* str, const char* sub);

CT_API b8 strEqual(const char* str0, const char* str1);

CT_API b8 strEqualI(const char* str0, const char* str1);

CT_API i32 strFmtV(char* dest, const char* format, void* vaListp);

CT_API i32 strFmt(char* dest, const char* format, ...);
/**
 * @brief If a negative value is passed, proceed to the end of the str.
 */
CT_API void strCut(char* dest, const char* source, i32 start, i32 length);

CT_API char* strCpy(char* dest, const char* source);

CT_API char* strNCpy(char* txt, const char* src, u64 len);

/**
 * @brief Empties the provided string by setting the first character to 0.
 *
 * @param str The string to be emptied.
 * @return A pointer to str.
 */
CT_API char* strEmpty(char* str);

CT_API char* strTrim(char* str);

CT_API u32 strSplit(const char* str, char delimeter, char*** strDinoArray,
                    b8 trimIt, b8 includeZeroCharLines);

CT_API void strCleanDinoArray(char** a);

CT_API i32 strIdxOf(const char* str, char c);

CT_API b8 strToMat4(const char* str, mat4* outMat);

CT_API b8 strToVec4(const char* str, vector4* outVector);

CT_API b8 strToVec3(const char* str, vector3* outVector);

CT_API b8 strToVec2(const char* str, vector2* outVector);

CT_API b8 strToF32(const char* str, f32* f);

CT_API b8 strToF64(const char* str, f64* f);

CT_API b8 strToI8(const char* str, i8* i);

CT_API b8 strToI16(const char* str, i16* i);

CT_API b8 strToI32(const char* str, i32* i);

CT_API b8 strToI64(const char* str, i64* i);

CT_API b8 strToU8(const char* str, u8* u);

CT_API b8 strToU16(const char* str, u16* u);

CT_API b8 strToU32(const char* str, u32* u);

CT_API b8 strToU64(const char* str, u64* u);

CT_API b8 strToBool(const char* str, b8* b);
