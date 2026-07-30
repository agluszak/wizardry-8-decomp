#pragma once

#include "srBinOStream.h"
#include "srImporter.h"

class __declspec(novtable) srSurfaceIOManager::SurfaceExporter : public srIOManager::Exporter {
public:
    virtual void exportSurface(
        srBinOStream& stream,
        srColorSurfaceIFace& surface,
        const srSurfaceIOManager::ExportInfo& options) = 0;
};
