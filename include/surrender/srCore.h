#pragma once

#include "srImporter.h"

class srCore {
public:
    SR_DLL_IMPORT srSurfaceIOManager* getSurfaceIOManager() const;
};

extern SR_DLL_IMPORT class srCore srCore;
