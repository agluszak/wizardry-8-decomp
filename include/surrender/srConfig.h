#pragma once

#include <iostream>

#include "srHeap.h"

class srConfig {
public:
    SR_DLL_IMPORT srConfig();
    SR_DLL_IMPORT ~srConfig();

    SR_DLL_IMPORT void append(const char* name, const char* value);
    SR_DLL_IMPORT void dump(std::ostream& stream);
    SR_DLL_IMPORT int exists(const char* name) const;
    SR_DLL_IMPORT const char* get(const char* name) const;
    SR_DLL_IMPORT int getBool(const char* name) const;
    SR_DLL_IMPORT float getFloat(const char* name) const;
    SR_DLL_IMPORT long getLong(const char* name) const;
    SR_DLL_IMPORT void remove(const char* name);
    SR_DLL_IMPORT void removeAll();
    SR_DLL_IMPORT void set(const char* name, const char* value);
    SR_DLL_IMPORT void setBool(const char* name, int value);
    SR_DLL_IMPORT void setFloat(const char* name, float value);
    SR_DLL_IMPORT void setLong(const char* name, long value);

private:
    unsigned char unknown_00_[0x1c];
};

static_assert(sizeof(srConfig) == 0x1c, "srConfig_must_be_0x1c");

extern SR_DLL_IMPORT class srConfig srConfig;
