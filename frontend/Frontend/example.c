// #include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include "stdlib.h"
#include "stdio.h"

#include "assert.h"
#include "string.h"

#define WASM_EXPORT __attribute__((visibility("default")))

#include "../../WASMDOM/WObject.h"

void* g_ObjectTable[256];

WASM_EXPORT void* WRequestObject(WPtrIndex index)
{
    void* result;
    
    /* TODO */
    assert(g_ObjectTable[index] != NULL);
    
    result = g_ObjectTable[index];
    g_ObjectTable[index] = NULL;

    return result;
}

WASM_EXPORT void WReleaseObject(WPtrIndex index, void* ptr)
{
    /* TODO */
    assert(g_ObjectTable[index] == NULL);
    g_ObjectTable[index] = ptr;
}

WASM_EXPORT int TestExport(int c)
{
    assert(1 == 0);
    return c;
}

WASM_EXPORT int main()
{
    memset(g_ObjectTable, 0, sizeof(g_ObjectTable));

    printf("Test\n");

    printf("%llx %lx ", 0xFEDCBA9876543210, 0xFEDCBA9876543210);
    printf("%llx %lx %x ", -1, -1, -1);
    printf("%llx %lx %x ", -1L, -1L, -1L);
    printf("%llx %lx %x\n", -1LL, -1LL, -1LL);

    return 0;
}
