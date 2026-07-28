#pragma once

#include "srHeap.h"

class srConfig {
public:
    SR_DLL_IMPORT void set(const char* name, const char* value);
    SR_DLL_IMPORT int exists(const char* name) const;
    SR_DLL_IMPORT int getBool(const char* name) const;
    SR_DLL_IMPORT const char* get(const char* name) const;
};

extern SR_DLL_IMPORT class srConfig srConfig;
