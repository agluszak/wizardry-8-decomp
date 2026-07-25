#pragma once

#include "srImporter.h"

class srCore {
public:
    SR_DLL_IMPORT srSurfaceIOManager* getSurfaceIOManager() const;

    srRegistry* getRegistry() const { return registry_; }

private:
    unsigned char unknown_00_[0x2c];
    srRegistry* registry_;
};

extern SR_DLL_IMPORT class srCore srCore;
