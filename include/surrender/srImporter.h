#pragma once

#include "srBinIStream.h"
#include "srColorSurface.h"
#include "srIOManager.h"
#include "srOptionList.h"

class srSurfaceIOManager : public srIOManager {
public:
    struct ImportInfo {
        unsigned long unknown_00;
        unsigned long unknown_04;
        const char* option_string;
    };

    struct ExportInfo {
        unsigned long unknown_00;
        unsigned long unknown_04;
        const char* option_string;
    };

    class SurfaceImporter;
    class SurfaceExporter;
};

static_assert((sizeof(srSurfaceIOManager::ImportInfo) == 0x0c), "srSurfaceImportInfo_must_be_0x0c");
static_assert((sizeof(srSurfaceIOManager::ExportInfo) == 0x0c), "srSurfaceExportInfo_must_be_0x0c");

class __declspec(novtable) srSurfaceIOManager::SurfaceImporter : public srIOManager::Importer {
public:
    virtual SR_DLL_IMPORT int getSurfaceDesc(
        srColorSurfaceIFace::SurfaceDesc& description,
        srBinIStream& stream,
        const srSurfaceIOManager::ImportInfo& options);
    virtual srColorSurfaceIFace* importSurface(
        srBinIStream& stream,
        const srSurfaceIOManager::ImportInfo& options) = 0;
};
