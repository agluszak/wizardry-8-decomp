#pragma once

#include "srHeap.h"

class srIStreamOpener;
class srRegistry;
class srSurfaceIOManager;

class srCore {
public:
    SR_DLL_IMPORT srSurfaceIOManager* getSurfaceIOManager() const;
    SR_DLL_IMPORT srIStreamOpener* getIStreamOpener() const;
    SR_DLL_IMPORT const char* getCopyright() const;
    SR_DLL_IMPORT const char* getVersion() const;

    srRegistry* getRegistry() const { return registry_; }

private:
    unsigned char unknown_00_[0x2c];
    srRegistry* registry_;
    unsigned char unknown_30_[0x0c];
    char version_[0x20];
    char copyright_[1];
};

extern SR_DLL_IMPORT class srCore srCore;
