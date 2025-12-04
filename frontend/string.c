#include "string.h"

char* strcpy(char* dst, const char* src)
{
    char* o = dst;
    while (*src) *dst++ = *src++;
    *dst = 0;

    return o;
}

char* strcat(char* dst, const char* src)
{
    char* o = dst;

    while (*dst) dst++;
    strcpy(dst, src);

    return o;
}

size_t strlen(const char* str)
{
    size_t result = 0;
    
    while (*str)
    {
        str++;
        result++;
    }
    
    return result;
}
