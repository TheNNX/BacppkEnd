#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void* memset(void* dst, int ch, size_t size);
    void* memcpy(void* dst, void* src, size_t size);

#ifdef __cplusplus
}
#endif