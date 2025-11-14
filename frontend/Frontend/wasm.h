#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    extern void Log(const char* msg);
    extern void Eval(const char* jsExpression);

#ifdef __cplusplus
}
#endif