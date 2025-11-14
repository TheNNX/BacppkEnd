#pragma once

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif
    int sprintf(char* buffer, const char* format, ...);
    int snprintf(char* buffer, size_t size, const char* format, ...);
    int printf(const char* format, ...);

    int vsprintf(char* buffer, const char* format, va_list args);
    int vsnprintf(char* buffer, size_t size, const char* format, va_list args);
    int vprintf(const char* format, va_list args);
#ifdef __cplusplus
}
#endif