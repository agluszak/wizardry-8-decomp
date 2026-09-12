#pragma once

#include "srBinOStream.h"
#include "srImporter.h"

class __declspec(novtable) srSurfaceIOManager::SurfaceExporter : public srIOManager::Exporter {
public:
    virtual void exportSurface(srBinOStream& stream, srColorSurfaceIFace& surface,
                               const srSurfaceIOManager::ExportInfo& options) = 0;
};

class __declspec(novtable) srHierarchyIOManager::HierarchyExporter : public srIOManager::Exporter {
public:
    HierarchyExporter() {}
};

class __declspec(novtable) srModelIOManager::ModelExporter : public srIOManager::Exporter {
public:
    ModelExporter() {}
};
