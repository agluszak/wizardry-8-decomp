#pragma once

#include "srImporter.h"
#include "srTypeRegistry.h"

class srIStreamOpener;

class srCore {
public:
    SR_DLL_IMPORT srSurfaceIOManager* getSurfaceIOManager() const;
    SR_DLL_IMPORT srIStreamOpener* getIStreamOpener() const;

    srRegistry* getRegistry() const { return registry_; }

private:
    unsigned char unknown_00_[0x2c];
    srRegistry* registry_;
};

extern SR_DLL_IMPORT class srCore srCore;
