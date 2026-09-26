#pragma once

#include "srBinIStream.h"
#include "srBinOStream.h"
#include "srColorSurface.h"
#include "srIOManager.h"
#include "srOptionList.h"

class srBinOStream;
class srModel;

/* The provider exports the full member surface including the implicit
   lifecycle sweep (ctor, copy ctor, assignment, destructor) and the vftable, so
   the declaration is dllexport under SURRENDER_BUILD. */
// FUNCTION: SURRENDER 0x100050F0 SYNTHETIC
// ??0srSurfaceIOManager@@QAE@XZ
// VTABLE: SURRENDER 0x10075418
// class srSurfaceIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srSurfaceIOManager : public srIOManager {
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

    /* Implicit copy ctor/assignment/destructor emitted via the class-level
       dllexport as memberwise base-subobject copies. */
    // SYNTHETIC: SURRENDER 0x10005190
    // ??0srSurfaceIOManager@@QAE@ABV0@@Z
    // SYNTHETIC: SURRENDER 0x100051D0
    // ??4srSurfaceIOManager@@QAEAAV0@ABV0@@Z
    // SYNTHETIC: SURRENDER 0x10005210
    // ??1srSurfaceIOManager@@UAE@XZ

};

static_assert((sizeof(srSurfaceIOManager::ImportInfo) == 0x0c), "srSurfaceImportInfo_must_be_0x0c");
static_assert((sizeof(srSurfaceIOManager::ExportInfo) == 0x0c), "srSurfaceExportInfo_must_be_0x0c");

/* Retail exports the full implicit lifecycle sweep for the importer/exporter
   classes, so they are dllexport under SURRENDER_BUILD; consumers keep the
   novtable-only surface. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srSurfaceIOManager::SurfaceImporter : public srIOManager::Importer {
public:
    /* Implicit lifecycle sweep emitted via the class-level dllexport. */
    // SYNTHETIC: SURRENDER 0x10005110
    // ??0SurfaceImporter@srSurfaceIOManager@@QAE@XZ
    // SYNTHETIC: SURRENDER 0x10005120
    // ??0SurfaceImporter@srSurfaceIOManager@@QAE@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x10005130
    // ??4SurfaceImporter@srSurfaceIOManager@@QAEAAV01@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x10005140
    // ??1SurfaceImporter@srSurfaceIOManager@@UAE@XZ

    virtual int getSurfaceDesc(srColorSurfaceIFace::SurfaceDesc& description, srBinIStream& stream,
                               const srSurfaceIOManager::ImportInfo& options);
    virtual srColorSurfaceIFace* importSurface(srBinIStream& stream,
                                               const srSurfaceIOManager::ImportInfo& options) = 0;
};

/* The provider exports the full member surface including the implicit
   lifecycle sweep (ctor, copy ctor, assignment, destructor) and the vftable, so
   the declaration is dllexport under SURRENDER_BUILD. */
// VTABLE: SURRENDER 0x10075530 srHierarchyIOManager
// class srHierarchyIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srHierarchyIOManager : public srIOManager {
public:
    /* The provider exports the implicit Info assignments (0x10016490/0x100164A0). */
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ImportInfo {
    public:
        // SYNTHETIC: SURRENDER 0x10016490
        // ??4ImportInfo@srHierarchyIOManager@@QAEAAV01@ABV01@@Z

        unsigned char unknown_00;
    };
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ExportInfo {
    public:
        // SYNTHETIC: SURRENDER 0x100164A0
        // ??4ExportInfo@srHierarchyIOManager@@QAEAAV01@ABV01@@Z

        unsigned char unknown_00;
    };

    class HierarchyImporter;
    class HierarchyExporter;

    srHierarchyIOManager();
    srHierarchyIOManager(const srHierarchyIOManager& other);
    srHierarchyIOManager& operator=(const srHierarchyIOManager& other);
    virtual ~srHierarchyIOManager();

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
    // FUNCTION: SURRENDER 0x100164B0
    // ??0HierarchyImporter@srHierarchyIOManager@@QAE@XZ
    HierarchyImporter() {}

    /* Implicit copy ctor/assignment/destructor emitted via the class-level
       dllexport. */
    // SYNTHETIC: SURRENDER 0x100164C0
    // ??0HierarchyImporter@srHierarchyIOManager@@QAE@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x100164D0
    // ??4HierarchyImporter@srHierarchyIOManager@@QAEAAV01@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x100164E0
    // ??1HierarchyImporter@srHierarchyIOManager@@UAE@XZ

    /* importHierarchy's call site dispatches through vtable slot 2. */
    virtual void importHierarchy(srBinIStream& stream, const ImportInfo& options) = 0;
};

/* The provider exports the full member surface including the implicit
   lifecycle sweep (ctor, copy ctor, assignment, destructor) and the vftable, so
   the declaration is dllexport under SURRENDER_BUILD. */
// VTABLE: SURRENDER 0x10075534 srModelIOManager
// class srModelIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srModelIOManager : public srIOManager {
public:
    /* The provider exports the implicit Info assignments (0x100169A0/0x100169B0). */
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ImportInfo {
    public:
        // SYNTHETIC: SURRENDER 0x100169A0
        // ??4ImportInfo@srModelIOManager@@QAEAAV01@ABV01@@Z

        unsigned char unknown_00;
    };
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ExportInfo {
    public:
        // SYNTHETIC: SURRENDER 0x100169B0
        // ??4ExportInfo@srModelIOManager@@QAEAAV01@ABV01@@Z

        unsigned char unknown_00;
    };

    class ModelImporter;
    class ModelExporter;

    srModelIOManager();
    srModelIOManager(const srModelIOManager& other);
    srModelIOManager& operator=(const srModelIOManager& other);
    virtual ~srModelIOManager();

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
    // FUNCTION: SURRENDER 0x100169C0
    // ??0ModelImporter@srModelIOManager@@QAE@XZ
    ModelImporter() {}

    /* Implicit copy ctor/assignment/destructor emitted via the class-level
       dllexport. */
    // SYNTHETIC: SURRENDER 0x100169D0
    // ??0ModelImporter@srModelIOManager@@QAE@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x100169E0
    // ??4ModelImporter@srModelIOManager@@QAEAAV01@ABV01@@Z
    // SYNTHETIC: SURRENDER 0x100169F0
    // ??1ModelImporter@srModelIOManager@@UAE@XZ

    /* importModel's call site dispatches through vtable slot 2. */
    virtual srModel* importModel(srBinIStream& stream, const ImportInfo& options) = 0;
};
