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
    /* Implicit lifecycle sweep emitted via the class-level dllexport. */
    // SYNTHETIC: SURRENDER 0x10005150
    // ??0SurfaceExporter@srSurfaceIOManager@@QAE@XZ
    // SYNTHETIC: SURRENDER 0x10005160
    // ??0SurfaceExporter@srSurfaceIOManager@@QAE@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x10005170
    // ??4SurfaceExporter@srSurfaceIOManager@@QAEAAV01@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x10005180
    // ??1SurfaceExporter@srSurfaceIOManager@@UAE@XZ

    virtual void exportSurface(srBinOStream& stream, srColorSurfaceIFace& surface,
                               const srSurfaceIOManager::ExportInfo& options) = 0;
};

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srHierarchyIOManager::HierarchyExporter : public srIOManager::Exporter {
public:
    // FUNCTION: SURRENDER 0x100164F0
    // ??0HierarchyExporter@srHierarchyIOManager@@QAE@XZ
    HierarchyExporter() {}

    /* Implicit copy ctor/assignment/destructor emitted via the class-level
       dllexport. */
    // SYNTHETIC: SURRENDER 0x10016500
    // ??0HierarchyExporter@srHierarchyIOManager@@QAE@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x10016510
    // ??4HierarchyExporter@srHierarchyIOManager@@QAEAAV01@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x10016520
    // ??1HierarchyExporter@srHierarchyIOManager@@UAE@XZ

    /* exportHierarchy's call site dispatches through vtable slot 2. */
    virtual void exportHierarchy(srBinOStream& stream, const ExportInfo& options) = 0;
};

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srModelIOManager::ModelExporter : public srIOManager::Exporter {
public:
    // FUNCTION: SURRENDER 0x10016A00
    // ??0ModelExporter@srModelIOManager@@QAE@XZ
    ModelExporter() {}

    /* Implicit copy ctor/assignment/destructor emitted via the class-level
       dllexport. */
    // SYNTHETIC: SURRENDER 0x10016A10
    // ??0ModelExporter@srModelIOManager@@QAE@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x10016A20
    // ??4ModelExporter@srModelIOManager@@QAEAAV01@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x10016A30
    // ??1ModelExporter@srModelIOManager@@UAE@XZ

    /* exportModel's call site dispatches through vtable slot 2. */
    virtual void exportModel(srBinOStream& stream, srModel& model, const ExportInfo& options) = 0;
};
