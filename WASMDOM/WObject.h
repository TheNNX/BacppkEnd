#pragma once

#include <stdint.h>

#pragma pack(push, 1)

#define OBJ_BACKGROUND_ID 0

#ifdef __cplusplus
struct WPtrIndex;
#else
typedef uint32_t WPtrIndex;
#endif

#ifdef __cplusplus
extern "C" void* WRequestObject(uint32_t index);
extern "C" void  WReleaseObject(uint32_t index, void*);
struct WPtrIndex 
{
    uint32_t m_Index;

    template<typename T>
    T* Request()
    {
        return WRequestObject(m_Index);
    }

    template<typename T>
    void Release(void* object)
    {
        return WReleaseObject(m_Index, object);
    }
};
#else
void* WRequestObject(WPtrIndex index);
void  WReleaseObject(WPtrIndex index, void*);
#endif

typedef struct WObject
{

} WObject;

#pragma pack(pop)