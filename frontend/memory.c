#include "stdlib.h"

void* memset(void* dst, int ch, size_t size)
{
    size_t i;

    for (i = 0; i < size; i++)
    {
        ((char*)dst)[i] = (char)ch;
    }

    return dst;
}

void* memcpy(void* dst, void* src, size_t size)
{
    size_t i;

    for (i = 0; i < size; i++)
    {
        ((char*)dst)[i] = ((char*)src)[i];
    }

    return dst;
}
