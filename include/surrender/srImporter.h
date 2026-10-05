#pragma once

#include "srBinIStream.h"
#include "srBinOStream.h"
#include "srColorSurface.h"
#include "srIOManager.h"
#include "srOptionList.h"

class srBinOStream;
class srModel;

/* Provider exports include lifecycle symbols and the vftable. The reconstruction
   uses implicit base-only lifecycle; original declaration spelling is unresolved. */
// VTABLE: SURRENDER 0x10075418
// class srSurfaceIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srSurfaceIOManager : public srIOManager {
public:
    /* Retail callers build the import options as a single zeroed dword:
       srPalette::Sampler::addSurface reuses the dead name argument slot and
       srTextureFile::loadSurface reserves one dword local. ExportInfo keeps
       the wider triple (Wizardry's SaveJpegScreenshot proves {0,1,"QUALITY=..."}). */
    struct ImportInfo {
        unsigned long unknown_00;
    };

    struct ExportInfo {
        unsigned long unknown_00;
        unsigned long unknown_04;
        const char* option_string;
    };

    class SurfaceImporter;
    class SurfaceExporter;

    /* The default-constructor body contains base construction and a derived
       vftable store, consistent with implicit construction. */

    /* Provider-side entry (0x1002DCD0); no consumer import evidence, so it
       stays unannotated. */
    void exportSurface(const char* path, srBinOStream& stream, srColorSurfaceIFace& surface,
                       const ExportInfo& options);
    SR_DLL_IMPORT void exportSurface(const char* path, srColorSurfaceIFace& surface,
                                     const ExportInfo& options);
    /* Provider-side import entries (0x1002DB20/0x1002DA00); no consumer import
       evidence, so they stay unannotated. */
    srColorSurfaceIFace* importSurface(const char* path, srBinIStream& stream,
                                       const ImportInfo& options);
    srColorSurfaceIFace* importSurface(const char* path, const ImportInfo& options);
    /* Provider-side entries (0x1002D890/0x1002D8C0/0x1002D8F0); no consumer
       import evidence, so they stay unannotated. */
    SurfaceImporter* getImporter(const char* path);
    SurfaceExporter* getExporter(const char* path);
    void getSurfaceDesc(srColorSurfaceIFace::SurfaceDesc& description, const char* path,
                        const ImportInfo& options);

    /* The emitted lifecycle is consistent with ordinary base-subobject operations. */
};

static_assert((sizeof(srSurfaceIOManager::ImportInfo) == 0x04), "srSurfaceImportInfo_must_be_0x04");
static_assert((sizeof(srSurfaceIOManager::ExportInfo) == 0x0c), "srSurfaceExportInfo_must_be_0x0c");

/* Provider lifecycle symbols are exported. The reconstruction uses implicit
   base-only lifecycle; original declaration spelling is unresolved. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srSurfaceIOManager::SurfaceImporter : public srIOManager::Importer {
public:
    /* Exported lifecycle bodies contain ordinary base-only operations. */

    virtual int getSurfaceDesc(srColorSurfaceIFace::SurfaceDesc& description, srBinIStream& stream,
                               const srSurfaceIOManager::ImportInfo& options);
    virtual srColorSurfaceIFace* importSurface(srBinIStream& stream,
                                               const srSurfaceIOManager::ImportInfo& options) = 0;
};

/* Provider exports include lifecycle symbols and the vftable. The reconstruction
   uses implicit base-only lifecycle; original declaration spelling is unresolved. */
// VTABLE: SURRENDER 0x10075530 srHierarchyIOManager
// class srHierarchyIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srHierarchyIOManager : public srIOManager {
public:
    /* The provider exports Info assignments at 0x10016490 and 0x100164A0. */
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ImportInfo {
    public:
        unsigned char unknown_00;
    };
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ExportInfo {
    public:
        unsigned char unknown_00;
    };

    class HierarchyImporter;
    class HierarchyExporter;

    /* No state beyond srIOManager is modeled; exported lifecycle bodies contain
       ordinary base-only operations. */

    void importHierarchy(const char* path, const ImportInfo& options);
    void exportHierarchy(const char* path, const ExportInfo& options);
};

static_assert((sizeof(srHierarchyIOManager) == 0x1c), "srHierarchyIOManager_must_be_0x1c");
static_assert((sizeof(srHierarchyIOManager::ImportInfo) == 0x01),
              "srHierarchyImportInfo_must_be_0x01");
static_assert((sizeof(srHierarchyIOManager::ExportInfo) == 0x01),
              "srHierarchyExportInfo_must_be_0x01");

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srHierarchyIOManager::HierarchyImporter : public srIOManager::Importer {
public:
    /* Exported lifecycle bodies contain ordinary base-only operations. */

    /* importHierarchy's call site dispatches through vtable slot 2. */
    virtual void importHierarchy(srBinIStream& stream, const ImportInfo& options) = 0;
};

/* Provider exports include lifecycle symbols and the vftable. The reconstruction
   uses implicit base-only lifecycle; original declaration spelling is unresolved. */
// VTABLE: SURRENDER 0x10075534 srModelIOManager
// class srModelIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srModelIOManager : public srIOManager {
public:
    /* The provider exports Info assignments at 0x100169A0 and 0x100169B0. */
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ImportInfo {
    public:
        unsigned char unknown_00;
    };
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ExportInfo {
    public:
        unsigned char unknown_00;
    };

    class ModelImporter;
    class ModelExporter;

    /* No state beyond srIOManager is modeled; exported lifecycle bodies contain
       ordinary base-only operations. */

    srModel* importModel(const char* path, const ImportInfo& options);
    void exportModel(const char* path, srModel& model, const ExportInfo& options);
};

static_assert((sizeof(srModelIOManager) == 0x1c), "srModelIOManager_must_be_0x1c");
static_assert((sizeof(srModelIOManager::ImportInfo) == 0x01), "srModelImportInfo_must_be_0x01");
static_assert((sizeof(srModelIOManager::ExportInfo) == 0x01), "srModelExportInfo_must_be_0x01");

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srModelIOManager::ModelImporter : public srIOManager::Importer {
public:
    /* Exported lifecycle bodies contain ordinary base-only operations. */

    /* importModel's call site dispatches through vtable slot 2. */
    virtual srModel* importModel(srBinIStream& stream, const ImportInfo& options) = 0;
};
