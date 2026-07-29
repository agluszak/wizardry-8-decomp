#pragma once

#include "srCore.h"

class SR_DLL_IMPORT srExtension {
public:
    static srExtension* load(const char* name, const char* path);
};
