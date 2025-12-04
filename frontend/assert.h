#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    void _assert(const char* message, const char* file, int line);
#define assert(expression) ((void)((!!(expression)) || (_assert(#expression, __FILE__, __LINE__), 0)))

#ifdef __cplusplus
}
#endif