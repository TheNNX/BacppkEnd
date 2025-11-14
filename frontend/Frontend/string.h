#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    char* strcpy(char* dst, const char* src);
    char* strcat(char* dst, const char* src);
    size_t strlen(const char* str);

#ifdef __cplusplus
}
#endif