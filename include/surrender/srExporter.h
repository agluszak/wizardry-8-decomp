#pragma once

#include "srBinOStream.h"
#include "srImporter.h"

/* Provider lifecycle symbols are exported. The reconstruction uses implicit
   base-only lifecycle; original declaration spelling is unresolved. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srSurfaceIOManager::SurfaceExporter : public srIOManager::Exporter {
public:
    /* Exported lifecycle bodies contain ordinary base-only operations. */

    virtual void exportSurface(srBinOStream& stream, srColorSurfaceIFace& surface,
                               const srSurfaceIOManager::ExportInfo& options) = 0;
};

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srHierarchyIOManager::HierarchyExporter : public srIOManager::Exporter {
public:

    /* Exported lifecycle bodies are consistent with ordinary base-only operations. */

    /* exportHierarchy's call site dispatches through vtable slot 2. */
    virtual void exportHierarchy(srBinOStream& stream, const ExportInfo& options) = 0;
};

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srModelIOManager::ModelExporter : public srIOManager::Exporter {
public:

    /* Exported lifecycle bodies are consistent with ordinary base-only operations. */

    /* exportModel's call site dispatches through vtable slot 2. */
    virtual void exportModel(srBinOStream& stream, srModel& model, const ExportInfo& options) = 0;
};
