#pragma once

#include "srBinOStream.h"
#include "srImporter.h"

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srSurfaceIOManager::SurfaceExporter : public srIOManager::Exporter {
public:
    virtual void exportSurface(srBinOStream& stream, srColorSurfaceIFace& surface,
                               const srSurfaceIOManager::ExportInfo& options) = 0;
};

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srHierarchyIOManager::HierarchyExporter : public srIOManager::Exporter {
public:
    virtual void exportHierarchy(srBinOStream& stream, const ExportInfo& options) = 0;
};

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srModelIOManager::ModelExporter : public srIOManager::Exporter {
public:
    virtual void exportModel(srBinOStream& stream, srModel& model, const ExportInfo& options) = 0;
};
