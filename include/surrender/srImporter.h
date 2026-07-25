#pragma once

#include "srBinIStream.h"
#include "srColorSurface.h"
#include "srOptionList.h"

class srIOManager {
public:
    class Importer;
    class Exporter;
};

class srIOManager::Importer {
public:
    virtual const char* getTypeName() const = 0;
    virtual ~Importer() {}

protected:
    SR_DLL_IMPORT void addToImporters(srIOManager* manager, const char* extension);
    SR_DLL_IMPORT void addToImporters(
        srIOManager* manager,
        srIOManager::Importer* importer,
        const char* extension);
    SR_DLL_IMPORT void removeFromImporters(srIOManager* manager);
};

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

typedef char srSurfaceImportInfo_must_be_0x0c[
    (sizeof(srSurfaceIOManager::ImportInfo) == 0x0c) ? 1 : -1];
typedef char srSurfaceExportInfo_must_be_0x0c[
    (sizeof(srSurfaceIOManager::ExportInfo) == 0x0c) ? 1 : -1];

class srSurfaceIOManager::SurfaceImporter : public srIOManager::Importer {
public:
    virtual SR_DLL_IMPORT int getSurfaceDesc(
        srColorSurfaceIFace::SurfaceDesc& description,
        srBinIStream& stream,
        const srSurfaceIOManager::ImportInfo& options);
    virtual srColorSurfaceIFace* importSurface(
        srBinIStream& stream,
        const srSurfaceIOManager::ImportInfo& options) = 0;
};
