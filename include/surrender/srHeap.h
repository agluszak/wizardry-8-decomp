#pragma once

#if defined(_MSC_VER)
#define SR_DLL_IMPORT __declspec(dllimport)
#else
#define SR_DLL_IMPORT
#endif

class srHeap {
public:
    SR_DLL_IMPORT void* allocate(unsigned long size);
    SR_DLL_IMPORT void free(void* allocation);
};

extern SR_DLL_IMPORT class srHeap srHeap;
