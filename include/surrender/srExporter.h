#pragma once

#include "srBinOStream.h"
#include "srImporter.h"

class srIOManager::Exporter {
public:
    virtual const char* getTypeName() const = 0;
    virtual ~Exporter() {}

protected:
    SR_DLL_IMPORT void addToExporters(srIOManager* manager, const char* extension);
    SR_DLL_IMPORT void addToExporters(
        srIOManager* manager,
        srIOManager::Exporter* exporter,
        const char* extension);
    SR_DLL_IMPORT void removeFromExporters(srIOManager* manager);
};

class srSurfaceIOManager::SurfaceExporter : public srIOManager::Exporter {
public:
    virtual void exportSurface(
        srBinOStream& stream,
        srColorSurfaceIFace& surface,
        const srSurfaceIOManager::ExportInfo& options) = 0;
};
