#pragma once

#include "srBinOStream.h"
#include "srImporter.h"

/* Retail exports the full implicit lifecycle sweep for the importer/exporter
   classes, so they are dllexport under SURRENDER_BUILD; consumers keep the
   novtable-only surface. */
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
    HierarchyExporter() {}

    /* exportHierarchy's call site dispatches through vtable slot 2. */
    virtual void exportHierarchy(srBinOStream& stream, const ExportInfo& options) = 0;
};

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srModelIOManager::ModelExporter : public srIOManager::Exporter {
public:
    ModelExporter() {}

    /* exportModel's call site dispatches through vtable slot 2. */
    virtual void exportModel(srBinOStream& stream, srModel& model, const ExportInfo& options) = 0;
};
